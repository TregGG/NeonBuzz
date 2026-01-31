#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor();

    // Load image from file
    bool loadImage(const std::string& filepath);

    // Process: detect edges and contours
    void processImage();

    // Get results
    const cv::Mat& getOriginalImage() const { return originalImage; }
    const cv::Mat& getProcessedImage() const { return processedImage; }
    const cv::Mat& getEdgeImage() const { return edgeImage; }
    const cv::Mat& getBrushStrokeImage() const { return brushStrokeImage; }
    const std::vector<std::vector<cv::Point>>& getContours() const { return contours; }

    // Image info
    int getWidth() const { return originalImage.cols; }
    int getHeight() const { return originalImage.rows; }
    bool hasImage() const { return !originalImage.empty(); }

    // Processing parameters
    void setCannyThreshold1(double val) { cannyThreshold1 = val; }
    void setCannyThreshold2(double val) { cannyThreshold2 = val; }
    void setContourMinArea(double val) { contourMinArea = val; }
    void setBrushSize(int val) { brushSize = val; }
    void setBrushDensity(int val) { brushDensity = val; }

    double getCannyThreshold1() const { return cannyThreshold1; }
    double getCannyThreshold2() const { return cannyThreshold2; }
    double getContourMinArea() const { return contourMinArea; }
    int getBrushSize() const { return brushSize; }
    int getBrushDensity() const { return brushDensity; }

private:
    cv::Mat originalImage;
    cv::Mat processedImage;
    cv::Mat edgeImage;
    cv::Mat brushStrokeImage;
    std::vector<std::vector<cv::Point>> contours;

    // Processing parameters
    double cannyThreshold1 = 50.0;
    double cannyThreshold2 = 150.0;
    double contourMinArea = 100.0;
    int brushSize = 4;
    int brushDensity = 8;

    void detectEdges();
    void findContours();
    void createBrushStrokes();
};
