#include "App.h"
#include <iostream>

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
