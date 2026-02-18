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

class Scene : public Engine {
public:

    // Closing function
    void cleanup() override;


private:
    // Varibales related to cube
    const uint32_t MAX_CUBES = 64000000;
    uint32_t current_cubes = 1;
    uint32_t current_menger_step = 1;
    double cube_size = 9.0;
    glm::vec3 center = glm::vec3(0.f, 0.f, -10.f);
    glm::vec3 rot_speed = glm::vec3(0.05f, 0.05f, 0.0f);
    Cube main_cube;
    std::vector<glm::vec3> cube_positions;
    std::vector<MappedUBO> single_cube_ubo;
    std::vector<glm::vec4> positions;
    std::vector<MappedUBO> cube_ssbo_mapped;
    std::vector<MappedUBO> cube_ssbo; // They are not actually mapped, I should fix it later but it is to make it work with writeDescriptor
    uint8_t dirty_positions = 0;

    // Virtual function from engine
    void createInitResources() override;
    void updateUniformBuffers(float dtime, int current_frame) override;
    void recordCommandBuffer(uint32_t image_index) override;
    void processInput() override;

    // Function that splits and calculates new cubes
    void mengerStep();
};