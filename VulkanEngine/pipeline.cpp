#include "pipeline.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

RasterPipelineBundle Pipeline::createsRasterPipeline(const std::string &v_shader_path, const std::string &f_shader_path, 
                                std::vector<vk::DescriptorSetLayoutBinding> *bindings, vk::CullModeFlags cull_mode, 
                                vk::Format color_format, vk::Format depth_format, vk::SampleCountFlagBits &msaa_samples,
                                 std::string &name, vk::raii::DescriptorSetLayout *descriptor_set_layout,
                                vk::raii::Device &logical_device)
{
    RasterPipelineBundle pipeline_bundle;
    pipeline_bundle.pipeline_name = name;
    pipeline_bundle.v_shader_path = v_shader_path;
    pipeline_bundle.f_shader_path = f_shader_path;
    pipeline_bundle.cull_mode = cull_mode;
    pipeline_bundle.color_format = color_format;
    pipeline_bundle.depth_format = depth_format;
    pipeline_bundle.msaa_samples = msaa_samples;
    pipeline_bundle.topology = vk::PrimitiveTopology::eTriangleList;
    pipeline_bundle.polygon_mode = vk::PolygonMode::eFill;
    pipeline_bundle.front_face = vk::FrontFace::eCounterClockwise;

    std::cout << "Creating Raster pipeline. Name: " << pipeline_bundle.pipeline_name << std::endl;

    // Defining the shader associated with the pipeline
    pipeline_bundle.v_shader = createShaderModule(readFile(v_shader_path), logical_device);
    pipeline_bundle.f_shader = createShaderModule(readFile(f_shader_path), logical_device);

    vk::PipelineShaderStageCreateInfo vert_shader_stage_info;
    vert_shader_stage_info.stage = vk::ShaderStageFlagBits::eVertex;
    vert_shader_stage_info.module = *pipeline_bundle.v_shader;
    vert_shader_stage_info.pName = "main";

    vk::PipelineShaderStageCreateInfo frag_shader_stage_info;
    frag_shader_stage_info.stage = vk::ShaderStageFlagBits::eFragment;
    frag_shader_stage_info.module = *pipeline_bundle.f_shader;
    frag_shader_stage_info.pName = "main";

    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        vert_shader_stage_info, frag_shader_stage_info
    };

    // Creating the descriptor set layout
    if(descriptor_set_layout != nullptr){
        pipeline_bundle.descriptor_set_layout = std::move(*descriptor_set_layout);
    }
    else if(bindings != nullptr && (*bindings).size() > 0){
        pipeline_bundle.descriptor_set_layout = createDescriptorSetLayout(*bindings, logical_device);
    }
    else{
        std::cout << "Empty bindings! This might be an error!" << std::endl;
        pipeline_bundle.descriptor_set_layout = createDescriptorSetLayout(*bindings, logical_device);
    }

    // Vertex components
    vk::VertexInputBindingDescription binding_description = Vertex::getBindingDescription();
    auto attribute_descriptions = Vertex::getAttributeDescriptions(); // Here auto since probably the array will change in future, it is just safer

    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description; 
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size()); // Good practice to cast
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data(); 


    // Topology of the pipeline (how to group the vertices)
    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    input_assembly.topology = vk::PrimitiveTopology::eTriangleList;

    // Viewport information
    vk::PipelineViewportStateCreateInfo viewport_state;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    // Rasterizer main info
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = vk::False; // If true, pixels outside the viewing frustum will be smashed on the far/near plane
    rasterizer.rasterizerDiscardEnable = vk::False; // When ture, pipeline stops after vertex shader. USed for calculation with no rendering
    rasterizer.cullMode = pipeline_bundle.cull_mode;
    rasterizer.polygonMode = pipeline_bundle.polygon_mode;
    rasterizer.frontFace = pipeline_bundle.front_face;
    rasterizer.depthBiasEnable = vk::False; // Used to slightly offset depth value of pixels. Primarly used to fix "shadow acne"
    rasterizer.lineWidth = 1.f; // Only relevant if drawing lines or using eLine mode. Required wideLines GPU feature

    // Multisampling info
    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.rasterizationSamples = pipeline_bundle.msaa_samples;
    multisampling.sampleShadingEnable = vk::False;

    // Depth and stencil setup
    vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.depthTestEnable = vk::True;
    depth_stencil.depthWriteEnable = vk::True;
    depth_stencil.depthBoundsTestEnable = vk::False;
    depth_stencil.stencilTestEnable = vk::False;
    depth_stencil.depthCompareOp = vk::CompareOp::eLess;


    // Color attachment info, defines how to write on the color image
    vk::PipelineColorBlendAttachmentState color_blend_attachment;
    vk::PipelineColorBlendStateCreateInfo color_blending;
    color_blending.logicOpEnable = vk::False;
    color_blend_attachment.blendEnable = vk::False;
    color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    // Dynamic states
    std::vector dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
        vk::DynamicState::eCullMode
    };
    vk::PipelineDynamicStateCreateInfo dynamic_state;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();


    // Layout create info
    vk::PipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &*(pipeline_bundle.descriptor_set_layout);    
    pipeline_bundle.layout = vk::raii::PipelineLayout(logical_device, pipeline_layout_info);

    vk::PipelineRenderingCreateInfo pipeline_rendering_create_info;
    pipeline_rendering_create_info.colorAttachmentCount = 1;
    pipeline_rendering_create_info.pColorAttachmentFormats = &pipeline_bundle.color_format;
    pipeline_rendering_create_info.depthAttachmentFormat = pipeline_bundle.depth_format;


    vk::GraphicsPipelineCreateInfo pipeline_info;
    pipeline_info.pNext = &pipeline_rendering_create_info;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_bundle.layout;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pMultisampleState = &multisampling;

    pipeline_bundle.pipeline = vk::raii::Pipeline(logical_device, nullptr, pipeline_info);

    std::cout << "Created Pipeline:\n" << pipeline_bundle.to_str() << std::endl;


    return pipeline_bundle;

}

