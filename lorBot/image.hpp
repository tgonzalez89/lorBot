#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>
#include <Windows.h>

namespace image {

    cv::Mat HWND2Mat(HWND WindowHWND) {
        if (WindowHWND == NULL) {
            std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Couldn't find the window." << std::endl;
            cv::Mat fail;
            return fail;
        }

        RECT WindowRect;
        GetWindowRect(WindowHWND, &WindowRect);
        int WindowWidth = WindowRect.right - WindowRect.left;
        int WindowHeight = WindowRect.bottom - WindowRect.top;
        HDC WindowHDC = GetDC(WindowHWND);
        HDC WindowCompatibleHDC = CreateCompatibleDC(WindowHDC);
        HBITMAP WindowBitmap = CreateCompatibleBitmap(WindowHDC, WindowWidth, WindowHeight);
        SelectObject(WindowCompatibleHDC, WindowBitmap);
        PrintWindow(WindowHWND, WindowCompatibleHDC, 2);

        BITMAPINFOHEADER bi;
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = WindowWidth;
        bi.biHeight = -WindowHeight; //Negative indicates it's a top-down bitmap with it's origin at the upper-left corner
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;

        cv::Mat WindowMat(WindowHeight, WindowWidth, CV_8UC4);
        GetDIBits(WindowCompatibleHDC, WindowBitmap, 0, WindowHeight, WindowMat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        cvtColor(WindowMat, WindowMat, cv::COLOR_BGRA2BGR);

        DeleteObject(WindowBitmap);
        DeleteDC(WindowCompatibleHDC);
        ReleaseDC(WindowHWND, WindowHDC);

        return WindowMat;
    }

}
