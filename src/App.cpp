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

    window = glfwCreateWindow(windowWidth, windowHeight, "NeonBuzz", nullptr, nullptr);
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

    ImGui::Text("NeonBuzz");
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
        const char* modes[] = {"Original", "Edges", "Contours", "Brush Strokes", "Combined", "Neon"};
        if (ImGui::Combo("Display Mode", &displayMode, modes, 6)) {
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

        float mcl = static_cast<float>(imageProcessor->getMinContourLength());
        if (ImGui::SliderFloat("Min Contour Length", &mcl, 1, 200)) {
            imageProcessor->setMinContourLength(mcl);
            imageProcessor->processImage();
        }

        ImGui::Separator();
        ImGui::Text("Noise Reduction");

        int blurStrength = imageProcessor->getBlurStrength();
        if (ImGui::SliderInt("Blur Strength", &blurStrength, 1, 21)) {
            imageProcessor->setBlurStrength(blurStrength);
            imageProcessor->processImage();
        }

        bool useBilateral = imageProcessor->getBilateralFilter();
        if (ImGui::Checkbox("Bilateral Filter", &useBilateral)) {
            imageProcessor->setBilateralFilter(useBilateral);
            imageProcessor->processImage();
        }

        if (useBilateral) {
            int bilateralD = imageProcessor->getBilateralD();
            if (ImGui::SliderInt("Bilateral Diameter", &bilateralD, 3, 21)) {
                imageProcessor->setBilateralD(bilateralD);
                imageProcessor->processImage();
            }

            float sigmaColor = static_cast<float>(imageProcessor->getBilateralSigmaColor());
            if (ImGui::SliderFloat("Sigma Color", &sigmaColor, 10, 200)) {
                imageProcessor->setBilateralSigmaColor(sigmaColor);
                imageProcessor->processImage();
            }

            float sigmaSpace = static_cast<float>(imageProcessor->getBilateralSigmaSpace());
            if (ImGui::SliderFloat("Sigma Space", &sigmaSpace, 10, 200)) {
                imageProcessor->setBilateralSigmaSpace(sigmaSpace);
                imageProcessor->processImage();
            }
        }

        int morphSize = imageProcessor->getMorphologySize();
        if (ImGui::SliderInt("Morphology Size", &morphSize, 0, 7)) {
            imageProcessor->setMorphologySize(morphSize);
            imageProcessor->processImage();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Cleanup small noise with morphological ops (0=off)");
        }

        ImGui::Separator();
        ImGui::Text("Edge Smoothing");

        int edgeDilation = imageProcessor->getEdgeDilation();
        if (ImGui::SliderInt("Edge Dilation", &edgeDilation, 0, 7)) {
            imageProcessor->setEdgeDilation(edgeDilation);
            imageProcessor->processImage();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Connect fragmented edges (hair, fine details)");
        }

        int edgeSmoothing = imageProcessor->getEdgeSmoothing();
        if (ImGui::SliderInt("Edge Blur", &edgeSmoothing, 0, 11)) {
            imageProcessor->setEdgeSmoothing(edgeSmoothing);
            imageProcessor->processImage();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Smooth jagged edges (0=off)");
        }

        float contourSmooth = static_cast<float>(imageProcessor->getContourSmoothing());
        if (ImGui::SliderFloat("Contour Smoothing", &contourSmooth, 0.0f, 10.0f)) {
            imageProcessor->setContourSmoothing(contourSmooth);
            imageProcessor->processImage();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Simplify contour curves (0=off)");
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
        ImGui::Text("Neon Effect Settings");

        bool perContour = imageProcessor->getNeonPerContour();
        if (ImGui::Checkbox("Per-Contour Colors", &perContour)) {
            imageProcessor->setNeonPerContour(perContour);
            imageProcessor->processImage();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When enabled, every contour gets a unique color (no object grouping)");
        }

        if (perContour) {
            bool kmeansEnabled = imageProcessor->getNeonKMeansEnabled();
            if (ImGui::Checkbox("Group Nearby (K-Means)", &kmeansEnabled)) {
                imageProcessor->setNeonKMeansEnabled(kmeansEnabled);
                imageProcessor->processImage();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Run k-means on contour centroids, but only keep groups when they are very close");
            }

            if (kmeansEnabled) {
                int k = imageProcessor->getNeonKMeansK();
                if (ImGui::SliderInt("K-Means K", &k, 1, 128)) {
                    imageProcessor->setNeonKMeansK(k);
                    imageProcessor->processImage();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Higher K reduces forced merging of far contours");
                }

                float nearPx = imageProcessor->getNeonKMeansNearDistancePx();
                if (ImGui::SliderFloat("Near Distance (px)", &nearPx, 1.0f, 200.0f, "%.1f")) {
                    imageProcessor->setNeonKMeansNearDistancePx(nearPx);
                    imageProcessor->processImage();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Only contours within this distance of their cluster center stay grouped");
                }
            }
        }

        static float neonEdgeColor[3] = {1.0f, 0.0f, 0.0f};    // Red
        if (ImGui::ColorEdit3("Background Edges", neonEdgeColor)) {
            imageProcessor->setNeonEdgeColor(neonEdgeColor[0], neonEdgeColor[1], neonEdgeColor[2]);
            imageProcessor->processImage();
        }

        int glowStrength = imageProcessor->getNeonGlowStrength();
        if (ImGui::SliderInt("Glow Layers", &glowStrength, 1, 5)) {
            imageProcessor->setNeonGlowStrength(glowStrength);
            imageProcessor->processImage();
        }

        int glowSize = imageProcessor->getNeonGlowSize();
        if (ImGui::SliderInt("Glow Size", &glowSize, 1, 31)) {
            imageProcessor->setNeonGlowSize(glowSize);
            imageProcessor->processImage();
        }

        if (!perContour) {
            int maxObjects = imageProcessor->getNeonMaxObjects();
            if (ImGui::SliderInt("Main Objects", &maxObjects, 1, 12)) {
                imageProcessor->setNeonMaxObjects(maxObjects);
                imageProcessor->processImage();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Color only the largest N objects; everything else becomes background");
            }

            float minAreaRatio = imageProcessor->getNeonMinObjectAreaRatio();
            if (ImGui::SliderFloat("Min Object Area", &minAreaRatio, 0.001f, 0.10f, "%.3f")) {
                imageProcessor->setNeonMinObjectAreaRatio(minAreaRatio);
                imageProcessor->processImage();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Minimum object size as a fraction of the image (higher removes small clutter)");
            }

            int joinSize = imageProcessor->getNeonJoinSize();
            if (ImGui::SliderInt("Object Join", &joinSize, 3, 51)) {
                imageProcessor->setNeonJoinSize(joinSize);
                imageProcessor->processImage();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Connect nearby edges into one object (too high can merge everything)");
            }
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
        } else if (mode == Renderer::NEON) {
            renderer->renderImage(imageProcessor->getNeonImage());
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
