#include "App.h"
#include "ImageProcessor.h"
#include "Renderer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <tinyfiledialogs.h>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

App::App(int width, int height)
    : window(nullptr), windowWidth(width), windowHeight(height), running(true) {
    
    imageProcessor = std::make_unique<ImageProcessor>();
    renderer = std::make_unique<Renderer>();

    initOpenGL();
    initImGui();
}

App::~App() {
    cleanup();
}

void App::initOpenGL() {
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error: " << error << " - " << description << std::endl;
    });

    // Force X11 platform on Linux (needed for GLEW on Wayland systems)
#ifdef __linux__
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Image Contour Renderer", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        std::cerr << "GLEW Error: " << glewGetErrorString(glewStatus) << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // Clear any OpenGL errors that GLEW may have caused
    while (glGetError() != GL_NO_ERROR) {}

    // Store app pointer for callbacks
    glfwSetWindowUserPointer(window, this);

    glfwSetWindowCloseCallback(window, [](GLFWwindow* w) {
        App* app = static_cast<App*>(glfwGetWindowUserPointer(w));
        app->running = false;
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height) {
        App* app = static_cast<App*>(glfwGetWindowUserPointer(w));
        app->windowWidth = width;
        app->windowHeight = height;
        glViewport(0, 0, width, height);
    });

    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

    renderer->init();
}

void App::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    const char* glsl_version = "#version 410";
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void App::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void App::run() {
    while (running && !glfwWindowShouldClose(window)) {
        handleInput();
        processFrame();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

bool App::isRunning() const {
    return running && !glfwWindowShouldClose(window);
}

void App::processFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main control panel
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls", nullptr);

    ImGui::Text("Image Contour Renderer");
    ImGui::Separator();

    // File browser / load image
    static char filepath[1024] = "";
    ImGui::InputText("Image Path", filepath, sizeof(filepath));
    
    if (ImGui::Button("Browse...", ImVec2(100, 0))) {
        const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tiff", "*.webp" };
        char* selectedFile = tinyfd_openFileDialog(
            "Select Image",
            "",
            7,
            filterPatterns,
            "Image Files",
            0
        );
        if (selectedFile) {
            strncpy(filepath, selectedFile, sizeof(filepath) - 1);
            filepath[sizeof(filepath) - 1] = '\0';
            loadImage(filepath);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load", ImVec2(-1, 0))) {
        loadImage(filepath);
    }

    if (imageProcessor->hasImage()) {
        ImGui::Separator();
        ImGui::Text("Image: %dx%d", imageProcessor->getWidth(), imageProcessor->getHeight());

        // Display mode selection
        static int displayMode = 3; // BRUSH_STROKES
        const char* modes[] = {"Original", "Edges", "Contours", "Brush Strokes", "Combined"};
        if (ImGui::Combo("Display Mode", &displayMode, modes, 5)) {
            renderer->setDisplayMode(static_cast<Renderer::DisplayMode>(displayMode));
        }

        ImGui::Separator();
        ImGui::Text("Edge Detection Parameters");

        double threshold1 = imageProcessor->getCannyThreshold1();
        double threshold2 = imageProcessor->getCannyThreshold2();
        double minArea = imageProcessor->getContourMinArea();

        float t1 = static_cast<float>(threshold1);
        if (ImGui::SliderFloat("Canny T1", &t1, 10, 200)) {
            imageProcessor->setCannyThreshold1(t1);
            imageProcessor->processImage();
        }

        float t2 = static_cast<float>(threshold2);
        if (ImGui::SliderFloat("Canny T2", &t2, 50, 400)) {
            imageProcessor->setCannyThreshold2(t2);
            imageProcessor->processImage();
        }

        float ma = static_cast<float>(minArea);
        if (ImGui::SliderFloat("Min Contour Area", &ma, 1, 1000)) {
            imageProcessor->setContourMinArea(ma);
            imageProcessor->processImage();
        }

        ImGui::Separator();
        ImGui::Text("Brush Stroke Settings");

        int brushSize = imageProcessor->getBrushSize();
        if (ImGui::SliderInt("Brush Size", &brushSize, 1, 15)) {
            imageProcessor->setBrushSize(brushSize);
            imageProcessor->processImage();
        }

        int brushDensity = imageProcessor->getBrushDensity();
        if (ImGui::SliderInt("Brush Density", &brushDensity, 1, 20)) {
            imageProcessor->setBrushDensity(brushDensity);
            imageProcessor->processImage();
        }

        ImGui::Separator();
        ImGui::Text("Stroke Settings");

        static float strokeColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        if (ImGui::ColorEdit4("Stroke Color", strokeColor)) {
            renderer->setStrokeColor(strokeColor[0], strokeColor[1], strokeColor[2], strokeColor[3]);
        }

        static float strokeWidth = 2.0f;
        if (ImGui::SliderFloat("Stroke Width", &strokeWidth, 1.0f, 10.0f)) {
            renderer->setStrokeWidth(strokeWidth);
        }

        ImGui::Separator();
        ImGui::Text("Contours found: %zu", imageProcessor->getContours().size());
    }

    ImGui::End();

    // Render the image
    if (imageProcessor->hasImage()) {
        ImGui::SetNextWindowPos(ImVec2(320, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(950, 700), ImGuiCond_FirstUseEver);
        ImGui::Begin("Viewport");

        Renderer::DisplayMode mode = renderer->getDisplayMode();
        if (mode == Renderer::ORIGINAL) {
            renderer->renderImage(imageProcessor->getOriginalImage());
        } else if (mode == Renderer::EDGES) {
            renderer->renderEdges(imageProcessor->getEdgeImage());
        } else if (mode == Renderer::CONTOURS) {
            renderer->renderContours(imageProcessor->getOriginalImage(), imageProcessor->getContours());
        } else if (mode == Renderer::BRUSH_STROKES) {
            renderer->renderImage(imageProcessor->getBrushStrokeImage());
        } else { // COMBINED
            cv::Mat combined = imageProcessor->getBrushStrokeImage().clone();
            cv::drawContours(combined, imageProcessor->getContours(), -1, cv::Scalar(255, 255, 255), 1);
            renderer->renderImage(combined);
        }

        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)(intptr_t)renderer->getTextureID(), viewportSize, ImVec2(0, 1), ImVec2(1, 0));

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void App::handleInput() {
    // Handle keyboard input if needed
}

void App::loadImage(const std::string& filepath) {
    if (imageProcessor->loadImage(filepath)) {
        imageProcessor->processImage();
        std::cout << "Image loaded successfully: " << filepath << std::endl;
    } else {
        std::cerr << "Failed to load image: " << filepath << std::endl;
    }
}