vk::raii::DescriptorSetLayout Pipeline::createDescriptorSetLayout(std::vector<vk::DescriptorSetLayoutBinding> &bindings, const vk::raii::Device &logical_device)
{
    vk::DescriptorSetLayoutCreateInfo layout_info({}, bindings.size(), bindings.data());

    return std::move(vk::raii::DescriptorSetLayout(logical_device, layout_info));
}

vk::raii::DescriptorPool Pipeline::createDescriptorPool(std::vector<vk::DescriptorSetLayoutBinding> &bindings, vk::raii::Device &logical_device, int max_frames_in_flight)
{
    std::vector<vk::DescriptorPoolSize> pool_sizes;

    int total_uniform_buffers = 0;
    for(size_t i = 0; i < bindings.size(); i++){
        if(bindings[i].descriptorType == vk::DescriptorType::eUniformBuffer){
            total_uniform_buffers += bindings[i].descriptorCount;
        }
    }

    if(total_uniform_buffers > 0){
        pool_sizes.push_back(vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, total_uniform_buffers));
    }

    vk::DescriptorPoolCreateInfo pool_info;
    pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    pool_info.maxSets = max_frames_in_flight;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size()); 
    pool_info.pPoolSizes = pool_sizes.data();

    return std::move(vk::raii::DescriptorPool(logical_device, pool_info));
}

std::vector<vk::raii::DescriptorSet> Pipeline::createDescriptorSets(vk::raii::DescriptorSetLayout &descriptor_set_layout, vk::raii::DescriptorPool &descriptor_pool, vk::raii::Device &logical_device, int max_frames_in_flight)
{
    std::vector<vk::raii::DescriptorSet> descriptor_sets;

    vk::DescriptorSetAllocateInfo alloc_info(*descriptor_pool, 1, &*descriptor_set_layout);

    for(size_t i = 0; i < max_frames_in_flight; i++){
        descriptor_sets.push_back(std::move(logical_device.allocateDescriptorSets(alloc_info).front()));
    }

    return std::move(descriptor_sets);
}

// TODO: Chenge from void * to a more safe option
void Pipeline::writeDescriptorSets(
    const std::vector<vk::raii::DescriptorSet> &descriptor_sets, 
    const std::vector<vk::DescriptorSetLayoutBinding> &bindings, 
    const std::vector<void *> &resources, 
    vk::raii::Device &logical_device, 
    const int max_frames_in_flight) 
{
    for (size_t i = 0; i < max_frames_in_flight; i++) {
        std::vector<vk::WriteDescriptorSet> writes;
        
        std::deque<std::vector<vk::DescriptorBufferInfo>> multi_buffer_infos;
        std::deque<vk::DescriptorBufferInfo> single_buffer_infos;

        for (size_t j = 0; j < bindings.size(); j++) {
            if (bindings[j].descriptorType == vk::DescriptorType::eUniformBuffer) {
                
                if (bindings[j].descriptorCount > 1) {
                    auto &info_vec = multi_buffer_infos.emplace_back();
                    info_vec.reserve(bindings[j].descriptorCount);

                    auto* res_ptr = static_cast<std::vector<std::vector<MappedUBO>>*>(resources[j]);
                    
                    for (size_t k = 0; k < bindings[j].descriptorCount; k++) {
                        AllocatedBuffer &buffer = (*res_ptr)[k][i].buffer;
                        info_vec.push_back(vk::DescriptorBufferInfo{buffer.buffer, 0, buffer.size});
                    }

                    writes.push_back(vk::WriteDescriptorSet{
                        *descriptor_sets[i], bindings[j].binding, 0,
                        bindings[j].descriptorCount, vk::DescriptorType::eUniformBuffer,
                        nullptr, info_vec.data(), nullptr
                    });
                } 
                else {
                    auto* res_ptr = static_cast<std::vector<MappedUBO>*>(resources[j]);
                    AllocatedBuffer &buffer = (*res_ptr)[i].buffer;
                    
                    vk::DescriptorBufferInfo &info = single_buffer_infos.emplace_back(
                        buffer.buffer, 0, buffer.size
                    );

                    writes.push_back(vk::WriteDescriptorSet{
                        *descriptor_sets[i], bindings[j].binding, 0,
                        1, vk::DescriptorType::eUniformBuffer,
                        nullptr, &info, nullptr
                    });
                }
            }
        }

        if (!writes.empty()) {
            logical_device.updateDescriptorSets(writes, nullptr);
        }

        multi_buffer_infos.clear();
        single_buffer_infos.clear();
    }
}

vk::raii::ShaderModule Pipeline::createShaderModule(const std::vector<char> &code, const vk::raii::Device &logical_device)
{
    vk::ShaderModuleCreateInfo create_info;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    vk::raii::ShaderModule shader_module{logical_device, create_info};
    return std::move(shader_module);
}

std::vector<char> Pipeline::readFile(const std::string& filename){
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if(!file.is_open()){
        throw std::runtime_error("failed to open file: " + filename);
    }
    size_t file_size = (size_t) file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();
    return buffer;
}
