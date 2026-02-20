#include "image.hpp"

AllocatedImage Image::createImage(uint32_t width, uint32_t height, vk::ImageType image_type, uint32_t mip_levels, 
    vk::SampleCountFlagBits msaa_samples, vk::Format format, uint32_t array_layers, vk::ImageTiling tiling, 
    vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, std::string image_name, vk::ImageCreateFlags flags, VmaAllocator &vma_allocator)
{
    AllocatedImage image;
    image.image_format = format;
    image.image_extent = vk::Extent3D(width, height, 1);
    image.mip_levels = mip_levels;
    image.image_type = image_type;
    image.array_layers = array_layers;
    image.allocator = vma_allocator;
    image.image_name = image_name;

    vk::ImageCreateInfo image_info{};
    image_info.imageType = image.image_type; // e1D is for gradients or simple lookup tables, e2D is texture, render targets, and depth buffers, e3D is for volumetric data
    image_info.format = image.image_format; // arrangement of bytes per pixel
    image_info.extent = image.image_extent;
    image_info.arrayLayers = image.array_layers; // number of layers in the image (for example, 6 is for Cube Maps)
    image_info.tiling = tiling; // how to organize the pixels in memory, eOpimal for GPU only and best performace, eLinear to be read also by CPU
    image_info.usage = usage; // what the image is used for
    image_info.sharingMode = vk::SharingMode::eExclusive; // Tells that only one queue can hold ownership of the image at a time
    image_info.initialLayout = vk::ImageLayout::eUndefined;
    image_info.mipLevels = image.mip_levels;
    image_info.samples = msaa_samples;
    image_info.flags = flags;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VkImage temp_image;
    VkImageCreateInfo temp_image_info = (VkImageCreateInfo)image_info;
    VkResult result = vmaCreateImage(vma_allocator, &temp_image_info, &alloc_info, &temp_image, &image.allocation, nullptr);

    if (result != VK_SUCCESS)
    {
        std::cerr << "Error: vmaCreateImage failed with code: " << result << std::endl;

        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            std::cerr << "--> You are out of GPU device memory." << std::endl;
        } else if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
            std::cerr << "--> You are out of system (host) memory." << std::endl;
        } else {
            std::cerr << "--> This is NOT an out-of-memory error. Check parameters." << std::endl;
        }
        
        image.image = VK_NULL_HANDLE; // Ensure handle is null on failure
    }else{
        image.image = vk::Image(temp_image);
    }

    std::cout << "Created Image:\n" << image.to_str() << std::endl;
    return image;
}

vk::raii::ImageView Image::createImageView(AllocatedImage &image, vk::raii::Device &logical_device)
{
    vk::ImageAspectFlagBits aspect_flags = vk::ImageAspectFlagBits::eColor;
    if(image.image_format == vk::Format::eD32Sfloat || image.image_format == vk::Format::eD24UnormS8Uint
        || image.image_format == vk::Format::eD32SfloatS8Uint){
        aspect_flags = vk::ImageAspectFlagBits::eDepth;
    }

    vk::ImageViewCreateInfo view_info = {};
    if(image.array_layers == 1){
        view_info.viewType = vk::ImageViewType::e2D;
    }
    else if(image.array_layers == 6 && image.flags & vk::ImageCreateFlagBits::eCubeCompatible){
        view_info.viewType = vk::ImageViewType::eCube;
    }
    else{
        view_info.viewType = vk::ImageViewType::e2DArray;;
    }
    view_info.image = image.image;
    view_info.format = image.image_format;
    view_info.subresourceRange = {
        aspect_flags,       // aspectMask
        0,                  // baseMipLevel
        image.mip_levels,   // levelCount
        0,                  // baseArrayLayer
        image.array_layers  // layerCount
    };

    return std::move(vk::raii::ImageView(logical_device, view_info));
}

vk::Format Image::findSupportedFormat(vk::raii::PhysicalDevice &physical_device, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (const auto format : candidates){
        vk::FormatProperties props = physical_device.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features){
            return format;
        }
        if(tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features){
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

vk::Format Image::findDepthFormat(vk::raii::PhysicalDevice &physical_device)
{
    return findSupportedFormat(physical_device,
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

void Image::transitionImageLayout(vk::Image &image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask,
                                    vk::ImageAspectFlagBits image_aspect, vk::raii::CommandBuffer &command_buffer)
{
    vk::ImageMemoryBarrier2 barrier{};
    barrier.srcStageMask = src_stage_mask;
    barrier.dstStageMask = dst_stage_mask;
    barrier.srcAccessMask = src_access_mask;
    barrier.dstAccessMask = dst_access_mask;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    // TODO - FIX THIS FOR MULTISAMPLING
    barrier.subresourceRange.aspectMask = image_aspect;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::DependencyInfo dependency_info{};
    dependency_info.dependencyFlags = {};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &barrier;
	
    command_buffer.pipelineBarrier2(dependency_info);
}
