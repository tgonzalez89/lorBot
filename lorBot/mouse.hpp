#pragma once

#include <cstdlib>
#include <Windows.h>
#include "util.hpp"

namespace mouse {

    void left_click(const HWND window, const POINT p)
    {
        PostMessage(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(p.x, p.y));
        Sleep(1);
        PostMessage(window, WM_LBUTTONUP, 0, MAKELPARAM(p.x, p.y));
        Sleep(1);
    }

    void drag(const HWND window, const POINT p1, const POINT p2, const unsigned int speed = 1)
    {
        int x_diff = p2.x - p1.x;
        int y_diff = p2.y - p1.y;
        int i_max = std::abs(x_diff) > std::abs(y_diff) ? std::abs(x_diff) : std::abs(y_diff);
        POINT p = p1;
        PostMessage(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(p.x, p.y));
        Sleep(1);
        for (int i = 0; i <= i_max; i++) {
            p.x = p1.x + i * x_diff / i_max;
            p.y = p1.y + i * y_diff / i_max;
            PostMessage(window, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(p.x, p.y));
            Sleep(speed);
        }
        PostMessage(window, WM_LBUTTONUP, 0, MAKELPARAM(p.x, p.y));
        Sleep(1);
    }

    void drag_adb(const int port, const POINT p1, const POINT p2, const int time = 250) {
        std::stringstream ss;
        ss << util::ADB_EXE << " -s localhost:" << port << " shell input touchscreen swipe " << p1.x << " " << p1.y << " " << p2.x << " " << p2.y << " " << time;
        std::system(ss.str().c_str());
    }

    void drag_adb2(const int port, const POINT p1, const POINT p2, const int max_x, const int max_y)
    {
        int x_diff = p2.x - p1.x;
        int y_diff = p2.y - p1.y;
        int i_max = std::abs(x_diff) > std::abs(y_diff) ? std::abs(x_diff) : std::abs(y_diff);
        POINT p = p1;

        for (int i = 0; i <= i_max; i += 20) {
            p.x = p1.x + i * x_diff / i_max;
            p.y = p1.y + i * y_diff / i_max;
            std::stringstream ss;
            ss << util::ADB_EXE << " -s localhost:" << port << " shell \"";
            ss << "sendevent /dev/input/event6 3 53 " << p.x * 32767 / max_x <<  " && ";
            ss << "sendevent /dev/input/event6 3 54 " << p.y * 32767 / max_y << " && ";
            ss << "sendevent /dev/input/event6 0 2 0 && ";
            ss << "sendevent /dev/input/event6 0 0 0\"";
            std::system(ss.str().c_str());
        }

        std::stringstream ss;
        ss << util::ADB_EXE << " -s localhost:" << port << " shell \"";
        ss << "sendevent /dev/input/event6 0 2 0 && ";
        ss << "sendevent /dev/input/event6 0 0 0\"";
        std::system(ss.str().c_str());
    }

}
