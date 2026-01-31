#include "ImageProcessor.h"
#include <opencv2/ximgproc.hpp>
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
    createNeonEffect();
}

void ImageProcessor::detectEdges() {
    cv::Mat gray;
    if (originalImage.channels() == 3) {
        cv::cvtColor(originalImage, gray, cv::COLOR_RGB2GRAY);
    } else {
        gray = originalImage.clone();
    }

    // Apply noise reduction
    cv::Mat blurred;
    if (useBilateralFilter) {
        // Bilateral filter - edge-preserving blur
        cv::bilateralFilter(gray, blurred, bilateralD, bilateralSigmaColor, bilateralSigmaSpace);
    } else {
        // Gaussian blur - ensure kernel size is odd and >= 1
        int kernelSize = std::max(1, blurStrength);
        if (kernelSize % 2 == 0) kernelSize++;
        cv::GaussianBlur(gray, blurred, cv::Size(kernelSize, kernelSize), 0);
    }

    // Canny edge detection
    cv::Canny(blurred, edgeImage, cannyThreshold1, cannyThreshold2);
    
    // Apply morphological operations to reduce noise
    if (morphologySize > 0) {
        int kernelSize = morphologySize;
        if (kernelSize % 2 == 0) kernelSize++;
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(kernelSize, kernelSize));
        
        // Close operation (dilate then erode) - fills small gaps
        cv::morphologyEx(edgeImage, edgeImage, cv::MORPH_CLOSE, kernel);
        
        // Open operation (erode then dilate) - removes small noise
        cv::morphologyEx(edgeImage, edgeImage, cv::MORPH_OPEN, kernel);
    }
    
    // Edge dilation to connect fragmented edges (like hair strands)
    if (edgeDilation > 0) {
        int kernelSize = edgeDilation;
        if (kernelSize % 2 == 0) kernelSize++;
        cv::Mat dilateKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(kernelSize, kernelSize));
        cv::dilate(edgeImage, edgeImage, dilateKernel);
        
        // Re-thin edges using skeletonization approximation
        cv::Mat thinned;
        cv::ximgproc::thinning(edgeImage, thinned, cv::ximgproc::THINNING_ZHANGSUEN);
        edgeImage = thinned;
    }
    
    // Edge smoothing - blur the edge image then re-threshold
    if (edgeSmoothing > 0) {
        int kernelSize = edgeSmoothing;
        if (kernelSize % 2 == 0) kernelSize++;
        cv::GaussianBlur(edgeImage, edgeImage, cv::Size(kernelSize, kernelSize), 0);
        cv::threshold(edgeImage, edgeImage, 30, 255, cv::THRESH_BINARY);
    }
}

void ImageProcessor::findContours() {
    if (edgeImage.empty()) {
        return;
    }

    contours.clear();
    cv::Mat tempEdge = edgeImage.clone();
    std::vector<cv::Vec4i> hierarchy;

    cv::findContours(tempEdge, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // Filter contours by area and arc length
    std::vector<std::vector<cv::Point>> filteredContours;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        double length = cv::arcLength(contour, false);
        
        // Filter out small contours by area AND length
        if (area > contourMinArea && length > minContourLength) {
            // Apply contour smoothing if enabled
            if (contourSmoothing > 0) {
                std::vector<cv::Point> smoothed;
                cv::approxPolyDP(contour, smoothed, contourSmoothing, false);
                if (smoothed.size() >= 2) {
                    filteredContours.push_back(smoothed);
                }
            } else {
                filteredContours.push_back(contour);
            }
        }
    }

    contours = filteredContours;
}

