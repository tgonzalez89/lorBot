#pragma once

#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_map>
#include <opencv2/opencv.hpp>
#include <Windows.h>
#include "image.hpp"
#include "mouse.hpp"
#include "util.hpp"

constexpr int NUM_PLAYERS = 2;
constexpr char IMAGES_PATH[] = "..\\..\\images";
constexpr char CONFIG_FILE[] = "..\\..\\config.txt";
constexpr int DEFAULT_SLEEP = 2000;

constexpr POINT BIAS = { 2, 42 };
constexpr POINT HOME = { 54 - BIAS.x, 100 - BIAS.y };
constexpr POINT PLAY = { 54 - BIAS.x, 194 - BIAS.y };
constexpr POINT VS_PLAYER = { 184 - BIAS.x, 84 - BIAS.y };
constexpr POINT VS_AI = { 184 - BIAS.x, 132 - BIAS.y };
constexpr POINT DECK_1 = { 360 - BIAS.x, 200 - BIAS.y };
constexpr POINT FRIENDS_MENU = { 812 - BIAS.x, 76 - BIAS.y };
constexpr POINT OPTIONS = { 862 - BIAS.x, 74 - BIAS.y };
constexpr POINT NORMAL = { 824 - BIAS.x, 130 - BIAS.y };

constexpr int REFERENCE_WINDOW_SIZE_WIDTH = 896;
constexpr int REFERENCE_WINDOW_SIZE_HEIGHT = 503;
constexpr int INTERNAL_RESOLUTION_WIDTH = 1280;
constexpr int INTERNAL_RESOLUTION_HEIGHT = 720;
constexpr int PORT[] = { 5555, 5565 };
constexpr double IMG_MATCH_TOL_PERC = 0.08;
constexpr int NUM_MATCHES = 10;
constexpr int MAX_MATCH_TIME = 300;

class Bot
{
private:

	HWND game_HWND[NUM_PLAYERS];
	cv::Mat game_window_img[NUM_PLAYERS];
	double scale_factor[NUM_PLAYERS];
	std::unordered_map<std::string, cv::Mat> ref_imgs;
	std::unordered_map<std::string, POINT> coords;

	struct size {
		int width;
		int height;
	};
	size reference_window_size = { REFERENCE_WINDOW_SIZE_WIDTH, REFERENCE_WINDOW_SIZE_HEIGHT };
	size internal_resolution = { INTERNAL_RESOLUTION_WIDTH, INTERNAL_RESOLUTION_HEIGHT };
	int port[NUM_PLAYERS] = { PORT[0], PORT[1] };
	double img_match_tol_perc = IMG_MATCH_TOL_PERC;
	int vs_friend = NUM_MATCHES;
	int vs_player = NUM_MATCHES;
	int vs_ai = NUM_MATCHES;
	int xp_farm = NUM_MATCHES;
	int max_match_time = MAX_MATCH_TIME;

	void read_config()
	{
		std::ifstream config_file(CONFIG_FILE);
		if (!config_file.is_open()) {
			std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Failed to open file " << CONFIG_FILE << std::endl;
			return;
		}
		std::string line;
		int line_num = 1;
		const std::string delim = "=";
		while (std::getline(config_file, line)) {
			line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
			size_t pos = line.find(delim);
			std::string setting_name = line.substr(0, pos);
			line.erase(0, pos + delim.length());
			try {
				if (setting_name == "img_match_tol_perc") {
					img_match_tol_perc = std::stod(line);
				}
				else if (setting_name == "reference_window_size.width") {
					reference_window_size.width = std::stoul(line);
				}
				else if (setting_name == "reference_window_size.height") {
					reference_window_size.height = std::stoul(line);
				}
				else if (setting_name == "internal_resolution.width") {
					internal_resolution.width = std::stoul(line);
				}
				else if (setting_name == "internal_resolution.height") {
					internal_resolution.height = std::stoul(line);
				}
				else if (setting_name == "port[0]") {
					port[0] = std::stoul(line);
				}
				else if (setting_name == "port[1]") {
					port[1] = std::stoul(line);
				}
				else if (setting_name == "vs_friend") {
					vs_friend = std::stoul(line);
				}
				else if (setting_name == "vs_player") {
					vs_player = std::stoul(line);
				}
				else if (setting_name == "vs_ai") {
					vs_ai = std::stoul(line);
				}
				else if (setting_name == "xp_farm") {
					xp_farm = std::stoul(line);
				}
				else if (setting_name == "max_match_time") {
					max_match_time = std::stoul(line);
				}
			}
			catch (...) {
				std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Failed to parse line " << line_num << " of file " << CONFIG_FILE << std::endl;
			}
			line_num++;
		}
		config_file.close();
	}

