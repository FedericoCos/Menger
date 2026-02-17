#pragma once

#include "../Helpers/GeneralLibraries.hpp"

namespace MemoryAllocator{
    // Creates the memory allocator
    VmaAllocator createMemoryAllocator(const vk::raii::PhysicalDevice &physical_device, const vk::raii::Device &logical_device, const vk::raii::Instance &instance);
}