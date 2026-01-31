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
    const cv::Mat& getNeonImage() const { return neonImage; }
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
    void setBlurStrength(int val) { blurStrength = val; }
    void setBilateralFilter(bool val) { useBilateralFilter = val; }
    void setBilateralD(int val) { bilateralD = val; }
    void setBilateralSigmaColor(double val) { bilateralSigmaColor = val; }
    void setBilateralSigmaSpace(double val) { bilateralSigmaSpace = val; }
    void setMorphologySize(int val) { morphologySize = val; }
    void setMinContourLength(double val) { minContourLength = val; }
    void setEdgeDilation(int val) { edgeDilation = val; }
    void setEdgeSmoothing(int val) { edgeSmoothing = val; }
    void setContourSmoothing(double val) { contourSmoothing = val; }
    
    // Neon effect parameters
    void setNeonCenterColor(float r, float g, float b) { neonCenterColor = cv::Scalar(b*255, g*255, r*255); }
    void setNeonOtherColor(float r, float g, float b) { neonOtherColor = cv::Scalar(b*255, g*255, r*255); }
    void setNeonEdgeColor(float r, float g, float b) { neonEdgeColor = cv::Scalar(b*255, g*255, r*255); }
    void setNeonGlowStrength(int val) { neonGlowStrength = val; }
    void setNeonGlowSize(int val) { neonGlowSize = val; }
    
    cv::Scalar getNeonCenterColor() const { return neonCenterColor; }
    cv::Scalar getNeonOtherColor() const { return neonOtherColor; }
    cv::Scalar getNeonEdgeColor() const { return neonEdgeColor; }
    int getNeonGlowStrength() const { return neonGlowStrength; }
    int getNeonGlowSize() const { return neonGlowSize; }

    double getCannyThreshold1() const { return cannyThreshold1; }
    double getCannyThreshold2() const { return cannyThreshold2; }
    double getContourMinArea() const { return contourMinArea; }
    int getBrushSize() const { return brushSize; }
    int getBrushDensity() const { return brushDensity; }
    int getBlurStrength() const { return blurStrength; }
    bool getBilateralFilter() const { return useBilateralFilter; }
    int getBilateralD() const { return bilateralD; }
    double getBilateralSigmaColor() const { return bilateralSigmaColor; }
    double getBilateralSigmaSpace() const { return bilateralSigmaSpace; }
    int getMorphologySize() const { return morphologySize; }
    double getMinContourLength() const { return minContourLength; }
    int getEdgeDilation() const { return edgeDilation; }
    int getEdgeSmoothing() const { return edgeSmoothing; }
    double getContourSmoothing() const { return contourSmoothing; }

private:
    cv::Mat originalImage;
    cv::Mat processedImage;
    cv::Mat edgeImage;
    cv::Mat brushStrokeImage;
    cv::Mat neonImage;
    std::vector<std::vector<cv::Point>> contours;

    // Processing parameters
    double cannyThreshold1 = 50.0;
    double cannyThreshold2 = 150.0;
    double contourMinArea = 100.0;
    int brushSize = 4;
    int brushDensity = 8;
    
    // Noise reduction parameters
    int blurStrength = 5;              // Gaussian blur kernel size (must be odd)
    bool useBilateralFilter = false;   // Use bilateral filter instead of Gaussian
    int bilateralD = 9;                // Bilateral filter diameter
    double bilateralSigmaColor = 75.0; // Color sigma for bilateral
    double bilateralSigmaSpace = 75.0; // Space sigma for bilateral
    int morphologySize = 0;            // Morphological operation kernel size (0 = disabled)
    double minContourLength = 10.0;    // Minimum contour arc length
    int edgeDilation = 0;              // Dilate edges to connect fragments (0 = disabled)
    int edgeSmoothing = 0;             // Gaussian blur on edge image (0 = disabled)
    double contourSmoothing = 0.0;     // Contour approximation epsilon (0 = disabled)
    
    // Neon effect parameters
    cv::Scalar neonCenterColor = cv::Scalar(255, 0, 255);   // Magenta (BGR)
    cv::Scalar neonOtherColor = cv::Scalar(255, 255, 0);    // Cyan (BGR)
    cv::Scalar neonEdgeColor = cv::Scalar(0, 0, 255);       // Red (BGR)
    int neonGlowStrength = 3;   // Number of glow layers
    int neonGlowSize = 15;      // Blur size for glow

    void detectEdges();
    void findContours();
    void createBrushStrokes();
    void createNeonEffect();
};
