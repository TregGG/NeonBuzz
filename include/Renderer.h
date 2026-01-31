#pragma once

#include <string>
#include <vector>
#include <GL/glew.h>
#include <opencv2/opencv.hpp>

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Initialize renderer
    void init();

    // Render image with contours
    void renderImage(const cv::Mat& image);
    void renderEdges(const cv::Mat& edgeImage);
    void renderContours(const cv::Mat& image, const std::vector<std::vector<cv::Point>>& contours);

    // Display modes
    enum DisplayMode {
        ORIGINAL,
        EDGES,
        CONTOURS,
        BRUSH_STROKES,
        COMBINED,
        NEON
    };

    void setDisplayMode(DisplayMode mode) { displayMode = mode; }
    DisplayMode getDisplayMode() const { return displayMode; }

    // Stroke settings
    void setStrokeColor(float r, float g, float b, float a = 1.0f);
    void setStrokeWidth(float width) { strokeWidth = width; }

    GLuint getTextureID() const { return textureID; }

private:
    GLuint VAO, VBO, EBO;
    GLuint shaderProgram;
    GLuint textureID;
    DisplayMode displayMode;

    float strokeColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float strokeWidth = 2.0f;

    void createShaders();
    void setupQuad();
    GLuint loadTexture(const cv::Mat& image);
    void drawContourStroke(const std::vector<std::vector<cv::Point>>& contours);
};
