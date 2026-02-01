# NeonBuzz - Complete Technical Documentation

## Table of Contents

1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [Core Components](#core-components)
   - [Main Entry Point](#main-entry-point)
   - [App Class](#app-class)
   - [ImageProcessor Class](#imageprocessor-class)
   - [Renderer Class](#renderer-class)
4. [Image Processing Pipeline](#image-processing-pipeline)
   - [Image Loading](#image-loading)
   - [Edge Detection](#edge-detection)
   - [Noise Reduction](#noise-reduction)
   - [Edge Smoothing](#edge-smoothing)
   - [Contour Detection](#contour-detection)
   - [Brush Stroke Generation](#brush-stroke-generation)
   - [Neon Effect Generation](#neon-effect-generation)
5. [Rendering System](#rendering-system)
   - [OpenGL Setup](#opengl-setup)
   - [Shader System](#shader-system)
   - [Texture Management](#texture-management)
   - [Display Modes](#display-modes)
6. [User Interface](#user-interface)
   - [ImGui Integration](#imgui-integration)
   - [Control Panel](#control-panel)
   - [File Browser](#file-browser)
7. [Build System](#build-system)
8. [Dependencies](#dependencies)
9. [Data Flow Diagram](#data-flow-diagram)

---

## Project Overview

**NeonBuzz** is a real-time image processing application that transforms photographs into artistic contour, brush stroke, and neon glow renderings. It combines computer vision algorithms with GPU-accelerated rendering to provide an interactive tool for edge detection, contour extraction, stylized brush stroke generation, and vibrant neon effects.

### Key Features

- **Real-time edge detection** using Canny algorithm
- **Contour extraction and filtering** with adjustable parameters
- **Artistic brush stroke rendering** with density-adaptive edge following
- **Neon glow effect** with per-contour or object-based coloring
- **Multiple noise reduction techniques** (Gaussian blur, bilateral filter, morphological operations)
- **Edge smoothing algorithms** (dilation, thinning, blur)
- **6 display modes**: Original, Edges, Contours, Brush Strokes, Combined, Neon
- **Save functionality** with native file browser
- **Interactive parameter adjustment** via ImGui interface
- **Cross-platform support** (Linux, Windows, macOS)

### Brush Stroke Algorithm Highlights

The brush stroke rendering uses a sophisticated density-adaptive approach:
- **Tight edge following**: Strokes deviate only 0.5°-5° from edge tangent direction
- **Density-aware skipping**: Areas with many edges (eyes, hair) automatically skip strokes to prevent over-saturation
- **Uniform brightness**: Stroke brightness adjusts based on local density for consistent appearance
- **Multi-layer rendering**: Main contour strokes + secondary texture strokes + edge pixel strokes

### Neon Effect Highlights

The neon glow rendering provides vibrant, colorful edge visualization:
- **Two coloring modes**: Per-contour rainbow colors or object-based grouping
- **K-Means clustering**: Optional spatial clustering to group nearby contours with same color
- **Near distance filtering**: Prevents distant contours from being incorrectly grouped
- **Multi-layer glow**: Configurable glow strength and blur radius for authentic neon look
- **Object detection**: Morphological operations to identify main objects in the scene

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                           main.cpp                               │
│                    (Application Entry Point)                     │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                           App Class                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │   GLFW Window   │  │     ImGui UI    │  │  Event Loop     │  │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘  │
└───────────┼────────────────────┼────────────────────┼───────────┘
            │                    │                    │
            ▼                    ▼                    ▼
┌───────────────────────┐  ┌───────────────────────────────────────┐
│   ImageProcessor      │  │              Renderer                  │
│  ┌─────────────────┐  │  │  ┌─────────────┐  ┌─────────────────┐ │
│  │  Edge Detection │  │  │  │   Shaders   │  │  Texture Mgmt   │ │
│  │  Contour Find   │  │  │  │   VAO/VBO   │  │  Display Modes  │ │
│  │  Brush Strokes  │  │  │  └─────────────┘  └─────────────────┘ │
│  └─────────────────┘  │  └───────────────────────────────────────┘
└───────────────────────┘
```

### Design Patterns Used

1. **Composition**: `App` owns `ImageProcessor` and `Renderer` as unique pointers
2. **Single Responsibility**: Each class has a focused purpose
3. **Callback Pattern**: GLFW uses callbacks for window events

---

## Core Components

### Main Entry Point

**File**: `src/main.cpp`

```cpp
int main(int argc, char* argv[]) {
    try {
        App app(1280, 720);

        // Load a sample image if provided as argument
        if (argc > 1) {
            app.loadImage(argv[1]);
        }

        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

**Functionality**:
- Creates the main `App` instance with default window size 1280x720
- Optionally loads an image from command-line argument
- Enters the main application loop
- Exception handling for graceful error reporting

---

### App Class

**Files**: `include/App.h`, `src/App.cpp`

The `App` class is the main application controller that manages the window, event loop, and coordinates between `ImageProcessor` and `Renderer`.

#### Class Structure

```cpp
class App {
public:
    App(int width = 1280, int height = 720);
    ~App();

    void run();
    bool isRunning() const;
    void processFrame();
    void handleInput();
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
```

#### Constructor Flow

1. **Create ImageProcessor and Renderer instances**
   ```cpp
   imageProcessor = std::make_unique<ImageProcessor>();
   renderer = std::make_unique<Renderer>();
   ```

2. **Initialize OpenGL** (`initOpenGL()`)
   - Set up GLFW error callback
   - Force X11 platform on Linux (for GLEW compatibility with Wayland)
   - Initialize GLFW
   - Create OpenGL 4.1 Core Profile context
   - Create window with title "NeonBuzz"
   - Initialize GLEW for OpenGL extension loading
   - Set up window callbacks (close, resize)
   - Initialize viewport and clear color

3. **Initialize ImGui** (`initImGui()`)
   - Create ImGui context
   - Enable keyboard navigation
   - Apply dark theme
   - Initialize GLFW and OpenGL3 backends

#### Main Loop (`run()`)

```cpp
void App::run() {
    while (running && !glfwWindowShouldClose(window)) {
        handleInput();
        processFrame();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
```

Each frame:
1. Handle input events
2. Process and render the frame
3. Swap double buffers
4. Poll for window events

#### Frame Processing (`processFrame()`)

1. Clear OpenGL buffers (color and depth)
2. Start new ImGui frame
3. Render control panel window with all parameters
4. Render viewport window with current display mode
5. Finalize and render ImGui draw data

#### Image Loading

```cpp
void App::loadImage(const std::string& filepath) {
    if (imageProcessor->loadImage(filepath)) {
        imageProcessor->processImage();
        std::cout << "Image loaded successfully: " << filepath << std::endl;
    }
}
```

When an image is loaded:
1. `ImageProcessor` loads and scales the image
2. Full processing pipeline runs (edges → contours → brush strokes)
3. Results are ready for rendering

---

### ImageProcessor Class

**Files**: `include/ImageProcessor.h`, `src/ImageProcessor.cpp`

The `ImageProcessor` class handles all image processing operations using OpenCV.

#### Class Structure

```cpp
class ImageProcessor {
public:
    // Core functions
    bool loadImage(const std::string& filepath);
    void processImage();

    // Getters for processed data
    const cv::Mat& getOriginalImage() const;
    const cv::Mat& getEdgeImage() const;
    const cv::Mat& getBrushStrokeImage() const;
    const std::vector<std::vector<cv::Point>>& getContours() const;

    // Parameter setters/getters
    void setCannyThreshold1(double val);
    void setCannyThreshold2(double val);
    void setContourMinArea(double val);
    void setMinContourLength(double val);
    void setBrushSize(int val);
    void setBrushDensity(int val);
    void setBlurStrength(int val);
    void setBilateralFilter(bool val);
    void setBilateralD(int val);
    void setBilateralSigmaColor(double val);
    void setBilateralSigmaSpace(double val);
    void setMorphologySize(int val);
    void setEdgeDilation(int val);
    void setEdgeSmoothing(int val);
    void setContourSmoothing(double val);
    // ... corresponding getters

private:
    cv::Mat originalImage;
    cv::Mat processedImage;
    cv::Mat edgeImage;
    cv::Mat brushStrokeImage;
    std::vector<std::vector<cv::Point>> contours;

    // Processing parameters with defaults
    double cannyThreshold1 = 50.0;
    double cannyThreshold2 = 150.0;
    double contourMinArea = 100.0;
    double minContourLength = 10.0;
    int brushSize = 4;
    int brushDensity = 8;
    int blurStrength = 5;
    bool useBilateralFilter = false;
    int bilateralD = 9;
    double bilateralSigmaColor = 75.0;
    double bilateralSigmaSpace = 75.0;
    int morphologySize = 0;
    int edgeDilation = 0;
    int edgeSmoothing = 0;
    double contourSmoothing = 0.0;

    void detectEdges();
    void findContours();
    void createBrushStrokes();
};
```

#### Processing Parameters Explained

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `cannyThreshold1` | 50.0 | 10-200 | Lower threshold for Canny edge detection |
| `cannyThreshold2` | 150.0 | 50-400 | Upper threshold for Canny edge detection |
| `contourMinArea` | 100.0 | 1-1000 | Minimum contour area in pixels² |
| `minContourLength` | 10.0 | 1-200 | Minimum contour arc length in pixels |
| `brushSize` | 4 | 1-15 | Base brush stroke thickness |
| `brushDensity` | 8 | 1-20 | Controls secondary stroke frequency |
| `blurStrength` | 5 | 1-21 | Gaussian blur kernel size (odd) |
| `useBilateralFilter` | false | - | Toggle edge-preserving blur |
| `bilateralD` | 9 | 3-21 | Bilateral filter diameter |
| `bilateralSigmaColor` | 75.0 | 10-200 | Color space sigma for bilateral |
| `bilateralSigmaSpace` | 75.0 | 10-200 | Coordinate space sigma for bilateral |
| `morphologySize` | 0 | 0-7 | Morphological cleanup kernel size |
| `edgeDilation` | 0 | 0-7 | Edge dilation for connecting fragments |
| `edgeSmoothing` | 0 | 0-11 | Gaussian blur on edge image |
| `contourSmoothing` | 0.0 | 0-10 | Polygon approximation epsilon |

---

### Renderer Class

**Files**: `include/Renderer.h`, `src/Renderer.cpp`

The `Renderer` class handles all OpenGL rendering operations.

#### Class Structure

```cpp
class Renderer {
public:
    enum DisplayMode {
        ORIGINAL,
        EDGES,
        CONTOURS,
        BRUSH_STROKES,
        COMBINED
    };

    void init();
    void renderImage(const cv::Mat& image);
    void renderEdges(const cv::Mat& edgeImage);
    void renderContours(const cv::Mat& image, 
                        const std::vector<std::vector<cv::Point>>& contours);

    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const;
    void setStrokeColor(float r, float g, float b, float a = 1.0f);
    void setStrokeWidth(float width);

    GLuint getTextureID() const;

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
};
```

---

## Image Processing Pipeline

The processing pipeline consists of four main stages:

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│  loadImage() │ -> │ detectEdges()│ -> │findContours()│ -> │createBrush   │
│              │    │              │    │              │    │  Strokes()   │
└──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘
```

### Image Loading

**Function**: `ImageProcessor::loadImage()`

```cpp
bool ImageProcessor::loadImage(const std::string& filepath) {
    originalImage = cv::imread(filepath);
    if (originalImage.empty()) {
        std::cerr << "Failed to load image: " << filepath << std::endl;
        return false;
    }

    // Convert BGR to RGB (OpenCV loads as BGR, OpenGL expects RGB)
    if (originalImage.channels() == 3) {
        cv::cvtColor(originalImage, originalImage, cv::COLOR_BGR2RGB);
    }

    // Limit image size for performance
    const int maxDim = 1024;
    if (originalImage.cols > maxDim || originalImage.rows > maxDim) {
        float scale = static_cast<float>(maxDim) / 
                      std::max(originalImage.cols, originalImage.rows);
        cv::resize(originalImage, originalImage, cv::Size(), scale, scale, 
                   cv::INTER_AREA);
    }

    processedImage = originalImage.clone();
    return true;
}
```

**Steps**:
1. Load image using OpenCV's `imread()`
2. Convert color space from BGR to RGB (OpenCV uses BGR internally)
3. Scale down images larger than 1024px on any dimension for performance
4. Clone to `processedImage` for modifications

### Edge Detection

**Function**: `ImageProcessor::detectEdges()`

This function performs the complete edge detection pipeline including noise reduction and edge smoothing.

#### Step 1: Convert to Grayscale

```cpp
cv::Mat gray;
if (originalImage.channels() == 3) {
    cv::cvtColor(originalImage, gray, cv::COLOR_RGB2GRAY);
} else {
    gray = originalImage.clone();
}
```

#### Step 2: Apply Noise Reduction

**Option A: Gaussian Blur** (default)
```cpp
int kernelSize = std::max(1, blurStrength);
if (kernelSize % 2 == 0) kernelSize++;  // Must be odd
cv::GaussianBlur(gray, blurred, cv::Size(kernelSize, kernelSize), 0);
```

Gaussian blur is a simple but effective noise reduction that smooths the image uniformly in all directions.

**Option B: Bilateral Filter** (edge-preserving)
```cpp
cv::bilateralFilter(gray, blurred, bilateralD, bilateralSigmaColor, bilateralSigmaSpace);
```

Bilateral filter smooths regions while preserving edges. It considers both spatial distance and color similarity:
- `bilateralD`: Diameter of pixel neighborhood
- `sigmaColor`: Filter sigma in color space (larger = more colors mixed)
- `sigmaSpace`: Filter sigma in coordinate space (larger = farther pixels mixed)

#### Step 3: Canny Edge Detection

```cpp
cv::Canny(blurred, edgeImage, cannyThreshold1, cannyThreshold2);
```

The Canny algorithm:
1. Applies Gaussian smoothing (already done above)
2. Computes gradient magnitude and direction using Sobel
3. Applies non-maximum suppression to thin edges
4. Uses hysteresis thresholding:
   - Pixels > `threshold2`: Strong edges (kept)
   - Pixels < `threshold1`: Non-edges (discarded)
   - Pixels in between: Kept only if connected to strong edges

#### Step 4: Morphological Operations

```cpp
if (morphologySize > 0) {
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, 
                                                cv::Size(kernelSize, kernelSize));
    
    // Close operation (dilate then erode) - fills small gaps
    cv::morphologyEx(edgeImage, edgeImage, cv::MORPH_CLOSE, kernel);
    
    // Open operation (erode then dilate) - removes small noise
    cv::morphologyEx(edgeImage, edgeImage, cv::MORPH_OPEN, kernel);
}
```

**Morphological operations**:
- **Dilation**: Expands white regions
- **Erosion**: Shrinks white regions
- **Close** (dilate→erode): Fills small holes and gaps
- **Open** (erode→dilate): Removes small noise dots

#### Step 5: Edge Dilation and Thinning

```cpp
if (edgeDilation > 0) {
    cv::Mat dilateKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, 
                                                      cv::Size(kernelSize, kernelSize));
    cv::dilate(edgeImage, edgeImage, dilateKernel);
    
    // Re-thin edges using Zhang-Suen thinning
    cv::Mat thinned;
    cv::ximgproc::thinning(edgeImage, thinned, cv::ximgproc::THINNING_ZHANGSUEN);
    edgeImage = thinned;
}
```

This technique connects fragmented edges (like hair strands):
1. **Dilate**: Expand edges so nearby fragments touch
2. **Thin**: Reduce back to 1-pixel wide lines using skeletonization

**Zhang-Suen Thinning Algorithm**: Iteratively removes pixels from the boundaries while preserving topology (no breaking of lines).

#### Step 6: Edge Smoothing

```cpp
if (edgeSmoothing > 0) {
    cv::GaussianBlur(edgeImage, edgeImage, cv::Size(kernelSize, kernelSize), 0);
    cv::threshold(edgeImage, edgeImage, 30, 255, cv::THRESH_BINARY);
}
```

Softens jagged edges:
1. Blur the binary edge image
2. Re-threshold to get clean edges (30 as threshold prevents losing thin edges)

### Contour Detection

**Function**: `ImageProcessor::findContours()`

```cpp
void ImageProcessor::findContours() {
    contours.clear();
    cv::Mat tempEdge = edgeImage.clone();
    std::vector<cv::Vec4i> hierarchy;

    cv::findContours(tempEdge, contours, hierarchy, 
                     cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // Filter contours
    std::vector<std::vector<cv::Point>> filteredContours;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        double length = cv::arcLength(contour, false);
        
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
```

**Contour retrieval modes**:
- `RETR_TREE`: Retrieves all contours with full hierarchy

**Contour approximation**:
- `CHAIN_APPROX_SIMPLE`: Compresses horizontal, vertical, and diagonal segments

**Filtering**:
- `contourArea()`: Calculates enclosed area
- `arcLength()`: Calculates perimeter length
- `approxPolyDP()`: Douglas-Peucker algorithm for polygon simplification

### Brush Stroke Generation

**Function**: `ImageProcessor::createBrushStrokes()`

This is the most complex function that creates artistic brush stroke renderings with density-adaptive edge following.

#### Design Philosophy

The brush stroke algorithm balances two goals:
1. **Edge fidelity**: Strokes should closely follow detected edges for recognizable results
2. **Artistic quality**: Subtle variations give a hand-drawn appearance without being too random

#### Step 1: Compute Gradient Direction

```cpp
cv::Mat gradX, gradY;
cv::Sobel(gray, gradX, CV_32F, 1, 0, 3);  // Horizontal gradient
cv::Sobel(gray, gradY, CV_32F, 0, 1, 3);  // Vertical gradient
```

Sobel operators compute image gradients, which indicate edge direction.

#### Step 2: Compute Edge Density Map

```cpp
cv::Mat edgeDensity;
int densityKernelSize = 21;
cv::blur(edgeImage, edgeDensity, cv::Size(densityKernelSize, densityKernelSize));

// Normalize to 0-1 range
double minDensity, maxDensity;
cv::minMaxLoc(edgeDensity, &minDensity, &maxDensity);
edgeDensity.convertTo(edgeDensity, CV_32F, 
                      1.0 / (maxDensity - minDensity), 
                      -minDensity / (maxDensity - minDensity));
```

The density map measures edge concentration in each local region:
- **High density** (eyes, hair, detailed areas): More edges packed together
- **Low density** (simple contours, backgrounds): Sparse edge distribution

This map controls:
- **Stroke skipping**: Prevent over-saturation in busy areas
- **Angle variation**: Tighter edge following in complex regions
- **Brightness adjustment**: Uniform appearance across all areas

#### Step 3: Draw Main Strokes Along Contours

```cpp
// Tight angle constraints for edge-following
const float maxAngleOffset = 5.0f * CV_PI / 180.0f;   // Max 5 degrees
const float minAngleOffset = 0.5f * CV_PI / 180.0f;   // Min 0.5 degrees

for (const auto& contour : contours) {
    for (size_t i = 0; i < contour.size() - 1; i++) {
        cv::Point pt1 = contour[i];
        cv::Point pt2 = contour[(i + 1) % contour.size()];
        
        // Get local density at this point
        float density = edgeDensity.at<float>(pt1.y, pt1.x);
        
        // Density-adaptive skip: up to 70% skip in dense areas
        float skipProbability = density * 0.7f;
        if (skipDist(gen) < skipProbability) continue;
        
        // Calculate tangent direction from contour segment
        float tangentAngle = atan2(pt2.y - pt1.y, pt2.x - pt1.x);
        
        // Small angle offset for brush effect (density-adaptive)
        float angleRange = std::max(minAngleOffset, 
            maxAngleOffset - (density * (maxAngleOffset - minAngleOffset)));
        float angleOffset = angleOffsetDist(gen) * (signDist(gen) ? 1 : -1);
        float strokeAngle = tangentAngle + angleOffset;
        
        // Stroke length with 10% extension for overlap
        float strokeLen = sqrt(pow(pt2.x - pt1.x, 2) + pow(pt2.y - pt1.y, 2)) * 1.1f;
        
        // Minimal position jitter (-1 to +1 pixels)
        int offset_x = offsetDist(gen);
        int offset_y = offsetDist(gen);
        
        cv::Point strokePt1(pt1.x + offset_x, pt1.y + offset_y);
        cv::Point strokePt2(
            pt1.x + offset_x + static_cast<int>(strokeLen * cos(strokeAngle)),
            pt1.y + offset_y + static_cast<int>(strokeLen * sin(strokeAngle))
        );
        
        // Brightness: 220-255, slightly darker in dense areas
        int baseGray = 220 + static_cast<int>((1.0f - density) * 35);
        
        cv::line(brushStrokeImage, strokePt1, strokePt2, 
                 cv::Scalar(grayVal, grayVal, grayVal), 
                 brushSize, cv::LINE_AA);
    }
}
```

Key improvements over random stroke approaches:
- **Tight angle bounds** (0.5°-5°): Strokes closely follow edges
- **Density-adaptive skip**: Prevents over-drawing in complex areas
- **Consistent brightness**: No blown-out white regions
```

#### Step 4: Draw Secondary Sketch Lines

```cpp
if (brushDensity < 15) {
    for (size_t i = 0; i < contour.size() - 1; i += 3) {
        // Every 3rd contour point for sparser coverage
        // Higher skip probability (80% in dense areas)
        // Thinner strokes (brushSize - 1)
        // Same tight angle constraints
    }
}
```

Secondary strokes add subtle texture without overwhelming the image.

#### Step 5: Draw Strokes on Individual Edge Pixels

```cpp
for (int y = 1; y < edgeImage.rows - 1; y++) {
    for (int x = 1; x < edgeImage.cols - 1; x++) {
        if (edgeImage.at<uchar>(y, x) > 128) {
            // Get local density for skip decision (75% in dense areas)
            float density = edgeDensity.at<float>(y, x);
            float skipProbability = density * 0.75f;
            if (skipDist(gen) < skipProbability) continue;
            
            // Get gradient direction at this pixel
            float gx = gradX.at<float>(y, x);
            float gy = gradY.at<float>(y, x);
            
            // Gradient is perpendicular to edge, rotate 90° for tangent
            float gradientAngle = atan2(gy, gx);
            float tangentAngle = gradientAngle + CV_PI / 2.0;
            
            // Apply same tight angle constraints (0.5°-5°)
            float angleOffset = angleOffsetDist(gen) * (signDist(gen) ? 1 : -1);
            float strokeAngle = tangentAngle + angleOffset;
            
            // Draw short stroke along edge direction
            int strokeLen = brushSize * 2;
            // Consistent brightness (210-255 based on density)
        }
    }
}
```

This adds fine detail strokes following individual edge pixels, filling gaps between contour-based strokes.

### Neon Effect Generation

**Function**: `ImageProcessor::createNeonEffect()`

The neon effect creates vibrant, glowing edge visualizations with multiple coloring strategies.

#### Overview

The neon effect supports two main modes:
1. **Per-Contour Mode**: Each contour gets a unique color from a rainbow palette
2. **Object Mode**: Contours are grouped into objects, with the N largest objects colored uniquely

#### Step 1: Generate Rainbow Color Palette

```cpp
std::vector<cv::Scalar> generateRainbowPalette(int count) {
    std::vector<cv::Scalar> palette;
    for (int i = 0; i < count; i++) {
        float hue = (float)i / count * 180.0f;  // OpenCV uses 0-180 for hue
        cv::Mat hsv(1, 1, CV_8UC3, cv::Scalar(hue, 255, 255));
        cv::Mat bgr;
        cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);
        palette.push_back(cv::Scalar(bgr.at<cv::Vec3b>(0,0)));
    }
    return palette;
}
```

Creates evenly-spaced colors across the spectrum for visually distinct contour coloring.

#### Step 2a: Per-Contour Coloring (with optional K-Means)

When `neonPerContour` is enabled:

```cpp
if (neonKMeansEnabled) {
    // Calculate centroid of each contour
    std::vector<cv::Point2f> centroids;
    for (const auto& contour : contours) {
        cv::Moments m = cv::moments(contour);
        centroids.push_back(cv::Point2f(m.m10/m.m00, m.m01/m.m00));
    }
    
    // Run K-Means clustering on centroids
    cv::Mat labels, centers;
    cv::kmeans(centroids, neonKMeansK, labels, 
               cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0),
               3, cv::KMEANS_PP_CENTERS, centers);
    
    // Apply near-distance filter
    for (int i = 0; i < contours.size(); i++) {
        float distToCenter = cv::norm(centroids[i] - centers.at<cv::Point2f>(labels.at<int>(i)));
        if (distToCenter > neonKMeansNearDistancePx) {
            // Too far from cluster - give unique color
            contourColors[i] = uniqueColor++;
        } else {
            // Close enough - use cluster color
            contourColors[i] = labels.at<int>(i);
        }
    }
}
```

**K-Means clustering** groups contours by spatial proximity:
- Contours near each other share the same color
- `neonKMeansK` controls how many color groups exist
- `neonKMeansNearDistancePx` prevents distant contours from being incorrectly grouped

#### Step 2b: Object-Based Coloring

When `neonPerContour` is disabled, contours are grouped into objects:

```cpp
// Create a mask from all edges
cv::Mat objectMask = edgeImage.clone();

// Dilate to connect nearby edges into objects
cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, 
                                            cv::Size(neonJoinSize, neonJoinSize));
cv::dilate(objectMask, objectMask, kernel);

// Find connected components (objects)
cv::Mat labels;
int numObjects = cv::connectedComponents(objectMask, labels);

// Sort objects by area, keep only the N largest
// Assign unique colors to main objects, background color to the rest
```

This mode:
- Uses morphological dilation to merge nearby edges
- Identifies distinct objects via connected component analysis
- Colors only the `neonMaxObjects` largest objects
- Applies `neonMinObjectAreaRatio` to filter noise

#### Step 3: Draw Glowing Contours

```cpp
for (int layer = neonGlowStrength; layer >= 0; layer--) {
    float alpha = 1.0f / (layer + 1);  // Outer layers are more transparent
    int thickness = 1 + layer * 2;      // Outer layers are thicker
    
    for (int i = 0; i < contours.size(); i++) {
        cv::Scalar color = palette[contourColors[i] % palette.size()];
        cv::Scalar glowColor = color * alpha;  // Fade for glow effect
        cv::drawContours(neonImage, contours, i, glowColor, thickness, cv::LINE_AA);
    }
}

// Apply Gaussian blur for glow
cv::GaussianBlur(neonImage, neonImage, cv::Size(neonGlowSize, neonGlowSize), 0);

// Draw sharp core on top
for (int i = 0; i < contours.size(); i++) {
    cv::Scalar color = palette[contourColors[i] % palette.size()];
    cv::drawContours(neonImage, contours, i, color, 1, cv::LINE_AA);
}
```

The glow effect is created by:
1. Drawing multiple layers of decreasing opacity and increasing thickness
2. Applying Gaussian blur to create the soft glow
3. Drawing the sharp, bright core line on top

#### Neon Parameters Reference

| Parameter | Description |
|-----------|-------------|
| `neonPerContour` | Enable per-contour rainbow coloring |
| `neonKMeansEnabled` | Enable K-Means clustering for contour grouping |
| `neonKMeansK` | Number of K-Means clusters |
| `neonKMeansNearDistancePx` | Max distance for contours to stay in cluster |
| `neonEdgeColor` | Color for background/non-main edges |
| `neonGlowStrength` | Number of glow layers (1-5) |
| `neonGlowSize` | Gaussian blur kernel size for glow |
| `neonMaxObjects` | Maximum main objects to color (object mode) |
| `neonMinObjectAreaRatio` | Minimum object area as ratio of image |
| `neonJoinSize` | Morphological dilation size for object joining |

---

## Rendering System

### OpenGL Setup

**OpenGL Context Configuration**:
```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

Uses OpenGL 4.1 Core Profile for modern shader-based rendering.

### Shader System

**Vertex Shader**:
```glsl
#version 410 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(position, 1.0);
    TexCoord = texCoord;
}
```

Passes through vertex positions and texture coordinates.

**Fragment Shader**:
```glsl
#version 410 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;

void main() {
    FragColor = texture(texture1, TexCoord);
}
```

Samples the texture at the given coordinates.

### Texture Management

**Loading OpenCV Mat as OpenGL Texture**:
```cpp
GLuint Renderer::loadTexture(const cv::Mat& image) {
    cv::Mat displayImage = image.clone();
    
    // Flip vertically (OpenCV: top-left origin, OpenGL: bottom-left)
    cv::flip(displayImage, displayImage, 0);
    
    // Ensure RGB format
    if (displayImage.channels() == 1) {
        cv::cvtColor(displayImage, displayImage, cv::COLOR_GRAY2RGB);
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
```

### Display Modes

| Mode | Description | Data Source |
|------|-------------|-------------|
| ORIGINAL | Shows loaded image | `originalImage` |
| EDGES | Shows Canny edge detection | `edgeImage` |
| CONTOURS | Shows contours overlaid on image | `originalImage` + `contours` |
| BRUSH_STROKES | Shows artistic brush strokes | `brushStrokeImage` |
| COMBINED | Shows brush strokes + contour lines | `brushStrokeImage` + `contours` |

---

## User Interface

### ImGui Integration

ImGui is initialized with GLFW and OpenGL3 backends:

```cpp
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
```

### Control Panel

The control panel provides real-time parameter adjustment:

**File Controls**:
- Image path text input
- Browse button (uses tinyfiledialogs)
- Load button

**Display Mode Selection**:
- Dropdown with 5 modes

**Edge Detection Parameters**:
- Canny T1 slider (10-200)
- Canny T2 slider (50-400)
- Min Contour Area slider (1-1000)
- Min Contour Length slider (1-200)

**Noise Reduction**:
- Blur Strength slider (1-21)
- Bilateral Filter checkbox
- Bilateral Diameter slider (3-21)
- Sigma Color slider (10-200)
- Sigma Space slider (10-200)
- Morphology Size slider (0-7)

**Edge Smoothing**:
- Edge Dilation slider (0-7)
- Edge Blur slider (0-11)
- Contour Smoothing slider (0-10)

**Brush Stroke Settings**:
- Brush Size slider (1-15)
- Brush Density slider (1-20)

**Stroke Settings**:
- Stroke Color picker (RGBA)
- Stroke Width slider (1-10)

**Info Display**:
- Image dimensions
- Contour count

### File Browser

Uses tinyfiledialogs for native file dialogs:

```cpp
const char* filterPatterns[] = { 
    "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tiff", "*.webp" 
};
char* selectedFile = tinyfd_openFileDialog(
    "Select Image",
    "",
    7,
    filterPatterns,
    "Image Files",
    0
);
```

---

## Build System

### CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.20)
project(NeonBuzz)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(OpenGL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# Link everything
target_link_libraries(NeonBuzz PRIVATE 
    OpenGL::OpenGL
    glfw
    GLEW::GLEW
    ${OpenCV_LIBS}
)
```

### Build Commands

```bash
# Configure
cmake -B build -S .

# Compile
make -C build -j$(nproc)

# Run
./build/NeonBuzz [optional_image_path]
```

---

## Dependencies

| Library | Purpose | Version |
|---------|---------|---------|
| OpenGL | GPU rendering | 4.1+ |
| GLFW | Window/input management | 3.x |
| GLEW | OpenGL extension loading | 2.x |
| OpenCV | Image processing | 4.x |
| opencv_ximgproc | Thinning algorithm | (part of OpenCV contrib) |
| ImGui | Immediate mode GUI | 1.89+ |
| tinyfiledialogs | Native file dialogs | - |
| GLM | Math library | 0.9.9+ |

---

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              User Input                                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────────────────┐  │
│  │ Load Image  │  │ Adjust Params│  │        Select Display Mode         │  │
│  └──────┬──────┘  └──────┬──────┘  └──────────────────┬──────────────────┘  │
└─────────┼────────────────┼────────────────────────────┼─────────────────────┘
          │                │                            │
          ▼                ▼                            │
┌─────────────────────────────────────────────┐         │
│           ImageProcessor                     │         │
│  ┌─────────────────────────────────────┐    │         │
│  │           loadImage()                │    │         │
│  │  • cv::imread()                      │    │         │
│  │  • BGR → RGB                         │    │         │
│  │  • Scale to max 1024px               │    │         │
│  └─────────────┬───────────────────────┘    │         │
│                ▼                            │         │
│  ┌─────────────────────────────────────┐    │         │
│  │           detectEdges()              │    │         │
│  │  • Grayscale conversion              │    │         │
│  │  • Gaussian/Bilateral blur           │    │         │
│  │  • Canny edge detection              │    │         │
│  │  • Morphological cleanup             │    │         │
│  │  • Edge dilation + thinning          │    │         │
│  │  • Edge smoothing                    │    │         │
│  └─────────────┬───────────────────────┘    │         │
│                ▼                            │         │
│  ┌─────────────────────────────────────┐    │         │
│  │          findContours()              │    │         │
│  │  • cv::findContours()                │    │         │
│  │  • Filter by area & length           │    │         │
│  │  • Polygon approximation             │    │         │
│  └─────────────┬───────────────────────┘    │         │
│                ▼                            │         │
│  ┌─────────────────────────────────────┐    │         │
│  │        createBrushStrokes()          │    │         │
│  │  • Compute gradient map              │    │         │
│  │  • Compute edge density              │    │         │
│  │  • Draw contour strokes              │    │         │
│  │  • Draw secondary strokes            │    │         │
│  │  • Draw edge pixel strokes           │    │         │
│  └─────────────┬───────────────────────┘    │         │
└────────────────┼────────────────────────────┘         │
                 │                                      │
                 ▼                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Renderer                                        │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │                     Select based on DisplayMode                        │  │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────────┐  │  │
│  │  │ORIGINAL │  │ EDGES   │  │CONTOURS │  │ BRUSH   │  │  COMBINED   │  │  │
│  │  │         │  │         │  │         │  │ STROKES │  │             │  │  │
│  │  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘  └──────┬──────┘  │  │
│  └───────┼───────────┼───────────┼───────────┼────────────────┼─────────┘  │
│          ▼           ▼           ▼           ▼                ▼            │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        loadTexture()                                 │   │
│  │  • Flip image vertically                                             │   │
│  │  • Convert to RGB                                                    │   │
│  │  • Create OpenGL texture                                             │   │
│  └─────────────────────────────────────────┬───────────────────────────┘   │
│                                            ▼                               │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                         Render Quad                                  │   │
│  │  • Bind VAO, shader, texture                                         │   │
│  │  • Draw textured quad                                                │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
                                            │
                                            ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Screen Output                                   │
│                    ┌──────────────────────────────────────┐                 │
│                    │                                      │                 │
│                    │         Rendered Image               │                 │
│                    │                                      │                 │
│                    └──────────────────────────────────────┘                 │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Algorithm Deep Dives

### Canny Edge Detection

The Canny edge detector is a multi-stage algorithm:

1. **Gaussian Smoothing**: Reduces noise that could cause false edges
2. **Gradient Calculation**: Uses Sobel operators to find intensity gradients
3. **Non-Maximum Suppression**: Thins edges to 1 pixel wide by keeping only local maxima
4. **Hysteresis Thresholding**: 
   - Strong edges (> T2): Always kept
   - Weak edges (T1 < x < T2): Kept if connected to strong edges
   - Non-edges (< T1): Discarded

### Bilateral Filter

Unlike Gaussian blur, bilateral filtering is non-linear and edge-preserving:

$$G_p = \frac{1}{W_p} \sum_{q \in S} G_{\sigma_s}(||p-q||) \cdot G_{\sigma_r}(|I_p - I_q|) \cdot I_q$$

Where:
- $G_{\sigma_s}$: Spatial Gaussian (based on pixel distance)
- $G_{\sigma_r}$: Range Gaussian (based on intensity difference)
- $W_p$: Normalization factor

### Zhang-Suen Thinning

An iterative algorithm that removes boundary pixels while preserving connectivity:

1. Mark pixels for deletion based on 8-neighborhood conditions
2. Delete marked pixels
3. Repeat until no more pixels can be deleted

This produces a 1-pixel wide skeleton of the shape.

### Douglas-Peucker Algorithm (Contour Smoothing)

Reduces points in a curve while preserving shape:

1. Connect first and last points with a line
2. Find the point farthest from this line
3. If distance > epsilon, recursively simplify each segment
4. If distance < epsilon, replace segment with the line

---

## Performance Considerations

1. **Image Size Limiting**: Images are scaled to max 1024px to ensure real-time processing
2. **Density-based Stroke Skipping**: Reduces overdraw in busy areas
3. **Anti-aliased Lines**: Uses `cv::LINE_AA` for smooth rendering
4. **Texture Reuse**: Textures are deleted and recreated only when needed
5. **Single Threaded**: Processing is synchronous for simplicity

---

## Extensibility Points

1. **New Display Modes**: Add to `Renderer::DisplayMode` enum and handle in `processFrame()`
2. **Additional Filters**: Add parameters to `ImageProcessor` and apply in `detectEdges()`
3. **Custom Brush Styles**: Modify `createBrushStrokes()` for different artistic effects
4. **Export Functionality**: Add save methods using `cv::imwrite()`
5. **Animation**: Add time-based variation to brush stroke parameters

---

*Document Version: 1.0*
*Last Updated: January 31, 2026*
