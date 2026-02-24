#pragma once

#include "VulkanEngine/engine.hpp"
#include "cube.hpp"
#include "cinematicCamera.hpp"

struct CubeBuffer{
    glm::vec4 position;
};

struct FirstCubeBuffer{
    glm::mat4 rotation_matrix;
    glm::vec4 center_and_scale;
};

struct PointLightBuffer{
    glm::vec4 position;
    glm::vec4 color; // last value will be intensity
};

class Scene : public Engine {
public:

    // Closing function
    void cleanup() override;


private:
    // Varibales related to cube
    const uint32_t MAX_CUBES = 3200000;
    const uint8_t max_menger_level = 5;
    uint32_t current_cubes = 1;
    uint32_t current_menger_level = 0;
    double cube_size = 2187.0;
    double original_size;
    glm::vec3 center = glm::vec3(0.f, 0.f, -3000.f);
    glm::vec3 rot_speed = glm::vec3(0);
    Cube main_cube;
    std::vector<glm::vec3> cube_positions;
    std::vector<glm::vec3> temp_positions;
    std::vector<MappedUBO> single_cube_ubo;
    std::vector<glm::vec4> positions;
    std::vector<MappedUBO> cube_ssbo_mapped;
    std::vector<MappedUBO> cube_ssbo; // They are not actually mapped, I should fix it later but it is to make it work with writeDescriptor
    uint8_t dirty_positions = 0;

    // Variables related to camera
    C_camera c_camera;
    float n_plane = 0.1f;
    float f_plane = 10000.f;
    std::vector<glm::vec4> free_positions;
    std::vector<glm::vec4> grid_positions;

    // Variables related to light
    const uint32_t MAX_LIGHTS = 168421;
    std::vector<glm::vec4> centers_and_levels;
    uint32_t current_pointlights = 0;
    uint32_t current_connections = 0;
    std::vector<MappedUBO> light_ssbo_mapped;
    std::vector<MappedUBO> light_ssbo;
    std::vector<PointLightBuffer> pointlight_buffers;
    float base_light_intensity = 500000.f;
    uint16_t intensity_divisor = 9;
    float light_threshold = 0.1f;
    std::array<glm::vec3, 5> light_colors = {
        glm::vec3(0.00f, 0.10f, 0.50f), // Level 0: Deep marine blue
        glm::vec3(0.00f, 0.50f, 0.80f), // Level 1: Ocean cyan
        glm::vec3(0.00f, 0.85f, 0.60f), // Level 2: Mint/Teal
        glm::vec3(0.40f, 0.95f, 0.20f), // Level 3: Bioluminescent lime
        glm::vec3(0.80f, 1.00f, 0.60f)  // Level 4: Pale yellow-green
    };

    // Variables for the simil deferred shading
    std::vector<uint32_t> light_indices; // This will connect each cube to only the lights it can see, a simil deferred shading
    std::vector<uint32_t> light_indices_size; // Indicates how many lights per specific cube
    std::vector<MappedUBO> light_indices_ssbo;
    std::vector<MappedUBO> light_indices_ssbo_mapped;
    std::vector<MappedUBO> light_indices_size_ssbo;
    std::vector<MappedUBO> light_indices_size_ssbo_mapped;
    uint32_t MAX_CONNECTIONS = 50;


    // Virtual function from engine
    void createInitResources() override;
    void updateUniformBuffers(float dtime, int current_frame) override;
    void recordCommandBuffer(uint32_t image_index) override;
    void processInput() override;

    // Function that splits and calculates new cubes
    void mengerStep();

    // This function takes all the cubes, all the lights, and connects them
    void connectLights();

    // Function to setup all the variables befor initialization
    void initialSetup();

    // Function to allocate cubes resources for the GPU
    void initCubeResources();

    // Function to allocate lights resources for the GPU
    void initLightResources();

    // Function to allocate camera resources for the GPU
    void initCameraResources();
};