	void load_imgs()
	{
		for (const auto& entry : std::filesystem::directory_iterator(IMAGES_PATH)) {
			cv::Mat img = cv::imread(entry.path().string(), cv::IMREAD_COLOR);
			if (img.empty()) {
				std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Failed to load image " << entry.path() << std::endl;
				return;
			}
			ref_imgs[entry.path().stem().string()] = img.clone();
		}
	}

	void init()
	{
		read_config();
		load_imgs();

		for (int player = 0; player < NUM_PLAYERS; player++) {
			std::cout << "Place the mouse cursor over player " << player+1 << " game window." << std::endl;
			std::cout << "    Window will be captured in ..." << std::endl;
			for (int i = 3; i >= 1; i--) {
				std::cout << "    " << i << std::endl;
				Sleep(1000);
			}

			POINT mouse_pos;
			GetCursorPos(&mouse_pos);
			game_HWND[player] = WindowFromPoint(mouse_pos);

			char game_window_title[128];
			GetWindowTextA(game_HWND[player], game_window_title, 128);
			std::cout << "DEBUG" << " - " << __FUNCTION__ << " - " << "Game window captured:" << std::endl;
			std::cout << "    game_HWND:         " << game_HWND[player] << std::endl;
			std::cout << "    game_window_title: " << game_window_title << std::endl;

			std::stringstream ss;
			ss << util::ADB_EXE << " connect localhost:" << port[player] << std::endl;
			std::system(ss.str().c_str());

			update_status(player);
		}
	}

	void update_status(const int player)
	{
		game_window_img[player] = image::HWND2Mat(game_HWND[player]);
		if (game_window_img[player].empty()) {
			std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Failed to get game window screenshot." << std::endl;
			return;
		}

		RECT game_window_rect;
		if (!GetClientRect(game_HWND[player], &game_window_rect)) {
			std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Failed to get game window rect." << std::endl;
			return;
		}
		scale_factor[player] = static_cast<double>(game_window_rect.bottom) / static_cast<double>(reference_window_size.height);
	}

	void click_coord(const int player, POINT coord, const unsigned int sleep = DEFAULT_SLEEP, const bool update = true)
	{
		if (update) update_status(player);
		coord = {
			static_cast<LONG>(scale_factor[player] * static_cast<double>(coord.x)),
			static_cast<LONG>(scale_factor[player] * static_cast<double>(coord.y))
		};
		mouse::left_click(game_HWND[player], coord);
		if (sleep) Sleep(sleep);
	}

	void swipe(const int player, POINT coord1, POINT coord2, const int time = 250)
	{
		coord1 = {
			coord1.x * internal_resolution.width / reference_window_size.width,
			coord1.y * internal_resolution.height / reference_window_size.height
		};
		coord2 = {
			coord2.x * internal_resolution.width / reference_window_size.width,
			coord2.y * internal_resolution.height / reference_window_size.height
		};
		mouse::drag_adb(port[player], coord1, coord2, time);
	}

