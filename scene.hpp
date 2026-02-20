#pragma once

#include "VulkanEngine/engine.hpp"
#include "cube.hpp"

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
    const uint32_t MAX_CUBES = 64000000;
    uint32_t current_cubes = 1;
    uint32_t current_menger_step = 1;
    double cube_size = 2187.0;
    double original_size;
    glm::vec3 center = glm::vec3(0.f, 0.f, -3000.f);
    glm::vec3 rot_speed = glm::vec3(0.05f, 0.05f, 0.0f);
    Cube main_cube;
    std::vector<glm::vec3> cube_positions;
    std::vector<glm::vec3> temp_positions;
    std::vector<MappedUBO> single_cube_ubo;
    std::vector<glm::vec4> positions;
    std::vector<MappedUBO> cube_ssbo_mapped;
    std::vector<MappedUBO> cube_ssbo; // They are not actually mapped, I should fix it later but it is to make it work with writeDescriptor
    uint8_t dirty_positions = 0;

    // Variables related to camera
    float n_plane = 0.1f;
    float f_plane = 10000.f;

    // Variables related to light
    const uint32_t MAX_LIGHTS = 3368421;
    std::vector<glm::vec4> centers_and_levels;
    uint32_t current_pointlights = 0;
    std::vector<MappedUBO> light_ssbo_mapped;
    std::vector<MappedUBO> light_ssbo;
    std::vector<PointLightBuffer> pointlight_buffers;
    float base_light_intensity = 100000.f;
    uint16_t intensity_divisor = 10;
    float light_threshold = 0.01;
    std::array<glm::vec3, 3> light_colors = {
        glm::vec3(1.0, 0.65, 0.0),
        glm::vec3(0.55, 0.31, 0.08),
        glm::vec3(0.06, 0.06, 0.1)
    };



    // Virtual function from engine
    void createInitResources() override;
    void updateUniformBuffers(float dtime, int current_frame) override;
    void recordCommandBuffer(uint32_t image_index) override;
    void processInput() override;

    // Function that splits and calculates new cubes
    void mengerStep();
};