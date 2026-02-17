#include "VulkanEngine/engine.hpp"

int main(int argc, char * argv[]){
    Engine engine;

    std::array<uint32_t, 2> dimensions{0, 0};
    // Extracting title from input
    const std::string title = argv[1];

    // Extracting dimensions from input
    for(size_t i = 2; i < argc; ++i){
        dimensions[i-2] = std::atoi(argv[i]);
    }

    engine.init(title, dimensions[0], dimensions[1]);

    engine.run();


    engine.cleanup();
}