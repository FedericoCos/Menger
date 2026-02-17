#include "scene.hpp"

void Scene::createInitResources(){

    int total_obj = 10;
    objects.reserve(total_obj);
    for(int i = 0; i < total_obj; i++){
        objects.push_back(Gameobject(glm::vec3(-(total_obj/2) + i, 0, -5), glm::vec3(1), glm::vec3(-45.f, 45.f, 0.f), glm::vec3(0, 0, 0), glm::vec3(.1f, .1f, 0)));
    }
    objects[0].start(vma_allocator, logical_device, queue_pool);

    ubo_objects_mapped.clear();
    ubo_objects_mapped.resize(total_obj);
    vk::DeviceSize gameobject_buffer_size = sizeof(UniformBufferGameObjects);
    for(size_t i = 0; i < total_obj; i++){
        ubo_objects_mapped[i].resize(queue_pool.max_frames_in_flight);
        for(size_t j = 0; j < queue_pool.max_frames_in_flight; j++){
            ubo_objects_mapped[i][j].buffer = Device::createBuffer(
                gameobject_buffer_size,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                "Gameobject Buffer",
                vma_allocator
            );
            vmaMapMemory(vma_allocator, ubo_objects_mapped[i][j].buffer.allocation, &ubo_objects_mapped[i][j].data);
        }
    }

    // CAMERA RESOURCES SETUP
    ubo_camera_mapped.clear();
    ubo_camera_mapped.resize(queue_pool.max_frames_in_flight);
    vk::DeviceSize camera_buffer_size = sizeof(UniformBufferCamera);

    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        ubo_camera_mapped[i].buffer = Device::createBuffer(
            camera_buffer_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            "Camera Buffer",
            vma_allocator
        );
        vmaMapMemory(vma_allocator, ubo_camera_mapped[i].buffer.allocation, &ubo_camera_mapped[i].data);
    }

    camera = Camera(glm::vec3(0, 0, 2));



    const std::string vertex_shader_path = "Shaders/Samples/vertex.vert.spv";
    const std::string fragment_shader_path = "Shaders/Samples/fragment.frag.spv";

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        // Binding 0: Camera Uniform Object
        vk::DescriptorSetLayoutBinding(
            0, // binding location
            vk::DescriptorType::eUniformBuffer, // Type of binding
            1, // binding count
            vk::ShaderStageFlagBits::eVertex,
            nullptr
        ),

        // Binding 1: GameObject Uniform Buffer
        vk::DescriptorSetLayoutBinding(
            1,
            vk::DescriptorType::eUniformBuffer,
            total_obj,
            vk::ShaderStageFlagBits::eVertex,
            nullptr
        )
    };
    std::string name = "dumb pipeline";
    raster_pipelines.push_back(Pipeline::createsRasterPipeline(vertex_shader_path, fragment_shader_path,
                                                    &bindings, vk::CullModeFlagBits::eBack, swapchain.format, 
                                                    Image::findDepthFormat(physical_device), msaa_samples, 
                                                    name, nullptr, logical_device
                                                    ));
    
    raster_pipelines[0].descriptor_pool = Pipeline::createDescriptorPool(bindings, logical_device, queue_pool.max_frames_in_flight);
    raster_pipelines[0].descriptor_sets = Pipeline::createDescriptorSets(raster_pipelines[0].descriptor_set_layout,
                                                                        raster_pipelines[0].descriptor_pool,
                                                                        logical_device,
                                                                        queue_pool.max_frames_in_flight);
    std::vector<void *> resources{
        &ubo_camera_mapped,
        &ubo_objects_mapped
    };
    Pipeline::writeDescriptorSets(raster_pipelines[0].descriptor_sets, bindings, resources, logical_device, queue_pool.max_frames_in_flight);

    
    pip_to_obj[&raster_pipelines[0]] = std::vector<Gameobject*>();
    pip_to_obj[&raster_pipelines[0]].reserve(objects.size());
    for(size_t i = 0; i < objects.size(); i++){
        pip_to_obj[&raster_pipelines[0]].push_back(&objects[i]);
    }
}

