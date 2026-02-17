#include "memory.hpp"

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include "../Helpers/vk_mem_alloc.h"
#endif

VmaAllocator MemoryAllocator::createMemoryAllocator(const vk::raii::PhysicalDevice &physical_device, const vk::raii::Device &logical_device, const vk::raii::Instance &instance)
{
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.physicalDevice = *physical_device;
    allocator_info.device = *logical_device;
    allocator_info.instance = *instance;
    allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT | // allows VMA to allocate memory that can be referenced by a 64-bit GPU pointer rather than just a descriptor set.
                           VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT; // the allocator can communicate with the OS to see the available memory

    VmaAllocator allocator;
    VkResult res = vmaCreateAllocator(&allocator_info, &allocator);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("vmaCreateAllocator failed");
    }


    return allocator;
}