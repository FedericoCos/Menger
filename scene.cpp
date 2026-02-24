#include "scene.hpp"

void Scene::initialSetup(){
    target_fps = 40;
    original_size = cube_size; // For displacement calculations

    // Reserving memory for all the vectors in scene, so no reallocation is needded
    cube_positions.resize(MAX_CUBES);
    temp_positions.resize(MAX_CUBES);
    positions.resize(MAX_CUBES);

    centers_and_levels.resize(MAX_LIGHTS);
    pointlight_buffers.resize(MAX_LIGHTS);
    light_indices_size.resize(MAX_CUBES + 1);
    light_indices.resize(MAX_CONNECTIONS * MAX_CUBES);

    // Setting up the main cube
    main_cube = Cube(center, glm::vec3(cube_size), glm::vec3(0.0f), glm::vec3(0.0f), rot_speed, glm::vec3(0.0), center, true);
    main_cube.start(vma_allocator, logical_device, queue_pool);
    cube_positions[0] = center;

    // Setting up the camera
    c_camera = C_camera(glm::vec3(0, 0, 2), 0.1f, 0.01f, 0.001f, 0.48f, 0.f, glm::vec3(0, 1, 0), -90.f, 0.f, 75.f);

}

void Scene::initCubeResources(){
    /**
     * Cube ssbo holds all the positions of the cubes in the scene. 
     * The mapped SSBO is made for easy write from the CPU, and the one not mapped is used for fast access by the GPU
     */
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

    /**
     * Uniform Buffer of the main cube, to store rotation and scale information that are shared across all cubes
     */
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
}

void Scene::initLightResources(){
    /**
     * The light ssbo is used to store all the info about the lights that are needed from the fragment shader
     * It also tells how many pointlights are currently in the scene
     * The mapped ssbo is for writing by the CPU
     * The non mapped version is for fast access from the GPU
     */
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

    /**
     * This SSBO holds, for each cube, the indices of the light that have a meaningful influence over the final color
     * of the cube itself
     */
    light_indices_ssbo_mapped.clear();
    light_indices_ssbo_mapped.resize(queue_pool.max_frames_in_flight);
    vk::DeviceSize light_indices_device_size = sizeof(uint32_t) * (MAX_CONNECTIONS * MAX_CUBES);
    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        light_indices_ssbo_mapped[i].buffer = Device::createBuffer(
            light_indices_device_size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            "Lights indices SSBO mapped",
            vma_allocator
        );
        vmaMapMemory(vma_allocator, light_indices_ssbo_mapped[i].buffer.allocation, &light_indices_ssbo_mapped[i].data);
    }

    light_indices_ssbo.clear();
    light_indices_ssbo.resize(queue_pool.max_frames_in_flight);
    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        light_indices_ssbo[i].buffer = Device::createBuffer(
            light_indices_device_size,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            "Lights indices SSBO",
            vma_allocator
        );
    }

    /**
     * This SSBO is tightly connected to the previous one
     * It stores a counter of how many connections there were from the first cube to the last
     * Thus, to get the total lights influencing a cube, you just need to subtract the value in the array immediately after to 
     * the one referring to the cube itself
     */
    light_indices_size_ssbo_mapped.clear();
    light_indices_size_ssbo_mapped.resize(queue_pool.max_frames_in_flight);
    vk::DeviceSize light_indices_size_device_size = sizeof(uint32_t) * (MAX_CUBES + 1);
    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        light_indices_size_ssbo_mapped[i].buffer = Device::createBuffer(
            light_indices_size_device_size,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            "Lights indices size SSBO mapped",
            vma_allocator
        );
        vmaMapMemory(vma_allocator, light_indices_size_ssbo_mapped[i].buffer.allocation, &light_indices_size_ssbo_mapped[i].data);
    }

    light_indices_size_ssbo.clear();
    light_indices_size_ssbo.resize(queue_pool.max_frames_in_flight);
    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        light_indices_size_ssbo[i].buffer = Device::createBuffer(
            light_indices_size_device_size,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            "Lights indices size SSBO",
            vma_allocator
        );
    }
}

void Scene::initCameraResources(){
    /**
     * A very standard UBO to hold projection and view matrices
     */
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
}

