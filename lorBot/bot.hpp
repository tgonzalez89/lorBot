#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
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
constexpr int DEFAULT_SLEEP = 1000;
constexpr int MAX_IMG_SEARCH_TIME = 60;

constexpr POINT BIAS = { 2, 42 };
constexpr POINT HOME = { 54 - BIAS.x, 100 - BIAS.y };
constexpr POINT PLAY = { 54 - BIAS.x, 194 - BIAS.y };
constexpr POINT VS_PLAYER = { 184 - BIAS.x, 84 - BIAS.y };
constexpr POINT VS_AI = { 184 - BIAS.x, 132 - BIAS.y };
constexpr POINT NORMAL = { 824 - BIAS.x, 130 - BIAS.y };
constexpr POINT DECK_1 = { 360 - BIAS.x, 200 - BIAS.y };
constexpr POINT FRIENDS_MENU = { 812 - BIAS.x, 76 - BIAS.y };
constexpr POINT OPTIONS = { 862 - BIAS.x, 74 - BIAS.y };
constexpr POINT HAND_TUCKED = { 800 - BIAS.x, 500 - BIAS.y };
constexpr POINT HAND_EXPANDED = { 470 - BIAS.x, 540 - BIAS.y };
constexpr POINT PLAY_CARD = { 446 - BIAS.x, 340 - BIAS.y };
constexpr POINT ATTACK_SWIPE_START = { 424 - BIAS.x, 514 - BIAS.y };
constexpr POINT ATTACK_SWIPE_END = { 712 - BIAS.x, 408 - BIAS.y };
constexpr POINT BLOCK_SWIPE_START = { 446 - BIAS.x, 480 - BIAS.y };
constexpr POINT BLOCK_SWIPE_END[] = {
	{ 192 - BIAS.x, 340 - BIAS.y },
	{ 296 - BIAS.x, 340 - BIAS.y },
	{ 400 - BIAS.x, 340 - BIAS.y },
	{ 502 - BIAS.x, 340 - BIAS.y },
	{ 604 - BIAS.x, 340 - BIAS.y },
	{ 708 - BIAS.x, 340 - BIAS.y },
};
constexpr POINT OK = { 820 - BIAS.x, 296 - BIAS.y };
constexpr POINT ABOVE_HAND_TUCKED = { 852 - BIAS.x, 410 - BIAS.y };

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

		for (int player = 0; player < (vs_friend ? NUM_PLAYERS : 1); player++) {
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
			std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Game window captured." << std::endl;
			//std::cout << "DEBUG" << " - " << __FUNCTION__ << " - " << "game_HWND:" << game_HWND[player] << std::endl;
			//std::cout << "DEBUG" << " - " << __FUNCTION__ << " - " << "game_window_title:" << game_window_title << std::endl;

			std::stringstream ss;
			ss << util::ADB_EXE << " connect localhost:" << port[player] << std::endl;
			std::system(ss.str().c_str());

			update_status(player);
		}
	}

	void update_window_img_status(const int player)
	{
		game_window_img[player] = image::HWND2Mat(game_HWND[player]);
		if (game_window_img[player].empty()) {
			std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Failed to get game window screenshot." << std::endl;
			return;
		}
	}

	void update_scale_factor_status(const int player)
	{
		RECT game_window_rect;
		if (!GetClientRect(game_HWND[player], &game_window_rect)) {
			std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Failed to get game window rect." << std::endl;
			return;
		}
		scale_factor[player] = static_cast<double>(game_window_rect.bottom) / static_cast<double>(reference_window_size.height);
	}

	void update_status(const int player)
	{
		update_window_img_status(player);
		update_scale_factor_status(player);
	}

	void click_coord(const int player, POINT coord, const unsigned int sleep = DEFAULT_SLEEP, const bool update = true)
	{
		if (update) update_scale_factor_status(player);
		coord = {
			static_cast<LONG>(scale_factor[player] * static_cast<double>(coord.x)),
			static_cast<LONG>(scale_factor[player] * static_cast<double>(coord.y))
		};
		mouse::left_click(game_HWND[player], coord);
		if (sleep) Sleep(sleep);
	}

	void swipe(const int player, POINT coord1, POINT coord2, const unsigned int time = 250, const unsigned int sleep = DEFAULT_SLEEP)
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
		if (sleep) Sleep(sleep);
	}

	bool find_click_img(const int player, const std::string img, const unsigned int sleep = DEFAULT_SLEEP, const bool click = true, const bool update = true)
	{
		if (update) update_status(player);
		cv::Mat resized;
		if (scale_factor[player] == 1.0) {
			resized = ref_imgs[img];
		}
		else {
			cv::resize(ref_imgs[img], resized, cv::Size(), scale_factor[player], scale_factor[player], scale_factor[player] < 1.0 ? cv::INTER_AREA : cv::INTER_CUBIC);
		}
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

	void click_coord_until_img_shows_up(const int player, POINT coord, const std::string img, const unsigned int sleep = DEFAULT_SLEEP, const unsigned int max_img_search_time = MAX_IMG_SEARCH_TIME)
	{
		auto start_t = std::chrono::high_resolution_clock::now();
		auto elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		bool result = false;
		while (!result && elapsed_t <= max_img_search_time) {
			click_coord(player, coord, sleep);
			result = find_img(player, img);
			elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		}
	}

	void click_coord_until_img_dissapears(const int player, POINT coord, const std::string img, const unsigned int sleep = DEFAULT_SLEEP, const unsigned int max_img_search_time = MAX_IMG_SEARCH_TIME)
	{
		auto start_t = std::chrono::high_resolution_clock::now();
		auto elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		bool result = true;
		while (result && elapsed_t <= max_img_search_time) {
			click_coord(player, coord, sleep);
			result = find_img(player, img);
			elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		}
	}

	void find_click_img_until_img_shows_up(const int player, const std::string img1, const std::string img2, const unsigned int sleep = DEFAULT_SLEEP, const unsigned int max_img_search_time = MAX_IMG_SEARCH_TIME)
	{
		auto start_t = std::chrono::high_resolution_clock::now();
		auto elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		bool result = false;
		while (!result && elapsed_t <= max_img_search_time) {
			if (!find_click_img(player, img1, sleep)) Sleep(sleep);
			result = find_img(player, img2);
			elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		}
	}

	void find_click_img_until_img_dissapears(const int player, const std::string img1, const std::string img2, const unsigned int sleep = DEFAULT_SLEEP, const unsigned int max_img_search_time = MAX_IMG_SEARCH_TIME)
	{
		auto start_t = std::chrono::high_resolution_clock::now();
		auto elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		bool result = true;
		while (result && elapsed_t <= max_img_search_time) {
			if (!find_click_img(player, img1, sleep)) Sleep(sleep);
			result = find_img(player, img2);
			elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		}
	}

	void wait_for_img_to_show_up(const int player, const std::string img, const unsigned int sleep = DEFAULT_SLEEP, const unsigned int max_img_search_time = MAX_IMG_SEARCH_TIME)
	{
		auto start_t = std::chrono::high_resolution_clock::now();
		auto elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		while (!find_img(player, img) && elapsed_t <= max_img_search_time) {
			if (sleep) Sleep(sleep);
			elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		}
	}

	void wait_for_img_to_dissapear(const int player, const std::string img, const unsigned int sleep = DEFAULT_SLEEP, const unsigned int max_img_search_time = MAX_IMG_SEARCH_TIME)
	{
		auto start_t = std::chrono::high_resolution_clock::now();
		auto elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		while (find_img(player, img) && elapsed_t <= max_img_search_time) {
			if (sleep) Sleep(sleep);
			elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
		}
	}

	void run_surrender_vs_friend(const int player)
	{
		if (!vs_friend) return;
		std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Running ..." << std::endl;

		click_coord(player, HOME);
		click_coord(!player, HOME);
		click_coord_until_img_shows_up(player, FRIENDS_MENU, "friends_icons");
		//TODO, if img "friend" not found, click on arrow to expand friends list.
		find_click_img_until_img_shows_up(player, "friend", "challenge");
		find_click_img_until_img_shows_up(player, "challenge", "normal_match");
		find_click_img_until_img_shows_up(player, "normal_match", "cancel_challenge");
		wait_for_img_to_show_up(!player, "accept");
		find_click_img_until_img_dissapears(!player, "accept", "accept");
		Sleep(4000);
		if (find_img(player, "cancel_challenge")) {
			find_click_img_until_img_dissapears(player, "cancel_challenge", "cancel_challenge");
		}

		const int half = vs_friend / 2;
		const int total = vs_friend;
		while (vs_friend) {
			SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
			std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Starting match #" << total - vs_friend + 1 << std::endl;

			wait_for_img_to_show_up(player, "select_your_deck");
			wait_for_img_to_show_up(!player, "select_your_deck");
			click_coord_until_img_shows_up(player, DECK_1, "ready");
			click_coord_until_img_shows_up(!player, DECK_1, "ready");
			find_click_img_until_img_dissapears(player, "ready", "ready");
			find_click_img_until_img_dissapears(!player, "ready", "ready");
			wait_for_img_to_show_up(player, "ok");
			wait_for_img_to_show_up(!player, "ok");

			int surr_player = vs_friend > half ? !player : player;
			click_coord_until_img_shows_up(surr_player, OPTIONS, "surrender");
			find_click_img_until_img_shows_up(surr_player, "surrender", "ok_surrender");
			find_click_img_until_img_dissapears(surr_player, "ok_surrender", "ok_surrender");

			click_coord_until_img_shows_up(player, ABOVE_HAND_TUCKED, "continue");
			click_coord_until_img_shows_up(!player, ABOVE_HAND_TUCKED, "continue");
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
		std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Running ..." << std::endl;

		click_coord(player, PLAY, 2000);
		click_coord_until_img_shows_up(player, VS_PLAYER, "vs_player");
		click_coord_until_img_shows_up(player, DECK_1, "play_deck");
		click_coord_until_img_shows_up(player, NORMAL, "normal");

		const int total = vs_player;
		while (vs_player) {
			SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
			std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Starting match #" << total - vs_player + 1 << std::endl;

			find_click_img_until_img_dissapears(player, "play_deck", "play_deck");
			wait_for_img_to_show_up(player, "ok"); // TODO: check if matchmaking failed
			click_coord_until_img_shows_up(player, OPTIONS, "surrender", 1000, 15);
			find_click_img_until_img_shows_up(player, "surrender", "ok_surrender", 1000, 5);
			find_click_img_until_img_dissapears(player, "ok_surrender", "ok_surrender", 1000, 5);
			click_coord_until_img_shows_up(player, ABOVE_HAND_TUCKED, "continue");
			find_click_img_until_img_dissapears(player, "continue", "continue");
			wait_for_img_to_show_up(player, "play_deck");
			vs_player--;
		}

		click_coord_until_img_dissapears(player, HOME, "play_deck");
		click_coord_until_img_dissapears(player, HOME, "select_your_deck");
		Sleep(4000);
	}

	void run_surrender_vs_ai(const int player)
	{
		if (!vs_ai) return;
		std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Running ..." << std::endl;

		click_coord(player, PLAY, 2000);
		click_coord_until_img_shows_up(player, VS_AI, "vs_ai");
		click_coord_until_img_shows_up(player, DECK_1, "play_deck");

		const int total = vs_ai;
		while (vs_ai) {
			SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
			std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Starting match #" << total - vs_ai + 1 << std::endl;

			find_click_img_until_img_dissapears(player, "play_deck", "play_deck");
			wait_for_img_to_show_up(player, "ok");
			click_coord_until_img_shows_up(player, OPTIONS, "surrender");
			find_click_img_until_img_shows_up(player, "surrender", "ok_surrender");
			find_click_img_until_img_dissapears(player, "ok_surrender", "ok_surrender");
			click_coord_until_img_shows_up(player, ABOVE_HAND_TUCKED, "continue");
			find_click_img_until_img_dissapears(player, "continue", "continue");
			wait_for_img_to_show_up(player, "play_deck");
			vs_ai--;
		}

		click_coord_until_img_dissapears(player, HOME, "play_deck");
		click_coord_until_img_dissapears(player, HOME, "select_your_deck");
		Sleep(4000);
	}

	void run_xp_farm(const int player)
	{
		if (!xp_farm) return;
		std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Running ..." << std::endl;

		click_coord(player, PLAY, 2000);
		click_coord_until_img_shows_up(player, VS_PLAYER, "vs_player");
		click_coord_until_img_shows_up(player, DECK_1, "play_deck");
		click_coord_until_img_shows_up(player, NORMAL, "normal");

		int total_duration = 0;
		const int total = xp_farm;
		while (xp_farm) {
			SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
			std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Starting match #" << total - xp_farm + 1 << std::endl;

			auto start_t = std::chrono::high_resolution_clock::now();
			find_click_img_until_img_dissapears(player, "play_deck", "play_deck");
			wait_for_img_to_show_up(player, "ok"); // TODO: check if matchmaking failed
			find_click_img_until_img_dissapears(player, "ok", "ok");
			auto elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();

			bool full_life = true;
			while (elapsed_t <= max_match_time && !find_img(player, "continue")) {
				update_status(player);
				if (find_img(player, "pass", false) || find_img(player, "end_round", false)) {
					SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
					if (!find_img(player, "full_life")) { // Check if player is bot or human by checking life totals
						full_life = false;
						break;
					}
					click_coord(player, HAND_TUCKED, 500); // Open hand.
					swipe(player, HAND_EXPANDED, PLAY_CARD, 250, 500); // Try to play a card.

					bool attack = false;
					update_status(player);
					if (find_img(player, "select_target", false)) { // If the played card is a creature and there is no space, return it to the hand.
						swipe(player, PLAY_CARD, HAND_EXPANDED, 250, 500);
						attack = true;
					}
					else if (find_img(player, "ok", false)) { // If the played card is a fast/slow spell, click OK to play the card.
						find_click_img_until_img_dissapears(player, "ok", "ok", 500);
					}
					else { // In case there is not enough mana, no card is played and the hand stays open, click on a 'neutral' spot to close the hand.
						click_coord(player, ABOVE_HAND_TUCKED, 500);
						attack = true;
					}

					update_status(player);
					// Check if attacking is possible.
					if (attack && find_img(player, "attack_token", false) && (find_img(player, "pass", false) || find_img(player, "end_round", false))) {
						int count = 0;
						// Move allies to attacking position.
						while (count < 6 && !find_img(player, "empty_board")) {
							swipe(player, ATTACK_SWIPE_START, ATTACK_SWIPE_END, 250, 500);
							count++;
						}
						find_click_img_until_img_dissapears(player, "attack", "attack", 500);
					}

					click_coord(player, OK, 500); // Click to pass the turn (just in case something went wrong).
				}
				else if (find_img(player, "skip_block", false)) {
					int count = 0;
					// Move allies to blocking position.
					while (count < 6 && !find_img(player, "empty_board")) {
						swipe(player, BLOCK_SWIPE_START, BLOCK_SWIPE_END[count], 250, 500);
						count++;
					}
					find_click_img_until_img_dissapears(player, "block", "block", 500);
				}
				else if (find_img(player, "ok", false) || find_img(player, "attack", false) || find_img(player, "block", false) || find_img(player, "replace", false)) { // Just in case something went wrong.
					click_coord(player, OK);
				}
				else {
					click_coord(player, ABOVE_HAND_TUCKED);
				}

				elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
			}

			if (elapsed_t > max_match_time || !full_life) {
				std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Surrendering ..." << std::endl;
				click_coord_until_img_shows_up(player, OPTIONS, "surrender", 1000, 15);
				find_click_img_until_img_shows_up(player, "surrender", "ok_surrender", 1000, 5);
				find_click_img_until_img_dissapears(player, "ok_surrender", "ok_surrender", 1000, 5);
			}

			click_coord_until_img_shows_up(player, ABOVE_HAND_TUCKED, "continue");
			find_click_img_until_img_dissapears(player, "continue", "continue");
			wait_for_img_to_show_up(player, "play_deck");

			xp_farm--;
			elapsed_t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_t).count();
			total_duration += static_cast<int>(elapsed_t);

			std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Match #" << total-xp_farm << " ended. Duration: " << elapsed_t << " s. Average duration: " << total_duration/(total-xp_farm) << " s." << std::endl;
		}

		click_coord_until_img_dissapears(player, HOME, "play_deck");
		click_coord_until_img_dissapears(player, HOME, "select_your_deck");
		Sleep(4000);
	}

public:

	void run()
	{
		init();
		std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Running bot ..." << std::endl;
		
		SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
		run_surrender_vs_friend(0);
		run_surrender_vs_player(0);
		run_surrender_vs_ai(0);
		run_xp_farm(0);
		SetThreadExecutionState(ES_CONTINUOUS);

		for (int player = 0; player < (vs_friend ? NUM_PLAYERS : 1); player++) {
			std::stringstream ss;
			ss << util::ADB_EXE << " disconnect localhost:" << port[player] << std::endl;
			std::system(ss.str().c_str());
		}
	}

};
