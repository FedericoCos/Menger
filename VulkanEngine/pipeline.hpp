#pragma once

#include "../Helpers/GeneralLibraries.hpp"

namespace Pipeline{
    // Creates a Raster Pipeline
    RasterPipelineBundle createsRasterPipeline(const std::string &v_shader_path, const std::string &f_shader_path, 
                                std::vector<vk::DescriptorSetLayoutBinding> *bindings, vk::CullModeFlags cull_mode, 
                                vk::Format color_format, vk::Format depth_format, vk::SampleCountFlagBits &msaa_samples,
                                std::string &name, vk::raii::DescriptorSetLayout *descriptor_set_layout,
                                vk::raii::Device &logical_device);

    // Creates the Descriptor Set Layout of a pipeline given its bindings
    vk::raii::DescriptorSetLayout createDescriptorSetLayout(std::vector<vk::DescriptorSetLayoutBinding> &bindings, const vk::raii::Device &logical_device);

    // Creates the Descriptor Pool starting from the binding information
    vk::raii::DescriptorPool createDescriptorPool(std::vector<vk::DescriptorSetLayoutBinding> &bindings, vk::raii::Device &logical_device, int max_frames_in_flight);

    // Creates the Descriptor Sets from given layout
    std::vector<vk::raii::DescriptorSet> createDescriptorSets(vk::raii::DescriptorSetLayout &descriptor_set_layout, vk::raii::DescriptorPool &descriptor_pool, vk::raii::Device &logical_device, int max_frames_in_flight);

    void writeDescriptorSets(const std::vector<vk::raii::DescriptorSet> &descriptor_sets, const std::vector<vk::DescriptorSetLayoutBinding> &bindings, const std::vector<void *> &resources, vk::raii::Device &logical_device, const int max_frames_in_flight);

    // Generates the shader module from the .spv files
    vk::raii::ShaderModule createShaderModule(const std::vector<char> &code, const vk::raii::Device &logical_device);

    // Reades a file
    std::vector<char> readFile(const std::string &filename);
}