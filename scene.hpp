#pragma once

#include "VulkanEngine/engine.hpp"

class Scene : public Engine {
public:


private:
    void createInitResources() override;
    void updateUniformBuffers(float dtime, int current_frame) override;
    void recordCommandBuffer(uint32_t image_index) override;
    void processInput() override;
};