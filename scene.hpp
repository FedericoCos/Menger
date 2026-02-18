#pragma once

#include "VulkanEngine/engine.hpp"
#include "cube.hpp"

struct CubeBuffer{
    glm::vec4 position;
};

struct FirstCubeBuffer{
    glm::mat4 rotation_matrix;
    glm::mat4 scale_matrix;
    glm::mat4 center_matrix;
};

class Scene : public Engine {
public:

    // Closing function
    void cleanup() override;


private:
    // Varibales related to cube
    const uint32_t MAX_CUBES = 160000;
    uint32_t current_cubes = 1;
    uint32_t current_menger_step = 1;
    double cube_size = 9.0;
    glm::vec3 center = glm::vec3(0.f, 0.f, -10.f);
    float rot_speed = 0.05f;
    std::vector<Cube> cubes;
    std::vector<MappedUBO> single_cube_ubo;

    // Virtual function from engine
    void createInitResources() override;
    void updateUniformBuffers(float dtime, int current_frame) override;
    void recordCommandBuffer(uint32_t image_index) override;
    void processInput() override;

    // Function that splits and calculates new cubes
    void mengerStep();
};