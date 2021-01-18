#pragma once

#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <Windows.h>

namespace image {

    cv::Mat HWND2Mat(const HWND WindowHWND) {
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

    std::vector<cv::Point> search_img_inside_other_img(const cv::Mat img, const cv::Mat other_img, const double tol_perc, const char mode, const int match_method, const std::string img_id = "") {
        std::vector<cv::Point> coords;

        if (img.empty()) {
            std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Image to search is empty. " << img_id << std::endl;
            return coords;
        }
        if (img.empty()) {
            std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Image to search into is empty. " << img_id << std::endl;
            return coords;
        }
        if (img.cols > other_img.cols || img.rows > other_img.rows) {
            std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Image dimentions are bigger than image to search into. " << img_id << std::endl;
            return coords;
        }
        if (img.type() != other_img.type()) {
            std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Image and image to search into must be of the same type. " << img_id << std::endl;
            return coords;
        }
        if (match_method < 0 || match_method > 5) {
            std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Match method must be one of: TM_SQDIFF, TM_SQDIFF_NORMED, TM_CCORR, TM_CCORR_NORMED, TM_CCOEFF or TM_CCOEFF_NORMED. " << img_id << std::endl;
            return coords;
        }
        if (tol_perc < 0.0 || tol_perc > 1.0) {
            std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Tolerance percentage must be between 0 and 1. " << img_id << std::endl;
            return coords;
        }
        if (mode != 'm' && mode != 's') {
            std::cout << "ERROR" << " - " << __FUNCTION__ << " - " << "Mode must be 's' to search for a single instance or 'm' for multiple instances. " << img_id << std::endl;
            return coords;
        }

        // Create an image with img appended to it so that the algorithm that searches has a reference of a perfect match.
        cv::Mat img_extended;
        cv::Mat img_to_search_into;
        cv::copyMakeBorder(img, img_extended, img.rows/5, img.rows/5, img.cols/5, img.cols/5, cv::BORDER_REPLICATE);
        cv::copyMakeBorder(other_img, img_to_search_into, 0, img_extended.rows, 0, img_extended.cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));
        img_extended.copyTo(img_to_search_into.rowRange(0, img_extended.rows).colRange(img_to_search_into.cols - img_extended.cols, img_to_search_into.cols));

        // Search for the image and normalize the results.
        cv::Mat result;
        cv::matchTemplate(img_to_search_into, img, result, match_method);
        cv::normalize(result, result, 0, 1, cv::NORM_MINMAX);

        // Get the coords and values of the image matches.
        double min_val, max_val, match_val = 0.0;
        cv::Point min_loc, max_loc, match_loc;
        const int rect_color = (match_method == cv::TM_SQDIFF || match_method == cv::TM_SQDIFF_NORMED) ? 255 : 0;
        // Loop until matches are worse than the accepted tolerance.
        while (match_val <= tol_perc) {
            // Get the min/max values and their locations.
            cv::minMaxLoc(result, &min_val, &max_val, &min_loc, &max_loc);
            // Set match_loc and match_val depending onf the match method.
            if (match_method == cv::TM_SQDIFF || match_method == cv::TM_SQDIFF_NORMED) {
                match_loc = min_loc;
                match_val = min_val;
            }
            else {
                match_loc = max_loc;
                match_val = 1.0 - max_val;
            }

            // Adjust match location to be the center of the image.
            match_loc.x += img.cols / 2;
            match_loc.y += img.rows / 2;

            // Check if image was found.
            if (match_loc.x < other_img.cols && match_loc.y < other_img.rows && match_val <= tol_perc) {
                coords.push_back(match_loc);
                std::cout << "DEBUG" << " - " << __FUNCTION__ << " - " << "Image found: match_val: " << match_val << " | match_loc: " << match_loc << " | img_id: " << img_id << std::endl;
                // If only a sigle image is needed, no need to do more work.
                if (mode == 's') break;
            }
            else if (match_val > tol_perc) {
                std::cout << "DEBUG" << " - " << __FUNCTION__ << " - " << "Image not found: match_val: " << match_val << " | match_loc: " << match_loc << " | img_id: " << img_id << std::endl;
            }

            // Erase the current match to leave it out of the min/max search in the next iteration.
            cv::rectangle(result, cv::Point(match_loc.x - img.cols, match_loc.y - img.rows), cv::Point(match_loc.x, match_loc.y), cv::Scalar::all(rect_color), cv::FILLED, 8, 0);

            //cv::imshow("DEBUG", result);
            //cv::waitKey(0);
        }

        return coords;
    }

}