void Scene::updateUniformBuffers(float dtime, int current_frame)
{
    UniformBufferCamera ubo_camera;

    ubo_camera.view = camera.getViewMatrix();
    ubo_camera.proj = camera.getProjectionMatrix(swapchain.extent.width * 1.f / swapchain.extent.height);

    memcpy(ubo_camera_mapped[current_frame].data, &ubo_camera, sizeof(UniformBufferCamera));

    UniformBufferGameObjects ubo_obj;
    for(size_t i = 0; i < objects.size(); i++){
        objects[i].update(dtime);
        ubo_obj.model = objects[i].getModelMat();

        memcpy(ubo_objects_mapped[i][current_frame].data, &ubo_obj, sizeof(UniformBufferGameObjects));
    }
}

void Scene::recordCommandBuffer(uint32_t image_index)
{
    vk::raii::CommandBuffer &command_buffer = queue_pool.graphics_command_buffers[current_frame];
    command_buffer.begin({});

    Image::transitionImageLayout(swapchain.images[image_index], 
            vk::ImageLayout::eUndefined,
		    vk::ImageLayout::eColorAttachmentOptimal,
		    {},                                                        // srcAccessMask (no need to wait for previous operations)
		    vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // dstStage
            command_buffer
    );
    vk::ClearValue  clear_color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

    vk::RenderingAttachmentInfo attachment_info{};
    attachment_info.imageView = swapchain.image_views[image_index];
    attachment_info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachment_info.loadOp = vk::AttachmentLoadOp::eClear;
    attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
    attachment_info.clearValue = clear_color;

    vk::RenderingInfo rendering_info{};
    rendering_info.renderArea.offset = vk::Offset2D{0, 0};
    rendering_info.renderArea.extent = swapchain.extent;
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &attachment_info;

    command_buffer.beginRendering(rendering_info);
    if(raster_pipelines.size() <= 0){
        throw std::runtime_error("There are no raster pipelines that can be used!");
    }
    command_buffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchain.extent.width), static_cast<float>(swapchain.extent.height), 0.0f, 1.0f)); // What portion of the window to use
    command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchain.extent)); // What portion of the image to use
    for(size_t i = 0; i < raster_pipelines.size(); i++){
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *(raster_pipelines[i].pipeline));
        command_buffer.setCullMode(raster_pipelines[i].cull_mode);
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            raster_pipelines[i].layout,
            0,
            *raster_pipelines[i].descriptor_sets[current_frame],
            {}
        );
        UniformBufferGameObjects ubo_obj;
        command_buffer.bindVertexBuffers(0, pip_to_obj[&raster_pipelines[i]][0] -> getVertexBuffer(), {0});
        command_buffer.bindIndexBuffer(pip_to_obj[&raster_pipelines[i]][0] -> getIndexBuffer(), 0, vk::IndexType::eUint32);
        command_buffer.drawIndexed(pip_to_obj[&raster_pipelines[i]][0] -> getIndexSize(), objects.size(), 0, 0, 0);
    }
    command_buffer.endRendering();

    // After rendering, transition the swapchain image to PRESENT_SRC
    Image::transitionImageLayout(
        swapchain.images[image_index],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,                // srcAccessMask
        {},                                                        // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe,                 // dstStage
        command_buffer
    );
    command_buffer.end();

    glfwSetWindowTitle(window, std::to_string(1000.0/time).c_str());

}

void Scene::processInput()
{
    if(inputs.count(GLFW_KEY_SPACE) && inputs[GLFW_KEY_SPACE] == InputState::PRESSED){
        std::cout << "You would have jumped, if there were a jumping feature!" << std::endl;
        inputs[GLFW_KEY_SPACE] = InputState::RELEASED; // to indicate input ahs been consumed
    }
}