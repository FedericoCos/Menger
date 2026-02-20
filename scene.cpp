#include "scene.hpp"

void Scene::createInitResources(){
    original_size = cube_size; // For displacement calculations
    rot_speed = glm::vec3(0);
    base_light_intensity = 500000.f;
    intensity_divisor = 10;
    light_threshold = 0.1;

    // Reserving memory for all cubes, instantiating only for one
    cube_positions.resize(MAX_CUBES);
    temp_positions.resize(MAX_CUBES);
    centers_and_levels.resize(MAX_LIGHTS);

    main_cube = Cube(center, glm::vec3(cube_size), glm::vec3(0.0f), glm::vec3(0.0f), rot_speed, glm::vec3(0.0), center, true);
    main_cube.start(vma_allocator, logical_device, queue_pool);

    cube_positions[0] = center;

    // The UBO containing the per-cube info
    cube_ssbo_mapped.clear();
    cube_ssbo_mapped.resize(queue_pool.max_frames_in_flight);
    vk::DeviceSize gameobject_buffer_size = sizeof(CubeBuffer);
    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        cube_ssbo_mapped[i].buffer = Device::createBuffer(
            sizeof(glm::vec4) * MAX_CUBES,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            "Gameobject SSBO Mapped",
            vma_allocator
        );
        vmaMapMemory(vma_allocator, cube_ssbo_mapped[i].buffer.allocation, &cube_ssbo_mapped[i].data);
    }

    cube_ssbo.clear();
    cube_ssbo.resize(queue_pool.max_frames_in_flight);
    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        cube_ssbo[i].buffer = Device::createBuffer(
            sizeof(glm::vec4) * MAX_CUBES,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            "Gameobject SSBO",
            vma_allocator
        );
    }

    single_cube_ubo.clear();
    single_cube_ubo.resize(queue_pool.max_frames_in_flight);
    vk::DeviceSize single_cubo_ubo_size = sizeof(FirstCubeBuffer);
    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        single_cube_ubo[i].buffer = Device::createBuffer(
            single_cubo_ubo_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            "Main Cube Buffer",
            vma_allocator
        );
        vmaMapMemory(vma_allocator, single_cube_ubo[i].buffer.allocation, &single_cube_ubo[i].data);
    }

    // LIGHTS SETUP
    light_ssbo_mapped.clear();
    light_ssbo_mapped.resize(queue_pool.max_frames_in_flight);
    vk::DeviceSize light_size = sizeof(glm::vec4) + sizeof(PointLightBuffer) * MAX_LIGHTS;
    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        light_ssbo_mapped[i].buffer = Device::createBuffer(
            light_size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            "Lights SSBO mapped",
            vma_allocator
        );
        vmaMapMemory(vma_allocator, light_ssbo_mapped[i].buffer.allocation, &light_ssbo_mapped[i].data);
    }

    light_ssbo.clear();
    light_ssbo.resize(queue_pool.max_frames_in_flight);
    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        light_ssbo[i].buffer = Device::createBuffer(
            light_size,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            "Lights SSBO",
            vma_allocator
        );
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

    camera = Camera(glm::vec3(0, 0, 2), 0.1f);



    const std::string vertex_shader_path = "Shaders/Menger/vertex.vert.spv";
    const std::string fragment_shader_path = "Shaders/Menger/fragment.frag.spv";

    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        // Binding 0: Camera Uniform Buffer
        vk::DescriptorSetLayoutBinding(
            0, // binding location
            vk::DescriptorType::eUniformBuffer, // Type of binding
            1, // binding count
            vk::ShaderStageFlagBits::eVertex,
            nullptr
        ),

        // Binding 1: Cubes SSBO
        vk::DescriptorSetLayoutBinding(
            1,
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex,
            nullptr
        ),

        // Binding 2: main cube uniform buffer
        vk::DescriptorSetLayoutBinding(
            2,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex,
            nullptr
        ),

        // Binding 3: Pointlights SSBO
        vk::DescriptorSetLayoutBinding(
            3,
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eFragment,
            nullptr
        ),

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
        &cube_ssbo,
        &single_cube_ubo,
        &light_ssbo
    };
    Pipeline::writeDescriptorSets(raster_pipelines[0].descriptor_sets, bindings, resources, logical_device, queue_pool.max_frames_in_flight);
    
    pip_to_obj[&raster_pipelines[0]] = std::vector<Gameobject*>();
    pip_to_obj[&raster_pipelines[0]].push_back(&main_cube);
}