	bool find_click_img(const int player, const std::string img, const unsigned int sleep = DEFAULT_SLEEP, const bool click = true, const bool update = true)
	{
		if (update) update_status(player);
		cv::Mat resized;
		cv::resize(ref_imgs[img], resized, cv::Size(), scale_factor[player], scale_factor[player], scale_factor[player] < 1.0 ? cv::INTER_AREA : cv::INTER_CUBIC);
		std::vector<cv::Point> result = image::search_img_inside_other_img(resized, game_window_img[player], img_match_tol_perc, 's', cv::TM_CCOEFF_NORMED, img);
		if (result.size() != 1) {
			return false;
		}
		if (click) {
			click_coord(player, { result[0].x, result[0].y }, sleep, false);
		}
		return true;
	}

	bool find_img(const int player, const std::string img, const bool update = true)
	{
		return find_click_img(player, img, 0, false, update);
	}

	void click_coord_until_img_shows_up(const int player, POINT coord, const std::string img, const unsigned int sleep = DEFAULT_SLEEP)
	{
		bool result = false;
		while (!result) {
			click_coord(player, coord, sleep);
			result = find_img(player, img);
		}
	}

	void click_coord_until_img_dissapears(const int player, POINT coord, const std::string img, const unsigned int sleep = DEFAULT_SLEEP)
	{
		bool result = true;
		while (result) {
			click_coord(player, coord, sleep);
			result = find_img(player, img);
		}
	}

	void find_click_img_until_img_shows_up(const int player, const std::string img1, const std::string img2, const unsigned int sleep = DEFAULT_SLEEP)
	{
		bool result = false;
		while (!result) {
			find_click_img(player, img1, sleep);
			result = find_img(player, img2);
		}
	}

	void find_click_img_until_img_dissapears(const int player, const std::string img1, const std::string img2, const unsigned int sleep = DEFAULT_SLEEP)
	{
		bool result = true;
		while (result) {
			find_click_img(player, img1, sleep);
			result = find_img(player, img2);
		}
	}

	void wait_for_img_to_show_up(const int player, const std::string img, const unsigned int sleep = DEFAULT_SLEEP)
	{
		while (!find_img(player, img)) {
			if (sleep) Sleep(sleep);
		}
	}

	void wait_for_img_to_dissapear(const int player, const std::string img, const unsigned int sleep = DEFAULT_SLEEP)
	{
		while (find_img(player, img)) {
			if (sleep) Sleep(sleep);
		}
	}

	void run_surrender_vs_friend(const int player)
	{
		if (!vs_friend) return;
		std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Running surrender_vs_friend ..." << std::endl;

		click_coord(player, coords["home"]);
		click_coord(!player, coords["home"]);
		click_coord_until_img_shows_up(player, coords["friends_menu"], "friends_icons");
		find_click_img_until_img_shows_up(player, "friend", "challenge");
		find_click_img_until_img_shows_up(player, "challenge", "normal_match");
		find_click_img_until_img_shows_up(player, "normal_match", "cancel_challenge");
		wait_for_img_to_show_up(!player, "accept");
		find_click_img_until_img_dissapears(!player, "accept", "accept", DEFAULT_SLEEP*2);
		if (find_img(player, "cancel_challenge")) {
			find_click_img_until_img_dissapears(player, "cancel_challenge", "cancel_challenge");
		}

		int half = vs_friend / 2;
		while (vs_friend) {
			wait_for_img_to_show_up(player, "select_your_deck");
			wait_for_img_to_show_up(!player, "select_your_deck");
			click_coord_until_img_shows_up(player, coords["deck_1"], "ready");
			click_coord_until_img_shows_up(!player, coords["deck_1"], "ready");
			find_click_img_until_img_dissapears(player, "ready", "ready");
			find_click_img_until_img_dissapears(!player, "ready", "ready");
			wait_for_img_to_show_up(player, "ok");
			wait_for_img_to_show_up(!player, "ok");

			int surr_player = vs_friend > half ? !player : player;
			click_coord_until_img_shows_up(surr_player, coords["options"], "surrender");
			find_click_img_until_img_shows_up(surr_player, "surrender", "ok_surrender");
			find_click_img_until_img_dissapears(surr_player, "ok_surrender", "ok_surrender");

			click_coord_until_img_shows_up(player, coords["deck_1"], "continue");
			click_coord_until_img_shows_up(!player, coords["deck_1"], "continue");
			find_click_img_until_img_dissapears(player, "continue", "continue");
			find_click_img_until_img_dissapears(!player, "continue", "continue");

			vs_friend--;
		}

		find_click_img_until_img_dissapears(player, "exit_lobby", "exit_lobby");
		wait_for_img_to_show_up(!player, "ok_exit_lobby");
		find_click_img_until_img_dissapears(!player, "ok_exit_lobby", "ok_exit_lobby");
	}