void Scene::createInitResources(){
    initialSetup();

    initCubeResources();

    initLightResources();

    initCameraResources();

    // PIPELINE SETUP
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

        // Binding 4: Pointlights indices SSBO
        vk::DescriptorSetLayoutBinding(
            4,
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eFragment,
            nullptr
        ),

        // Binding 5: Pointlights indices size SSBO
        vk::DescriptorSetLayoutBinding(
            5,
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eFragment,
            nullptr
        ),

    };
    std::string name = "cubes pipeline";
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
        &light_ssbo,
        &light_indices_ssbo,
        &light_indices_size_ssbo
    };
    Pipeline::writeDescriptorSets(raster_pipelines[0].descriptor_sets, bindings, resources, logical_device, queue_pool.max_frames_in_flight);

    mengerStep(); // performing the first step of menger
}

void Scene::updateUniformBuffers(float dtime, int current_frame)
{
    // update the camera and extract the projection and view matrix
    c_camera.update(dtime);

    UniformBufferCamera ubo_camera;
    ubo_camera.view = c_camera.getViewMatrix();
    ubo_camera.proj = c_camera.getProjectionMatrix(swapchain.extent.width * 1.f / swapchain.extent.height, n_plane, f_plane);

    memcpy(ubo_camera_mapped[current_frame].data, &ubo_camera, sizeof(UniformBufferCamera));

    // Writing info about the main cube
    FirstCubeBuffer first_cube;
    main_cube.update(dtime);
    first_cube.rotation_matrix = main_cube.getRotationMatrix();
    first_cube.center_and_scale = glm::vec4(main_cube.getCenterVector(), main_cube.getScaleFactor());
    memcpy(single_cube_ubo[current_frame].data, &first_cube, sizeof(FirstCubeBuffer));

    // Updating positions of the cubes if a new menger step took place
    if(dirty_positions < queue_pool.max_frames_in_flight){
        // Writing cubes
        dirty_positions++;
        for(size_t i = 0; i < current_cubes; i++){
            positions[i] = glm::vec4(cube_positions[i] - center, 1.0f);
        }

        memcpy(cube_ssbo_mapped[current_frame].data, positions.data(), current_cubes * sizeof(glm::vec4));

        Device::copyBuffer(cube_ssbo_mapped[current_frame].buffer, cube_ssbo[current_frame].buffer, current_cubes * sizeof(glm::vec4), logical_device, queue_pool, 0);

        // Writing lights
        if(current_pointlights > 0){
            for(size_t i = 0; i < current_pointlights; i++){
                PointLightBuffer buf;
                buf.color = glm::vec4(light_colors[centers_and_levels[i].w - 1], base_light_intensity / (centers_and_levels[i].w > 1 ? std::pow(intensity_divisor, centers_and_levels[i].w - 1) : 1));
                buf.position = glm::vec4(glm::vec3(centers_and_levels[i]), buf.color.w / light_threshold);

                pointlight_buffers[i] = buf;
            }

            glm::vec4 num(current_pointlights, 0, 0, 0);
            memcpy(light_ssbo_mapped[current_frame].data, &num, sizeof(glm::vec4));
            memcpy(static_cast<char*>(light_ssbo_mapped[current_frame].data) + sizeof(glm::vec4), pointlight_buffers.data(), current_pointlights * sizeof(PointLightBuffer));

            Device::copyBuffer(light_ssbo_mapped[current_frame].buffer, light_ssbo[current_frame].buffer, sizeof(glm::vec4) + current_pointlights * sizeof(PointLightBuffer), logical_device, queue_pool, 0);
        }

        if(current_pointlights > 0){
            memcpy(light_indices_size_ssbo_mapped[current_frame].data, light_indices_size.data(), sizeof(uint32_t) * (current_cubes + 1));
            Device::copyBuffer(light_indices_size_ssbo_mapped[current_frame].buffer, light_indices_size_ssbo[current_frame].buffer, sizeof(uint32_t) * (current_cubes + 1), logical_device, queue_pool, 0);

            memcpy(light_indices_ssbo_mapped[current_frame].data, light_indices.data(), sizeof(uint32_t) * (current_connections));
            Device::copyBuffer(light_indices_ssbo_mapped[current_frame].buffer, light_indices_ssbo[current_frame].buffer, sizeof(uint32_t) * (current_connections), logical_device, queue_pool, 0);
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

    vk::ClearValue  clear_color = vk::ClearColorValue(0.008f, 0.014f, 0.020f, 1.0f);

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
        command_buffer.bindVertexBuffers(0, main_cube.getVertexBuffer(), {0});
        command_buffer.bindIndexBuffer(main_cube.getIndexBuffer(), 0, vk::IndexType::eUint32);
        command_buffer.drawIndexed(main_cube.getIndexSize(), current_cubes, 0, 0, 0);
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
    if(inputs.count(GLFW_KEY_SPACE) && inputs[GLFW_KEY_SPACE] == InputState::PRESSED && current_menger_level < max_menger_level){
        mengerStep();
        inputs[GLFW_KEY_SPACE] = InputState::RELEASED;
    }

    if(inputs.count(GLFW_KEY_W) && (inputs[GLFW_KEY_W] == InputState::PRESSED || inputs[GLFW_KEY_W] == InputState::HOLD)){
        c_camera.processKeyboard(CameraMovement::FORWARD, time);
    }
    if(inputs.count(GLFW_KEY_S) && (inputs[GLFW_KEY_S] == InputState::PRESSED || inputs[GLFW_KEY_S] == InputState::HOLD)){
        c_camera.processKeyboard(CameraMovement::BACKWARD, time);
    }
    if(inputs.count(GLFW_KEY_A) && (inputs[GLFW_KEY_A] == InputState::PRESSED || inputs[GLFW_KEY_A] == InputState::HOLD)){
        c_camera.processKeyboard(CameraMovement::LEFT, time);
    }
    if(inputs.count(GLFW_KEY_D) && (inputs[GLFW_KEY_D] == InputState::PRESSED || inputs[GLFW_KEY_D] == InputState::HOLD)){
        c_camera.processKeyboard(CameraMovement::RIGHT, time);
    }

    if(inputs.count(GLFW_KEY_UP) && (inputs[GLFW_KEY_UP] == InputState::PRESSED || inputs[GLFW_KEY_UP] == InputState::HOLD)){
        c_camera.processKeyboard(CameraMovement::T_UP, time);
    }
    if(inputs.count(GLFW_KEY_DOWN) && (inputs[GLFW_KEY_DOWN] == InputState::PRESSED || inputs[GLFW_KEY_DOWN] == InputState::HOLD)){
        c_camera.processKeyboard(CameraMovement::T_DOWN, time);
    }
    if(inputs.count(GLFW_KEY_RIGHT) && (inputs[GLFW_KEY_RIGHT] == InputState::PRESSED || inputs[GLFW_KEY_RIGHT] == InputState::HOLD)){
        c_camera.processKeyboard(CameraMovement::T_RIGHT, time);
    }
    if(inputs.count(GLFW_KEY_LEFT) && (inputs[GLFW_KEY_LEFT] == InputState::PRESSED || inputs[GLFW_KEY_LEFT] == InputState::HOLD)){
        c_camera.processKeyboard(CameraMovement::T_LEFT, time);
    }
    if(inputs.count(GLFW_KEY_RIGHT_SHIFT) && (inputs[GLFW_KEY_RIGHT_SHIFT] == InputState::PRESSED || inputs[GLFW_KEY_RIGHT_SHIFT] == InputState::HOLD)){
        c_camera.processKeyboard(CameraMovement::T_ROLL_RIGHT, time);
    }
    if(inputs.count(GLFW_KEY_LEFT_SHIFT) && (inputs[GLFW_KEY_LEFT_SHIFT] == InputState::PRESSED || inputs[GLFW_KEY_LEFT_SHIFT] == InputState::HOLD)){
        c_camera.processKeyboard(CameraMovement::T_ROLL_LEFT, time);
    }

    if(inputs.count(GLFW_KEY_R) && (inputs[GLFW_KEY_R] == InputState::PRESSED || inputs[GLFW_KEY_R] == InputState::HOLD) && !c_camera.isAutomatic()){
        c_camera.createGrid(grid_positions, current_menger_level);
    }
}

void Scene::mengerStep()
{
    dirty_positions = 0;
    current_menger_level += 1;
    uint32_t dimension_step = std::pow(3, current_menger_level);
    cube_size /= 3.0;
    float cube_size_dim = cube_size - cube_size * 0.1;

    double start_offset = -(original_size / 2) + (cube_size / 2.0);
    
    uint32_t index = 0;

    std::copy_n(cube_positions.begin(), current_cubes, temp_positions.begin());

    /**
     * Copying to the camera grid positions the one that become available once you move to the next menger step
     */
    for(size_t i =0; i < free_positions.size(); i++){
        grid_positions.push_back(free_positions[i]);
    }
    free_positions.clear();

    for(size_t cube_ind = 0; cube_ind < current_cubes; cube_ind++){
        glm::vec3 &pos = temp_positions[cube_ind];
        for(size_t i = 0; i < 3; i++){ // x dimension
            for(size_t j = 0; j < 3; j++){ // y dimension
                for(size_t k = 0; k < 3; k++){ // z dimension
                    // Skipping empty cubes
                    if ((i == 1 && j == 1) || 
                        (i == 1 && k == 1) || 
                        (j == 1 && k == 1)){
                        glm::vec4 new_pos = {
                            pos.x + i * cube_size - cube_size,
                            pos.y + j * cube_size - cube_size,
                            pos.z + k * cube_size - cube_size,
                            current_menger_level // This is needed to extract the correct color for the light
                        };
                        if(i == 1 && j == 1 && k == 1 && current_pointlights < MAX_LIGHTS){
                            centers_and_levels[current_pointlights] = new_pos;
                            current_pointlights++;
                            /**
                             * Positions free for the camera
                             */
                            grid_positions.push_back(new_pos);
                        }
                        else{
                            /**
                             * Storing positions for the camera that become available once you move to the next menger step
                             */
                            free_positions.push_back(new_pos);
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

    main_cube.modifyCube(glm::vec3(start_offset, start_offset, -5.5 - (cube_size/2.0)), glm::vec3(cube_size_dim));
    current_cubes = index;

    std::cout << "Level: " << current_menger_level
              << " | Grid: " << dimension_step << "x" << dimension_step << "x" << dimension_step
              << " | Total cubes: " << index
              << " | Cube Size: " << cube_size << std::endl;

    connectLights();
}

void Scene::connectLights()
{
    std::vector<float> precomputed_radius(current_pointlights);
    for(size_t j = 0; j < current_pointlights; j++){
        float intensity = base_light_intensity / (centers_and_levels[j].w > 1 ? std::pow(intensity_divisor, centers_and_levels[j].w - 1) : 1.0f);
        precomputed_radius[j] = std::sqrt(intensity / light_threshold);
    }

    float cube_bounding_radius = 1.73205f * (cube_size / 2.0f);

    #pragma omp parallel for
    for(size_t i = 0; i < current_cubes; i++){
        uint16_t count = 0;
        glm::vec3 current_cube_pos = cube_positions[i]; 

        for(int j = current_pointlights - 1; j >= 0; j--){
            glm::vec3 diff = current_cube_pos - glm::vec3(centers_and_levels[j]);
            float dist = glm::length(diff);

            if(dist - cube_bounding_radius <= precomputed_radius[j] && count < MAX_CONNECTIONS){
                light_indices[count + (i * MAX_CONNECTIONS)] = j;
                count++;
            }
        }
        light_indices_size[i] = count;
    }

    size_t current_ind = 0;
    
    for(size_t i = 0; i < current_cubes; i++){
        size_t temp_count = light_indices_size[i];
        
        light_indices_size[i] = current_ind; 

        size_t start_ind = i * MAX_CONNECTIONS;

        for(size_t j = start_ind; j < start_ind + temp_count; j++){
            light_indices[current_ind] = light_indices[j];
            current_ind++;
        }
    }

    light_indices_size[current_cubes] = current_ind;
    current_connections = current_ind;
}

void Scene::cleanup(){
    main_cube = Cube();
    single_cube_ubo.clear();
    cube_ssbo_mapped.clear();
    cube_ssbo.clear();
    light_ssbo_mapped.clear();
    light_ssbo.clear();

    light_indices_size_ssbo.clear();
    light_indices_size_ssbo_mapped.clear();
    light_indices_ssbo.clear();
    light_indices_ssbo_mapped.clear();

    Engine::cleanup();
}