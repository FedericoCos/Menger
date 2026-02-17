#pragma once

#include "../Helpers/GeneralLibraries.hpp"
#include "../Helpers/GLFWhelper.hpp"

namespace Swapchain{
    SwapchainBundle createSwapchain(vk::raii::PhysicalDevice &physical_device, vk::raii::Device& logical_device, vk::raii::SurfaceKHR &surface, GLFWwindow * window, QueuePool& queue_indices);

    // Helper function to extract a suitable format for the swapchain
    vk::Format chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> available_formats);
    // Helper function to select a correct Extent for the swapchain images
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities, GLFWwindow *window);
    // Helper function to choose the best possible present mode
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &available_present_modes);
};