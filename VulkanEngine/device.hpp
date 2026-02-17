#pragma once

#include "../Helpers/GeneralLibraries.hpp"

namespace Device{
    // Returns physical device based on a metric score
    vk::raii::PhysicalDevice pickPhysicalDevice(const vk::raii::Instance& instance);

    // Return logical device and modifies the indices structure passed as parameter to hold the queue index
    vk::raii::Device createLogicalDevice(const vk::raii::PhysicalDevice &physical_device, vk::raii::SurfaceKHR &surface, QueuePool &indices);

    // Returns a score based on device capabilities. Return a score of 0 if device is not suitable for the application. Right now, score is 1 for all GPUs and 2 for discrete ones
    uint8_t calculateScore(vk::raii::PhysicalDevice &device);

    // Find the queue family indices. Modifies the indices structure passed as parameter
    void findQueueFamilies(const vk::raii::PhysicalDevice &physical_device, const vk::raii::SurfaceKHR &surface, QueuePool &indices);

    // Creates the necessary command pools
    vk::raii::CommandPool createCommandPool(const vk::raii::Device &logical_device, vk::CommandPoolCreateFlagBits flags, uint32_t queue_index);

    // Creates the necessary command buffers
    vk::raii::CommandBuffers createCommandBuffer(vk::raii::CommandPool &command_pool, vk::CommandBufferLevel level, int max_frames_in_flight, vk::raii::Device &logical_device);

    // Helper function for printing vma operation results
    const char* VmaResultToString(VkResult r);

    // Creates a buffer
    AllocatedBuffer createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, std::string name, VmaAllocator &vma_allocator);

    // Copies one buffer into another
    void copyBuffer(AllocatedBuffer &source_buffer, AllocatedBuffer &destination_buffer, vk::DeviceSize size, vk::raii::Device &logical_device, QueuePool &queue_pool,
                    vk::DeviceSize src_offset);
    
    // Functions for single time commands
    vk::raii::CommandBuffer beginSingleTimeCommands(vk::raii::CommandPool &command_pool, vk::raii::Device &logical_device);
    void endSingleTimeCommands(vk::raii::CommandBuffer &command_buffer, vk::raii::Queue &queue);
}