void Scene::updateUniformBuffers(float dtime, int current_frame)
{
    UniformBufferCamera ubo_camera;

    ubo_camera.view = camera.getViewMatrix();
    ubo_camera.proj = camera.getProjectionMatrix(swapchain.extent.width * 1.f / swapchain.extent.height, n_plane, f_plane);

    memcpy(ubo_camera_mapped[current_frame].data, &ubo_camera, sizeof(UniformBufferCamera));

    FirstCubeBuffer first_cube;
    main_cube.update(dtime);
    first_cube.rotation_matrix = main_cube.getRotationMatrix();
    first_cube.center_and_scale = glm::vec4(main_cube.getCenterVector(), main_cube.getScaleFactor());

    memcpy(single_cube_ubo[current_frame].data, &first_cube, sizeof(FirstCubeBuffer));

    if(dirty_positions < queue_pool.max_frames_in_flight){
        // Writing cubes
        dirty_positions++;
        if(positions.size() < current_cubes){
            positions.resize(current_cubes);
        }
        positions[0] = glm::vec4(cube_positions[0] - center, 1.0f);
        size_t i = 1;
        for(; i < current_cubes; i++){
            positions[i] = glm::vec4(cube_positions[i] - center, 1.0f);
        }

        memcpy(cube_ssbo_mapped[current_frame].data, positions.data(), current_cubes * sizeof(glm::vec4));

        Device::copyBuffer(cube_ssbo_mapped[current_frame].buffer, cube_ssbo[current_frame].buffer, current_cubes * sizeof(glm::vec4), logical_device, queue_pool, 0);

        // Writing lights
        if(pointlight_buffers.size() < current_pointlights){
            pointlight_buffers.resize(current_pointlights);
        }

        if(current_pointlights > 0 && current_menger_step < 5){
            for(i = 0; i < current_pointlights; i++){
                PointLightBuffer buf;
                buf.color = glm::vec4(light_colors[centers_and_levels[i].w], base_light_intensity / (centers_and_levels[i].w > 0 ? std::pow(intensity_divisor, centers_and_levels[i].w) : 1));
                buf.position = glm::vec4(glm::vec3(centers_and_levels[i]), buf.color.w / light_threshold);

                pointlight_buffers[i] = buf;
            }

            glm::vec4 num(current_pointlights, 0, 0, 0);
            memcpy(light_ssbo_mapped[current_frame].data, &num, sizeof(glm::vec4));
            memcpy(static_cast<char*>(light_ssbo_mapped[current_frame].data) + sizeof(glm::vec4), pointlight_buffers.data(), current_pointlights * sizeof(PointLightBuffer));

            Device::copyBuffer(light_ssbo_mapped[current_frame].buffer, light_ssbo[current_frame].buffer, sizeof(glm::vec4) + current_pointlights * sizeof(PointLightBuffer), logical_device, queue_pool, 0);
        }
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
            vk::ImageAspectFlagBits::eColor,
            command_buffer
    );

    Image::transitionImageLayout(
        depth_image.image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        {},                                               // srcAccessMask
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite, // dstAccessMask
        vk::PipelineStageFlagBits2::eTopOfPipe,            // srcStage
        vk::PipelineStageFlagBits2::eEarlyFragmentTests,   // dstStage (Where depth starts)
        vk::ImageAspectFlagBits::eDepth,
        command_buffer
    );

    vk::ClearValue  clear_color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

    vk::RenderingAttachmentInfo attachment_info{};
    attachment_info.imageView = swapchain.image_views[image_index];
    attachment_info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachment_info.loadOp = vk::AttachmentLoadOp::eClear;
    attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
    attachment_info.clearValue = clear_color;

    vk::ClearValue depth_clear_value;
    depth_clear_value.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
    vk::RenderingAttachmentInfo depth_attachment_info{};
    depth_attachment_info.imageView = *depth_image.image_view; 
    depth_attachment_info.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depth_attachment_info.loadOp = vk::AttachmentLoadOp::eClear;
    depth_attachment_info.storeOp = vk::AttachmentStoreOp::eDontCare;
    depth_attachment_info.clearValue = depth_clear_value;

    vk::RenderingInfo rendering_info{};
    rendering_info.renderArea.offset = vk::Offset2D{0, 0};
    rendering_info.renderArea.extent = swapchain.extent;
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &attachment_info;
    rendering_info.pDepthAttachment = &depth_attachment_info;

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
        command_buffer.bindVertexBuffers(0, pip_to_obj[&raster_pipelines[i]][0] -> getVertexBuffer(), {0});
        command_buffer.bindIndexBuffer(pip_to_obj[&raster_pipelines[i]][0] -> getIndexBuffer(), 0, vk::IndexType::eUint32);
        command_buffer.drawIndexed(pip_to_obj[&raster_pipelines[i]][0] -> getIndexSize(), current_cubes, 0, 0, 0);
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
        vk::ImageAspectFlagBits::eColor,
        command_buffer
    );
    command_buffer.end();

    glfwSetWindowTitle(window, std::to_string(1000.0/time).c_str());

}

