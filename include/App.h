#pragma once

#include <memory>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class ImageProcessor;
class Renderer;

class App {
public:
    App(int width = 1280, int height = 720);
    ~App();

    void run();
    bool isRunning() const;
    void processFrame();
    void handleInput();

    // Image loading
    void loadImage(const std::string& filepath);

private:
    GLFWwindow* window;
    int windowWidth, windowHeight;
    bool running;

    std::unique_ptr<ImageProcessor> imageProcessor;
    std::unique_ptr<Renderer> renderer;

    void initOpenGL();
    void initImGui();
    void cleanup();
};
