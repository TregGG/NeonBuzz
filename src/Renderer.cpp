#include "Renderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

const char* vertexShaderSource = R"(
#version 410 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(position, 1.0);
    TexCoord = texCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 410 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;

void main() {
    FragColor = texture(texture1, TexCoord);
}
)";

Renderer::Renderer()
    : VAO(0), VBO(0), EBO(0), shaderProgram(0), textureID(0), 
      displayMode(ORIGINAL) {
}

Renderer::~Renderer() {
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    if (EBO != 0) glDeleteBuffers(1, &EBO);
    if (shaderProgram != 0) glDeleteProgram(shaderProgram);
    if (textureID != 0) glDeleteTextures(1, &textureID);
}

void Renderer::init() {
    createShaders();
    setupQuad();
}

void Renderer::createShaders() {
    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Check for vertex shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // Check for fragment shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    // Link shaders
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Renderer::setupQuad() {
    // Quad vertices with texture coordinates
    float vertices[] = {
        // positions          // texture coords
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

GLuint Renderer::loadTexture(const cv::Mat& image) {
    if (image.empty()) {
        return 0;
    }

    cv::Mat displayImage = image.clone();

    // Flip image vertically for OpenGL (OpenCV has origin at top-left, OpenGL at bottom-left)
    cv::flip(displayImage, displayImage, 0);

    // Ensure we have 3 or 4 channels
    if (displayImage.channels() == 1) {
        cv::cvtColor(displayImage, displayImage, cv::COLOR_GRAY2RGB);
    } else if (displayImage.channels() == 4) {
        cv::cvtColor(displayImage, displayImage, cv::COLOR_RGBA2RGB);
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, displayImage.cols, displayImage.rows,
                 0, GL_RGB, GL_UNSIGNED_BYTE, displayImage.data);

    return tex;
}

void Renderer::renderImage(const cv::Mat& image) {
    if (image.empty()) {
        return;
    }

    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }

    textureID = loadTexture(image);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Renderer::renderEdges(const cv::Mat& edgeImage) {
    if (edgeImage.empty()) {
        return;
    }

    // Convert grayscale edges to 3-channel for display
    cv::Mat displayImage;
    cv::cvtColor(edgeImage, displayImage, cv::COLOR_GRAY2RGB);

    renderImage(displayImage);
}

void Renderer::renderContours(const cv::Mat& image, const std::vector<std::vector<cv::Point>>& contours) {
    if (image.empty()) {
        return;
    }

    cv::Mat displayImage = image.clone();

    auto hsvToRgb = [](float hDeg, float s, float v) -> cv::Scalar {
        hDeg = std::fmod(hDeg, 360.0f);
        if (hDeg < 0.0f) hDeg += 360.0f;
        s = std::clamp(s, 0.0f, 1.0f);
        v = std::clamp(v, 0.0f, 1.0f);

        float c = v * s;
        float x = c * (1.0f - std::fabs(std::fmod(hDeg / 60.0f, 2.0f) - 1.0f));
        float m = v - c;

        float r1 = 0.0f, g1 = 0.0f, b1 = 0.0f;
        if (hDeg < 60.0f) {
            r1 = c;
            g1 = x;
            b1 = 0.0f;
        } else if (hDeg < 120.0f) {
            r1 = x;
            g1 = c;
            b1 = 0.0f;
        } else if (hDeg < 180.0f) {
            r1 = 0.0f;
            g1 = c;
            b1 = x;
        } else if (hDeg < 240.0f) {
            r1 = 0.0f;
            g1 = x;
            b1 = c;
        } else if (hDeg < 300.0f) {
            r1 = x;
            g1 = 0.0f;
            b1 = c;
        } else {
            r1 = c;
            g1 = 0.0f;
            b1 = x;
        }

        uint8_t r = static_cast<uint8_t>(std::round((r1 + m) * 255.0f));
        uint8_t g = static_cast<uint8_t>(std::round((g1 + m) * 255.0f));
        uint8_t b = static_cast<uint8_t>(std::round((b1 + m) * 255.0f));
        return cv::Scalar(r, g, b);
    };

    const int thickness = std::max(1, static_cast<int>(strokeWidth));
    for (size_t i = 0; i < contours.size(); ++i) {
        float hue = std::fmod(137.508f * static_cast<float>(i), 360.0f);
        cv::Scalar color = hsvToRgb(hue, 0.95f, 1.0f);
        cv::drawContours(displayImage, contours, static_cast<int>(i), color, thickness, cv::LINE_AA);
    }

    renderImage(displayImage);
}

void Renderer::setStrokeColor(float r, float g, float b, float a) {
    strokeColor[0] = r;
    strokeColor[1] = g;
    strokeColor[2] = b;
    strokeColor[3] = a;
}
