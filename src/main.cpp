#include "CrossCraftApplet.hpp"
#include "gl4esinit.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <gc.h>


extern "C" void initialize_gl4es();

int main() {
    setenv("LIBGL_USEVBO", "1", 1);
    setenv("LIBGL_BATCH", "1", 1);
    
    GC_INIT();
    GC_add_roots(&CrossCraft::instance, &CrossCraft::instance + 1);
    
    std::cout << "CrossCraft C++ main() called" << std::endl;
    srand(time(NULL));

    try {
        std::cout << "Initializing gl4es..." << std::endl;
        initialize_gl4es();
        std::cout << "gl4es initialized successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "ERROR initializing gl4es: " << e.what() << std::endl;
        return -1;
    }
    
    std::cout << "Waiting for JavaScript to call startApplet..." << std::endl;
    std::cout << "main() completed successfully" << std::endl;
    
    return 0;
}
 