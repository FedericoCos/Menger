#pragma once

#include "../Helpers/GeneralLibraries.hpp"

namespace Image{
    // Creates an image along with its components (doesn't generate the image view)
    AllocatedImage createImage(uint32_t width, uint32_t height, vk::ImageType image_type, uint32_t mip_levels, 
            vk::SampleCountFlagBits msaa_samples, vk::Format format, uint32_t array_layers,
            vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
            std::string image_name, vk::ImageCreateFlags flags, VmaAllocator &vma_allocator);

    
    // Creates the image view for a specific ALlocated Image
    vk::raii::ImageView createImageView(AllocatedImage& image, vk::raii::Device &logical_device);

    // Helper function to find supported formats
    vk::Format findSupportedFormat(vk::raii::PhysicalDevice &physical_device, const std::vector<vk::Format>& candidates, 
            vk::ImageTiling tiling, vk::FormatFeatureFlags features);

    // Helper function to find supported depth formats
    vk::Format findDepthFormat(vk::raii::PhysicalDevice &physical_device);

    // Helper function to transition images from one stage mask to another
    void transitionImageLayout(
	    vk::Image               &image,
	    vk::ImageLayout         old_layout,
	    vk::ImageLayout         new_layout,
	    vk::AccessFlags2        src_access_mask,
	    vk::AccessFlags2        dst_access_mask,
	    vk::PipelineStageFlags2 src_stage_mask,
	    vk::PipelineStageFlags2 dst_stage_mask,
            vk::raii::CommandBuffer &command_buffer
        );


}