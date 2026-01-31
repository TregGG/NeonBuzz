#include "ImageProcessor.h"
#include <iostream>
#include <random>

ImageProcessor::ImageProcessor() {
}

ImageProcessor::~ImageProcessor() {
}

bool ImageProcessor::loadImage(const std::string& filepath) {
    originalImage = cv::imread(filepath);
    if (originalImage.empty()) {
        std::cerr << "Failed to load image: " << filepath << std::endl;
        return false;
    }

    // Convert to RGB if BGR
    if (originalImage.channels() == 3) {
        cv::cvtColor(originalImage, originalImage, cv::COLOR_BGR2RGB);
    }

    // Limit image size for performance
    const int maxDim = 1024;
    if (originalImage.cols > maxDim || originalImage.rows > maxDim) {
        float scale = static_cast<float>(maxDim) / std::max(originalImage.cols, originalImage.rows);
        cv::resize(originalImage, originalImage, cv::Size(), scale, scale, cv::INTER_AREA);
    }

    processedImage = originalImage.clone();
    return true;
}

void ImageProcessor::processImage() {
    if (originalImage.empty()) {
        return;
    }

    detectEdges();
    findContours();
    createBrushStrokes();
}

void ImageProcessor::detectEdges() {
    cv::Mat gray;
    if (originalImage.channels() == 3) {
        cv::cvtColor(originalImage, gray, cv::COLOR_RGB2GRAY);
    } else {
        gray = originalImage.clone();
    }

    // Apply Gaussian blur to reduce noise
    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 1.5);

    // Canny edge detection
    cv::Canny(gray, edgeImage, cannyThreshold1, cannyThreshold2);
}

void ImageProcessor::findContours() {
    if (edgeImage.empty()) {
        return;
    }

    contours.clear();
    cv::Mat tempEdge = edgeImage.clone();
    std::vector<cv::Vec4i> hierarchy;

    cv::findContours(tempEdge, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // Filter contours by area
    std::vector<std::vector<cv::Point>> filteredContours;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area > contourMinArea) {
            filteredContours.push_back(contour);
        }
    }

    contours = filteredContours;
}

void ImageProcessor::createBrushStrokes() {
    if (originalImage.empty() || edgeImage.empty()) {
        return;
    }

    // Create black background for brush strokes
    brushStrokeImage = cv::Mat::zeros(originalImage.size(), originalImage.type());
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Draw brush strokes along contours
    for (const auto& contour : contours) {
        if (contour.size() < 2) continue;
        
        for (size_t i = 0; i < contour.size() - 1; i += brushDensity) {
            cv::Point pt1 = contour[i];
            cv::Point pt2 = contour[std::min(i + brushDensity, contour.size() - 1)];
            
            // Get color from original image at this point
            if (pt1.x >= 0 && pt1.x < originalImage.cols && 
                pt1.y >= 0 && pt1.y < originalImage.rows) {
                
                cv::Vec3b color = originalImage.at<cv::Vec3b>(pt1.y, pt1.x);
                
                // Add slight random variation to brush stroke
                std::uniform_int_distribution<> offsetDist(-2, 2);
                std::uniform_int_distribution<> sizeDist(-1, 2);
                
                int offset_x = offsetDist(gen);
                int offset_y = offsetDist(gen);
                int sizeVar = sizeDist(gen);
                
                cv::Point strokePt1(pt1.x + offset_x, pt1.y + offset_y);
                cv::Point strokePt2(pt2.x + offset_x, pt2.y + offset_y);
                
                // Draw the brush stroke
                int thickness = std::max(1, brushSize + sizeVar);
                cv::line(brushStrokeImage, strokePt1, strokePt2, 
                         cv::Scalar(color[0], color[1], color[2]), 
                         thickness, cv::LINE_AA);
                
                // Add small circles at endpoints for a more painterly effect
                cv::circle(brushStrokeImage, strokePt1, thickness / 2 + 1, 
                          cv::Scalar(color[0], color[1], color[2]), -1, cv::LINE_AA);
            }
        }
    }
    
    // Also add strokes based on edge image for more coverage
    std::uniform_int_distribution<> skipDist(0, brushDensity * 2);
    
    for (int y = 0; y < edgeImage.rows; y += brushDensity) {
        for (int x = 0; x < edgeImage.cols; x += brushDensity) {
            if (edgeImage.at<uchar>(y, x) > 128) {
                // This is an edge pixel
                cv::Vec3b color = originalImage.at<cv::Vec3b>(y, x);
                
                std::uniform_int_distribution<> angleDist(0, 360);
                std::uniform_int_distribution<> lenDist(brushSize, brushSize * 3);
                
                int angle = angleDist(gen);
                int len = lenDist(gen);
                
                double rad = angle * CV_PI / 180.0;
                cv::Point pt1(x, y);
                cv::Point pt2(x + static_cast<int>(len * cos(rad)), 
                              y + static_cast<int>(len * sin(rad)));
                
                cv::line(brushStrokeImage, pt1, pt2, 
                         cv::Scalar(color[0], color[1], color[2]), 
                         brushSize, cv::LINE_AA);
            }
        }
    }
}