void Scene::processInput()
{
    if(inputs.count(GLFW_KEY_SPACE) && inputs[GLFW_KEY_SPACE] == InputState::PRESSED){
        mengerStep();
        inputs[GLFW_KEY_SPACE] = InputState::RELEASED;
    }

    if(inputs.count(GLFW_KEY_W) && (inputs[GLFW_KEY_W] == InputState::PRESSED || inputs[GLFW_KEY_W] == InputState::HOLD)){
        camera.processKeyboard(CameraMovement::FORWARD, time);
    }
    if(inputs.count(GLFW_KEY_S) && (inputs[GLFW_KEY_S] == InputState::PRESSED || inputs[GLFW_KEY_S] == InputState::HOLD)){
        camera.processKeyboard(CameraMovement::BACKWARD, time);
    }
    if(inputs.count(GLFW_KEY_A) && (inputs[GLFW_KEY_A] == InputState::PRESSED || inputs[GLFW_KEY_A] == InputState::HOLD)){
        camera.processKeyboard(CameraMovement::LEFT, time);
    }
    if(inputs.count(GLFW_KEY_D) && (inputs[GLFW_KEY_D] == InputState::PRESSED || inputs[GLFW_KEY_D] == InputState::HOLD)){
        camera.processKeyboard(CameraMovement::RIGHT, time);
    }
}

void Scene::mengerStep()
{
    dirty_positions = 0;
    uint32_t dimension_step = std::pow(3, current_menger_step);
    cube_size /= 3.0;
    uint32_t new_cube_tot = std::pow(20, current_menger_step);
    current_menger_step += 1;
    float cube_size_dim = cube_size - cube_size * 0.05;

    std::cout << "Step: " << current_menger_step
              << " | Grid: " << dimension_step << "x" << dimension_step 
              << " | Total cubes: " << new_cube_tot
              << " | Cube Size: " << cube_size << std::endl;

    double start_offset = -(original_size / 2) + (cube_size / 2.0);
    
    uint32_t index = 0;

    std::copy_n(cube_positions.begin(), current_cubes, temp_positions.begin());

    for(size_t cube_ind = 0; cube_ind < current_cubes; cube_ind++){
        glm::vec3 &pos = temp_positions[cube_ind];
        for(size_t i = 0; i < 3; i++){ // x dimension
            for(size_t j = 0; j < 3; j++){ // y dimension
                for(size_t k = 0; k < 3; k++){ // z dimension
                    // Skipping empty cubes
                    if ((i == 1 && j == 1) || 
                        (i == 1 && k == 1) || 
                        (j == 1 && k == 1)){
                        if(i == 1 && j == 1 && k == 1){
                            centers_and_levels[current_pointlights] = glm::vec4(
                                pos.x + i * cube_size - cube_size,
                                pos.y + j * cube_size - cube_size,
                                pos.z + k * cube_size - cube_size,
                                current_menger_step - 2 // This is needed to extract the correct color for the light
                            );
                            current_pointlights++;
                        }

                        continue;
                    }

                    glm::vec3 new_pos(
                        pos.x + i * cube_size - cube_size,
                        pos.y + j * cube_size - cube_size,
                        pos.z + k * cube_size - cube_size
                    );

                    cube_positions[index] = new_pos;
                    index++;
                }
            }
        }
    }


    if(index != new_cube_tot){
        std::cout << "ERROR! calculated cubes: " << new_cube_tot << " Actual cubes: " << index << std::endl;
    }
    main_cube.modifyCube(glm::vec3(start_offset, start_offset, -5.5 - (cube_size/2.0)), glm::vec3(cube_size_dim));
    current_cubes = index;
}


void Scene::cleanup(){
    main_cube = Cube();
    single_cube_ubo.clear();
    cube_ssbo_mapped.clear();
    cube_ssbo.clear();
    light_ssbo_mapped.clear();
    light_ssbo.clear();

    Engine::cleanup();
}