void ImageProcessor::createBrushStrokes() {
    if (originalImage.empty() || edgeImage.empty()) {
        return;
    }

    // Create black background for brush strokes
    brushStrokeImage = cv::Mat::zeros(originalImage.size(), CV_8UC3);
    
    // Compute gradient direction using Sobel
    cv::Mat gray;
    if (originalImage.channels() == 3) {
        cv::cvtColor(originalImage, gray, cv::COLOR_RGB2GRAY);
    } else {
        gray = originalImage.clone();
    }
    
    cv::Mat gradX, gradY;
    cv::Sobel(gray, gradX, CV_32F, 1, 0, 3);
    cv::Sobel(gray, gradY, CV_32F, 0, 1, 3);
    
    // Compute edge density map - how many edge pixels in local neighborhood
    cv::Mat edgeDensity;
    int densityKernelSize = 21;  // Size of neighborhood to check
    cv::blur(edgeImage, edgeDensity, cv::Size(densityKernelSize, densityKernelSize));
    
    // Normalize density to 0-1 range
    double minDensity, maxDensity;
    cv::minMaxLoc(edgeDensity, &minDensity, &maxDensity);
    if (maxDensity > minDensity) {
        edgeDensity.convertTo(edgeDensity, CV_32F, 1.0 / (maxDensity - minDensity), -minDensity / (maxDensity - minDensity));
    } else {
        edgeDensity.convertTo(edgeDensity, CV_32F, 0, 0.5);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> offsetDist(-1, 1);  // Reduced position jitter
    std::uniform_int_distribution<> sizeDist(-1, 1);
    std::uniform_int_distribution<> signDist(0, 1);
    std::uniform_real_distribution<> skipDist(0.0, 1.0);
    
    // Reduced angle offsets for tighter edge following
    const float maxAngleOffset = 5.0f * CV_PI / 180.0f;   // Max 5 degrees
    const float minAngleOffset = 0.5f * CV_PI / 180.0f;   // Min 0.5 degrees
    
    // Draw brush strokes along contours with sketchy effect
    for (const auto& contour : contours) {
        if (contour.size() < 2) continue;
        
        // Draw main stroke along contour
        for (size_t i = 0; i < contour.size() - 1; i++) {
            cv::Point pt1 = contour[i];
            cv::Point pt2 = contour[(i + 1) % contour.size()];
            
            // Get local density at this point
            float density = 0.5f;
            if (pt1.x >= 0 && pt1.x < edgeDensity.cols && pt1.y >= 0 && pt1.y < edgeDensity.rows) {
                density = edgeDensity.at<float>(pt1.y, pt1.x);
            }
            
            // Skip some strokes in high-density areas (up to 70%)
            float skipProbability = density * 0.7f;
            if (skipDist(gen) < skipProbability) {
                continue;
            }
            
            // Small angle offset for brush effect while following edge
            float angleRange = std::max(minAngleOffset, maxAngleOffset - (density * (maxAngleOffset - minAngleOffset)));
            std::uniform_real_distribution<> angleOffsetDist(0.0, angleRange);
            
            // Calculate tangent direction from contour points
            float tangentAngle = atan2(pt2.y - pt1.y, pt2.x - pt1.x);
            
            // Add small random angle offset
            float angleOffset = angleOffsetDist(gen) * (signDist(gen) ? 1 : -1);
            float strokeAngle = tangentAngle + angleOffset;
            
            // Calculate stroke length - slightly longer for smoother look
            float strokeLen = sqrt(pow(pt2.x - pt1.x, 2) + pow(pt2.y - pt1.y, 2)) * 1.1f;
            
            // Minimal position variation
            int offset_x = offsetDist(gen);
            int offset_y = offsetDist(gen);
            
            cv::Point strokePt1(pt1.x + offset_x, pt1.y + offset_y);
            cv::Point strokePt2(
                pt1.x + offset_x + static_cast<int>(strokeLen * cos(strokeAngle)),
                pt1.y + offset_y + static_cast<int>(strokeLen * sin(strokeAngle))
            );
            
            // More consistent brightness
            int baseGray = 220 + static_cast<int>((1.0f - density) * 35);  // 220-255 range
            std::uniform_int_distribution<> grayDist(std::max(200, baseGray - 15), baseGray);
            int grayVal = grayDist(gen);
            int thickness = std::max(1, brushSize + sizeDist(gen));
            
            // Draw the main stroke
            cv::line(brushStrokeImage, strokePt1, strokePt2, 
                     cv::Scalar(grayVal, grayVal, grayVal), 
                     thickness, cv::LINE_AA);
        }
        
        // Add secondary "sketch" lines with slight offset for texture
        if (brushDensity < 15) {
            for (size_t i = 0; i < contour.size() - 1; i += 3) {
                cv::Point pt1 = contour[i];
                cv::Point pt2 = contour[std::min(i + 3, contour.size() - 1)];
                
                // Get local density
                float density = 0.5f;
                if (pt1.x >= 0 && pt1.x < edgeDensity.cols && pt1.y >= 0 && pt1.y < edgeDensity.rows) {
                    density = edgeDensity.at<float>(pt1.y, pt1.x);
                }
                
                // Skip in dense areas
                float skipProbability = density * 0.8f;
                if (skipDist(gen) < skipProbability) {
                    continue;
                }
                
                float angleRange = std::max(minAngleOffset, maxAngleOffset - (density * (maxAngleOffset - minAngleOffset)));
                std::uniform_real_distribution<> angleOffsetDist(0.0, angleRange);
                
                float tangentAngle = atan2(pt2.y - pt1.y, pt2.x - pt1.x);
                float angleOffset = angleOffsetDist(gen) * (signDist(gen) ? 1 : -1);
                float strokeAngle = tangentAngle + angleOffset;
                float strokeLen = sqrt(pow(pt2.x - pt1.x, 2) + pow(pt2.y - pt1.y, 2));
                
                int offset = offsetDist(gen);
                int baseGray = 200 + static_cast<int>((1.0f - density) * 40);
                std::uniform_int_distribution<> grayDist2(std::max(180, baseGray - 15), baseGray);
                int grayVal = grayDist2(gen);
                
                cv::Point strokePt1(pt1.x + offset, pt1.y + offset);
                cv::Point strokePt2(
                    pt1.x + offset + static_cast<int>(strokeLen * cos(strokeAngle)),
                    pt1.y + offset + static_cast<int>(strokeLen * sin(strokeAngle))
                );
                
                cv::line(brushStrokeImage, strokePt1, strokePt2, 
                         cv::Scalar(grayVal, grayVal, grayVal), 
                         std::max(1, brushSize - 1), cv::LINE_AA);
            }
        }
    }
    
    // Add brush strokes along edge pixels for finer detail
    for (int y = 1; y < edgeImage.rows - 1; y++) {
        for (int x = 1; x < edgeImage.cols - 1; x++) {
            if (edgeImage.at<uchar>(y, x) > 128) {
                // Get local density at this point
                float density = edgeDensity.at<float>(y, x);
                
                // Skip more in dense areas
                float skipProbability = density * 0.75f;
                if (skipDist(gen) < skipProbability) {
                    continue;
                }
                
                // Tight angle offset for edge following
                float angleRange = std::max(minAngleOffset, maxAngleOffset - (density * (maxAngleOffset - minAngleOffset)));
                std::uniform_real_distribution<> angleOffsetDist(0.0, angleRange);
                
                // Get gradient direction (perpendicular to edge)
                float gx = gradX.at<float>(y, x);
                float gy = gradY.at<float>(y, x);
                
                // Gradient angle is perpendicular to edge, so add 90 degrees to get tangent
                float gradientAngle = atan2(gy, gx);
                float tangentAngle = gradientAngle + CV_PI / 2.0;  // Rotate 90 degrees to get edge direction
                
                // Add small random angle offset
                float angleOffset = angleOffsetDist(gen) * (signDist(gen) ? 1 : -1);
                float strokeAngle = tangentAngle + angleOffset;
                
                int strokeLen = brushSize * 2;
                int dx = static_cast<int>(strokeLen * cos(strokeAngle));
                int dy = static_cast<int>(strokeLen * sin(strokeAngle));
                
                // Consistent brightness
                int baseGray = 210 + static_cast<int>((1.0f - density) * 45);
                std::uniform_int_distribution<> grayDist3(std::max(190, baseGray - 15), baseGray);
                int grayVal = grayDist3(gen);
                int offset = offsetDist(gen);
                
                cv::Point pt1(x + offset, y + offset);
                cv::Point pt2(x + dx + offset, y + dy + offset);
                
                cv::line(brushStrokeImage, pt1, pt2, 
                         cv::Scalar(grayVal, grayVal, grayVal), 
                         std::max(1, brushSize / 2), cv::LINE_AA);
            }
        }
    }
}

void ImageProcessor::createNeonEffect() {
    if (originalImage.empty() || edgeImage.empty()) {
        return;
    }

    // Create black background
    neonImage = cv::Mat::zeros(originalImage.size(), CV_8UC3);
    
    // Vibrant neon color palette (BGR format)
    std::vector<cv::Scalar> neonPalette = {
        cv::Scalar(255, 0, 255),    // Magenta
        cv::Scalar(255, 255, 0),    // Cyan
        cv::Scalar(0, 255, 0),      // Green
        cv::Scalar(0, 165, 255),    // Orange
        cv::Scalar(255, 0, 0),      // Blue
        cv::Scalar(0, 255, 255),    // Yellow
        cv::Scalar(255, 0, 127),    // Pink/Purple
        cv::Scalar(0, 255, 127),    // Spring Green
        cv::Scalar(255, 127, 0),    // Deep Sky Blue
        cv::Scalar(127, 0, 255),    // Rose
        cv::Scalar(0, 127, 255),    // Dark Orange
        cv::Scalar(255, 255, 127),  // Light Cyan
    };
    
    // Create a mask with all contours drawn thick, then find connected components
    // This groups nearby contours into the same "object"
    cv::Mat objectMask = cv::Mat::zeros(originalImage.size(), CV_8UC1);
    for (size_t i = 0; i < contours.size(); i++) {
        cv::drawContours(objectMask, contours, static_cast<int>(i), cv::Scalar(255), 8);
    }
    
    // Dilate to connect nearby contours into single objects
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(25, 25));
    cv::dilate(objectMask, objectMask, kernel);
    
    // Find connected components (each component is an "object")
    cv::Mat labels, stats, centroids;
    int numLabels = cv::connectedComponentsWithStats(objectMask, labels, stats, centroids);
    
    // Assign each contour to an object based on which component it overlaps with
    std::vector<int> contourToObject(contours.size(), 0);
    for (size_t i = 0; i < contours.size(); i++) {
        // Sample a point from the contour to determine which object it belongs to
        if (contours[i].size() > 0) {
            cv::Point pt = contours[i][0];
            if (pt.x >= 0 && pt.x < labels.cols && pt.y >= 0 && pt.y < labels.rows) {
                contourToObject[i] = labels.at<int>(pt.y, pt.x);
            }
        }
    }
    
    // Calculate object sizes (sum of contour areas per object)
    std::vector<double> objectSizes(numLabels, 0);
    for (size_t i = 0; i < contours.size(); i++) {
        int objId = contourToObject[i];
        objectSizes[objId] += cv::contourArea(contours[i]);
    }
    
    // Sort objects by size and assign colors (largest objects get first colors)
    std::vector<std::pair<double, int>> objectOrder;
    for (int i = 1; i < numLabels; i++) {  // Skip label 0 (background)
        objectOrder.push_back({objectSizes[i], i});
    }
    std::sort(objectOrder.begin(), objectOrder.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Map object IDs to colors
    std::vector<cv::Scalar> objectColors(numLabels, neonEdgeColor);
    for (size_t i = 0; i < objectOrder.size(); i++) {
        int objId = objectOrder[i].second;
        objectColors[objId] = neonPalette[i % neonPalette.size()];
    }
    
    // Create a mask of all contour areas to exclude from edge drawing
    cv::Mat contourMask = cv::Mat::zeros(originalImage.size(), CV_8UC1);
    cv::drawContours(contourMask, contours, -1, cv::Scalar(255), 3);
    cv::dilate(contourMask, contourMask, cv::Mat(), cv::Point(-1,-1), 2);
    
    // Draw background edges with the edge color
    cv::Mat edgeLayer = cv::Mat::zeros(originalImage.size(), CV_8UC3);
    for (int y = 0; y < edgeImage.rows; y++) {
        for (int x = 0; x < edgeImage.cols; x++) {
            if (edgeImage.at<uchar>(y, x) > 128 && contourMask.at<uchar>(y, x) == 0) {
                edgeLayer.at<cv::Vec3b>(y, x) = cv::Vec3b(
                    static_cast<uchar>(neonEdgeColor[0]),
                    static_cast<uchar>(neonEdgeColor[1]),
                    static_cast<uchar>(neonEdgeColor[2])
                );
            }
        }
    }
    
    // Create layer for all contours with object-based colors
    cv::Mat contourLayer = cv::Mat::zeros(originalImage.size(), CV_8UC3);
    
    // Draw each contour with its object's color
    for (size_t i = 0; i < contours.size(); i++) {
        int objId = contourToObject[i];
        cv::Scalar color = objectColors[objId];
        
        // Find if this is a large object (top 5)
        bool isLargeObject = false;
        for (size_t j = 0; j < std::min(size_t(5), objectOrder.size()); j++) {
            if (objectOrder[j].second == objId) {
                isLargeObject = true;
                break;
            }
        }
        
        int thickness = isLargeObject ? 3 : 2;
        cv::drawContours(contourLayer, contours, static_cast<int>(i), 
                        color, thickness, cv::LINE_AA);
    }
    
    // Create glow effect
    cv::Mat glowEdge, glowContour;
    
    // Multiple glow passes for stronger effect
    for (int pass = 0; pass < neonGlowStrength; pass++) {
        int blurSize = neonGlowSize + pass * 10;
        if (blurSize % 2 == 0) blurSize++;
        
        cv::Mat tempGlowEdge, tempGlowContour;
        
        cv::GaussianBlur(edgeLayer, tempGlowEdge, cv::Size(blurSize, blurSize), 0);
        cv::GaussianBlur(contourLayer, tempGlowContour, cv::Size(blurSize, blurSize), 0);
        
        if (pass == 0) {
            glowEdge = tempGlowEdge;
            glowContour = tempGlowContour;
        } else {
            cv::addWeighted(glowEdge, 1.0, tempGlowEdge, 0.5, 0, glowEdge);
            cv::addWeighted(glowContour, 1.0, tempGlowContour, 0.5, 0, glowContour);
        }
    }
    
    // Combine layers: glow first, then sharp on top
    // Edge glow (dimmer, in background)
    cv::addWeighted(neonImage, 1.0, glowEdge, 0.6, 0, neonImage);
    
    // Contour glow (brighter)
    cv::addWeighted(neonImage, 1.0, glowContour, 1.2, 0, neonImage);
    
    // Sharp edge layer (dim)
    cv::Mat edgeLayerDim;
    cv::addWeighted(edgeLayer, 0.5, cv::Mat::zeros(edgeLayer.size(), edgeLayer.type()), 0, 0, edgeLayerDim);
    cv::add(neonImage, edgeLayerDim, neonImage);
    
    // Sharp contour layer
    cv::add(neonImage, contourLayer, neonImage);
    
    // Add white core to the largest objects for intense neon effect
    cv::Mat whiteCore = cv::Mat::zeros(originalImage.size(), CV_8UC3);
    for (size_t i = 0; i < contours.size(); i++) {
        int objId = contourToObject[i];
        // Check if this contour belongs to a top-5 object
        for (size_t j = 0; j < std::min(size_t(5), objectOrder.size()); j++) {
            if (objectOrder[j].second == objId) {
                cv::drawContours(whiteCore, contours, static_cast<int>(i), 
                                cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
                break;
            }
        }
    }
    cv::addWeighted(neonImage, 1.0, whiteCore, 0.5, 0, neonImage);
    
    // Convert BGR to RGB for OpenGL
    cv::cvtColor(neonImage, neonImage, cv::COLOR_BGR2RGB);
}