	void run_surrender_vs_player(const int player)
	{
		if (!vs_player) return;
		std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Running surrender_vs_player ..." << std::endl;

		click_coord(player, coords["play"]);
		click_coord_until_img_shows_up(player, coords["vs_player"], "vs_player");
		click_coord_until_img_shows_up(player, coords["deck_1"], "play_deck");
		click_coord_until_img_shows_up(player, coords["normal"], "normal");

		while (vs_player) {
			find_click_img_until_img_dissapears(player, "play_deck", "play_deck");
			wait_for_img_to_show_up(player, "ok");
			click_coord_until_img_shows_up(player, coords["options"], "surrender");
			find_click_img_until_img_shows_up(player, "surrender", "ok_surrender");
			find_click_img_until_img_dissapears(player, "ok_surrender", "ok_surrender");
			click_coord_until_img_shows_up(player, coords["deck_1"], "continue");
			find_click_img_until_img_dissapears(player, "continue", "continue");
			wait_for_img_to_show_up(player, "play_deck");
			vs_player--;
		}

		click_coord_until_img_dissapears(player, coords["home"], "play_deck");
		click_coord_until_img_dissapears(player, coords["home"], "select_your_deck");
	}

	void run_surrender_vs_ai(const int player)
	{
		if (!vs_ai) return;
		std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Running surrender_vs_ai ..." << std::endl;

		click_coord(player, coords["play"]);
		click_coord_until_img_shows_up(player, coords["vs_ai"], "vs_ai");
		click_coord_until_img_shows_up(player, coords["deck_1"], "play_deck");
		while (vs_ai) {
			find_click_img_until_img_dissapears(player, "play_deck", "play_deck");
			wait_for_img_to_show_up(player, "ok");
			click_coord_until_img_shows_up(player, coords["options"], "surrender");
			find_click_img_until_img_shows_up(player, "surrender", "ok_surrender");
			find_click_img_until_img_dissapears(player, "ok_surrender", "ok_surrender");
			click_coord_until_img_shows_up(player, coords["deck_1"], "continue");
			find_click_img_until_img_dissapears(player, "continue", "continue");
			wait_for_img_to_show_up(player, "play_deck");
			vs_ai--;
		}

		click_coord_until_img_dissapears(player, coords["home"], "play_deck");
		click_coord_until_img_dissapears(player, coords["home"], "select_your_deck");
	}

	void run_xp_farm(const int player)
	{
		if (!xp_farm) return;
		std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Running xp_farm ..." << std::endl;
	}

public:

	Bot()
	{
		coords["home"] = HOME;
		coords["play"] = PLAY;
		coords["vs_player"] = VS_PLAYER;
		coords["vs_ai"] = VS_AI;
		coords["deck_1"] = DECK_1;
		coords["friends_menu"] = FRIENDS_MENU;
		coords["options"] = OPTIONS;
		coords["normal"] = NORMAL;

		init();
	}

	void run()
	{
		std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Running bot ..." << std::endl;

		run_surrender_vs_friend(0);
		run_surrender_vs_player(0);
		run_surrender_vs_ai(0);
		run_xp_farm(0);

		for (int player = 0; player < NUM_PLAYERS; player++) {
			std::stringstream ss;
			ss << util::ADB_EXE << " disconnect localhost:" << port[player] << std::endl;
			std::system(ss.str().c_str());
		}
	}

};
