#include "scene.hpp"

void Scene::createInitResources(){
    // Reserving memory for all cubes, instantiating only for one
    cubes.reserve(MAX_CUBES);
    cubes.push_back(Cube(glm::vec3(0, 0, -10), glm::vec3(cube_size), glm::vec3(-45.f, 45.f, 0.f), glm::vec3(0, 0, 0), glm::vec3(rot_speed, rot_speed, 0), glm::vec3(0.0), center, true));
    cubes[0].start(vma_allocator, logical_device, queue_pool);

    // The UBO containing the per-cube info
    ubo_objects_mapped.clear();
    ubo_objects_mapped.resize(MAX_CUBES);
    vk::DeviceSize gameobject_buffer_size = sizeof(CubeBuffer);
    for(size_t i = 0; i < MAX_CUBES; i++){
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

    single_cube_ubo.clear();
    single_cube_ubo.resize(queue_pool.max_frames_in_flight);
    vk::DeviceSize single_cubo_ubo_size = sizeof(FirstCubeBuffer);
    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        single_cube_ubo[i].buffer = Device::createBuffer(
            single_cubo_ubo_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            "Camera Buffer",
            vma_allocator
        );
        vmaMapMemory(vma_allocator, single_cube_ubo[i].buffer.allocation, &single_cube_ubo[i].data);
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



    const std::string vertex_shader_path = "Shaders/Menger/vertex.vert.spv";
    const std::string fragment_shader_path = "Shaders/Menger/fragment.frag.spv";

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
            MAX_CUBES,
            vk::ShaderStageFlagBits::eVertex,
            nullptr
        ),

        // Binding 2
        vk::DescriptorSetLayoutBinding(
            2,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex,
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
        &ubo_objects_mapped,
        &single_cube_ubo
    };
    Pipeline::writeDescriptorSets(raster_pipelines[0].descriptor_sets, bindings, resources, logical_device, queue_pool.max_frames_in_flight);
    
    pip_to_obj[&raster_pipelines[0]] = std::vector<Gameobject*>();
    pip_to_obj[&raster_pipelines[0]].reserve(cubes.size());
    for(size_t i = 0; i < cubes.size(); i++){
        pip_to_obj[&raster_pipelines[0]].push_back(&cubes[i]);
    }
}

void Scene::updateUniformBuffers(float dtime, int current_frame)
{
    UniformBufferCamera ubo_camera;

    ubo_camera.view = camera.getViewMatrix();
    ubo_camera.proj = camera.getProjectionMatrix(swapchain.extent.width * 1.f / swapchain.extent.height);

    memcpy(ubo_camera_mapped[current_frame].data, &ubo_camera, sizeof(UniformBufferCamera));

    FirstCubeBuffer first_cube;
    cubes[0].update(dtime);
    first_cube.rotation_matrix = cubes[0].getRotationMatrix();
    first_cube.scale_matrix = cubes[0].getScaleMatrix();
    first_cube.center_matrix = cubes[0].getCenterMatrix();

    memcpy(single_cube_ubo[current_frame].data, &first_cube, sizeof(FirstCubeBuffer));

    CubeBuffer ubo_obj;
    for(size_t i = 0; i < current_cubes; i++){
        // cubes[i].update(dtime);
        ubo_obj.position = glm::vec4(cubes[i].getPositionVector() - cubes[i].getCenterVector(), 1.0);

        memcpy(ubo_objects_mapped[i][current_frame].data, &ubo_obj, sizeof(CubeBuffer));
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
}

bool isMengerHole(size_t x, size_t y, size_t z) {
    while (x > 0 || y > 0 || z > 0) {
        if ((x % 3 == 1 && y % 3 == 1) || 
            (x % 3 == 1 && z % 3 == 1) || 
            (y % 3 == 1 && z % 3 == 1)) {
            return true;
        }
        x /= 3; y /= 3; z /= 3;
    }
    return false;
}

void Scene::mengerStep()
{
    uint32_t dimension_step = std::pow(3, current_menger_step);
    cube_size /= 3.0;
    uint32_t new_cube_tot = std::pow(20, current_menger_step);
    current_menger_step += 1;
    float cube_size_dim = cube_size - cube_size * 0.05;

    std::cout << "Step: " << current_menger_step 
              << " | Grid: " << dimension_step << "x" << dimension_step 
              << " | Total cubes: " << new_cube_tot
              << " | Cube Size: " << cube_size << std::endl;

    double start_offset = -4.5 + (cube_size / 2.0);
    double z_center_world = -10.0;
    
    uint32_t index = 0;

    glm::vec3 rot = cubes[0].getRotationVector();

    for(size_t i = 0; i < dimension_step; i++){ // x dimension
        for(size_t j = 0; j < dimension_step; j++){ // y dimension
            for(size_t k = 0; k < dimension_step; k++){ // z dimension
                if(isMengerHole(i, j, k)) {
                    continue; 
                }
                glm::vec3 pos(
                    start_offset + i * cube_size, 
                    start_offset + j * cube_size,
                    -5.5 - (cube_size/2.0) - k * cube_size
                );

                // Update or Create Cube
                if(index < current_cubes){
                    cubes[index].modifyCube(pos, glm::vec3(cube_size_dim));
                }
                else{
                    cubes.push_back(Cube(pos, glm::vec3(cube_size_dim), rot, glm::vec3(0.0), glm::vec3(rot_speed, rot_speed, 0.0f), glm::vec3(0.0), center, false));
                }
                index++;
            }
        }
    }
    if(index != new_cube_tot){
        std::cout << "ERROR! calculated cubes: " << new_cube_tot << " Actual cubes: " << index << std::endl;
    }
    current_cubes = new_cube_tot;
}


void Scene::cleanup(){
    cubes.clear();
    single_cube_ubo.clear();

    Engine::cleanup();
}