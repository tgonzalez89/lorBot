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

constexpr POINT BIAS = { 2, 42 };
constexpr POINT HOME = { 54 - BIAS.x, 100 - BIAS.y };
constexpr POINT PLAY = { 54 - BIAS.x, 194 - BIAS.y };

constexpr int REFERENCE_WINDOW_SIZE_WIDTH = 896;
constexpr int REFERENCE_WINDOW_SIZE_HEIGHT = 503;
constexpr int INTERNAL_RESOLUTION_WIDTH = 1280;
constexpr int INTERNAL_RESOLUTION_HEIGHT = 720;
constexpr int PORT[] = { 5555, 5565 };
constexpr float IMG_MATCH_TOL_PERC = 0.08f;
constexpr int NUM_MATCHES = 10;
constexpr int MAX_MATCH_TIME = 300;

class Bot
{
private:

	HWND game_HWND[NUM_PLAYERS];
	cv::Mat game_window_img[NUM_PLAYERS];
	float scale_factor[NUM_PLAYERS];
	std::unordered_map<std::string, cv::Mat> ref_imgs;
	std::unordered_map<std::string, POINT> coords;

	struct size {
		int width;
		int height;
	};
	size reference_window_size = { REFERENCE_WINDOW_SIZE_WIDTH, REFERENCE_WINDOW_SIZE_HEIGHT };
	size internal_resolution = { INTERNAL_RESOLUTION_WIDTH, INTERNAL_RESOLUTION_HEIGHT };
	int port[NUM_PLAYERS] = { PORT[0], PORT[1] };
	float img_match_tol_perc = IMG_MATCH_TOL_PERC;
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
					img_match_tol_perc = std::stof(line);
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
		scale_factor[player] = static_cast<float>(game_window_rect.bottom) / static_cast<float>(reference_window_size.height);
	}

	void click(const int player, POINT coord)
	{
		coord = {
			static_cast<LONG>(scale_factor[player] * static_cast<float>(coord.x)),
			static_cast<LONG>(scale_factor[player] * static_cast<float>(coord.y))
		};
		mouse::left_click(game_HWND[player], coord);
	}

	void drag(const int player, POINT coord1, POINT coord2, const int time = 250)
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

public:

	Bot()
	{
		coords["home"] = HOME;
		coords["play"] = PLAY;

		init();
	}

	void run()
	{
		std::cout << "INFO" << " - " << __FUNCTION__ << " - " << "Running bot ..." << std::endl;

		//click(0, coords["home"]);

		for (int player = 0; player < NUM_PLAYERS; player++) {
			std::stringstream ss;
			ss << util::ADB_EXE << " disconnect localhost:" << port[player] << std::endl;
			std::system(ss.str().c_str());
		}
	}

};
