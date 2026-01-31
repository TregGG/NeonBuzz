# Image Contour Renderer

An OpenGL-based application that loads images, detects edges and contours, and renders them with customizable strokes.

## Features

- **Image Loading**: Load any image format (PNG, JPG, etc.)
- **Edge Detection**: Canny edge detection with adjustable thresholds
- **Contour Detection**: Automatic contour extraction from edges
- **Multiple Display Modes**: Original, Edges, Contours, and Combined views
- **Real-time Parameter Adjustment**: Modify edge detection and rendering parameters in real-time
- **Customizable Strokes**: Adjust stroke color and width
- **ImGui Interface**: Easy-to-use GUI for all controls

## Requirements

- C++17 or later
- CMake 3.20+
- OpenGL 4.1+
- OpenCV 4.0+
- GLFW3
- GLEW

## Installation

### Ubuntu/Debian

```bash
sudo apt-get install libglfw3-dev libglew-dev libopencv-dev
```

### macOS

```bash
brew install glfw3 glew opencv
```

### Windows

Download and install:
- [GLFW](https://www.glfw.org/download.html)
- [GLEW](http://glew.sourceforge.net/)
- [OpenCV](https://opencv.org/releases/)

## Building

```bash
cd /path/to/Imagecontorrender
mkdir -p build && cd build
cmake ..
make
```

## Running

```bash
./ImageContourRenderer [image_path]
```

Or run without arguments and use the GUI to load an image.

## Usage

1. **Load Image**: Enter the path to an image file in the "Image Path" field and click "Load Image"
2. **Select Display Mode**: Choose between Original, Edges, Contours, or Combined view
3. **Adjust Edge Detection**:
   - Canny T1: Lower threshold for edge detection (10-200)
   - Canny T2: Upper threshold for edge detection (50-400)
   - Min Contour Area: Filter out small contours (1-1000)
4. **Customize Strokes**:
   - Change the stroke color
   - Adjust the stroke width

## Controls

- **Display Mode Combo**: Switch between visualization modes
- **Real-time Sliders**: Adjust parameters and see results immediately
- **Color Picker**: Set custom stroke colors

## Project Structure

```
├── CMakeLists.txt          # Build configuration
├── include/
│   ├── App.h               # Main application class
│   ├── ImageProcessor.h    # Image processing with OpenCV
│   └── Renderer.h          # OpenGL rendering
├── src/
│   ├── main.cpp            # Entry point
│   ├── App.cpp             # Application implementation
│   ├── ImageProcessor.cpp  # Image processing implementation
│   └── Renderer.cpp        # OpenGL rendering implementation
└── assets/                 # Sample images (add your own)
```

## Technical Details

### Image Processing
- **Color Conversion**: BGR to RGB conversion for OpenGL compatibility
- **Resizing**: Automatic downscaling for performance (max 1024x1024)
- **Edge Detection**: Canny edge detection with Gaussian blur preprocessing
- **Contour Detection**: OpenCV contour detection with area filtering

### Rendering
- **OpenGL Core Profile**: 4.1+ for modern GPU rendering
- **Texture-based Display**: Images rendered as textured quads
- **Stroke Rendering**: Contours drawn using OpenCV's drawContours

### GUI
- ImGui for intuitive parameter control
- Real-time feedback on processing results
- Display statistics (image dimensions, contour count)

## Notes

- Large images are automatically resized to 1024x1024 for performance
- Edge detection parameters are tuned for general-purpose use; adjust for specific image types
- Stroke width is in pixels and may need adjustment based on image size
