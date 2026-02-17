#include "swapchain.hpp"

SwapchainBundle Swapchain::createSwapchain(vk::raii::PhysicalDevice &physical_device, vk::raii::Device& logical_device, vk::raii::SurfaceKHR &surface, GLFWwindow * window, QueuePool& queue_indices){
    SwapchainBundle swapchain;

    vk::SurfaceCapabilitiesKHR surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
    swapchain.format = chooseSwapSurfaceFormat(physical_device.getSurfaceFormatsKHR(surface));
    swapchain.extent = chooseSwapExtent(surface_capabilities, window);
    swapchain.present_mode = chooseSwapPresentMode(physical_device.getSurfacePresentModesKHR(surface));

    auto min_image_count = std::max(3u, surface_capabilities.minImageCount);
    min_image_count = (surface_capabilities.maxImageCount > 0 && min_image_count > surface_capabilities.maxImageCount) 
                    ? surface_capabilities.maxImageCount : min_image_count; // maxImageCount == 0 indicates no limit on images count

    if(!queue_indices.isIndexComplete()){
        throw std::runtime_error("Incomplete queue indices passed to the swapchain!");
    }

    const uint32_t queue_family_indices[] = {queue_indices.graphics_family.value(), queue_indices.present_family.value()};

    vk::SwapchainCreateInfoKHR swapchain_create_info;
    swapchain_create_info.surface = surface;
    swapchain_create_info.minImageCount = min_image_count;
    swapchain_create_info.imageFormat = swapchain.format;
    swapchain_create_info.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment; //  we are rendering directly to the images, add eTransferDsty to perform post-processing actions
    swapchain_create_info.preTransform = surface_capabilities.currentTransform; // Handles hardware level rotation, check for phone applications
    swapchain_create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque; // The window doesn't blend with windows behind it
    swapchain_create_info.presentMode = swapchain.present_mode;
    swapchain_create_info.clipped = true; // If a pixel is obscured by another window, Vulkan won't bother rendering it.
    swapchain_create_info.imageExtent = swapchain.extent;

    if(queue_family_indices[0] != queue_family_indices[1]){
        // Allows multiple queue families to access the images simultaneously without explicit ownership tansfers. Easier to code but less performant
        swapchain_create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else{
        // Most common. Ownership must be asked and obtained. Offerst best performance
        swapchain_create_info.imageSharingMode = vk::SharingMode::eExclusive;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = nullptr;
    }
    swapchain.sharing_mode = swapchain_create_info.imageSharingMode;

    swapchain.swapchain = vk::raii::SwapchainKHR(logical_device, swapchain_create_info);

    swapchain.images = swapchain.swapchain.getImages();
    if(swapchain.images.size() <= 0){
        throw std::runtime_error("Error with the acquisition of the images of the swapchain!");
    }

    swapchain.image_views.clear();

    vk::ImageViewCreateInfo imageview_create_info;
    imageview_create_info.viewType = vk::ImageViewType::e2D;
    imageview_create_info.format = swapchain.format;
    imageview_create_info.subresourceRange = { 
        vk::ImageAspectFlagBits::eColor, // aspectMask
        0, // baseMipLevel 
        1, // levelCount
        0, // baseArrayLayer
        1};// layerCount -> how many images compose the single image (multiple needed for stereographic app)

    for(const vk::Image& image : swapchain.images){
        imageview_create_info.image = image;
        swapchain.image_views.emplace_back(logical_device, imageview_create_info);
    }

    if(swapchain.image_views.size() <= 0 || swapchain.image_views.size() != swapchain.images.size()){
        throw std::runtime_error("Error with the creation of the image views for the swapchaiin!");
    }

    std::cout << "Created Swapchain:\n" << swapchain.to_str() <<  std::endl;

    return std::move(swapchain);
}

vk::Format Swapchain::chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> available_formats){
    const auto format_it = std::ranges::find_if(available_formats, 
                [](const auto& format){
                    return format.format == vk::Format::eB8G8R8A8Srgb &&
                            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
                });
    return format_it != available_formats.end() ? format_it -> format : available_formats[0].format;
}

vk::Extent2D Swapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow * window){
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()){
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

vk::PresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes){
    return std::ranges::any_of(available_present_modes, [](const vk::PresentModeKHR value){
        return vk::PresentModeKHR::eMailbox == value;
    }) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}