#include "engine.hpp"

// --- HELPER FUNCTIONS ---

// Gets GLFW extensions for Vulkan and necessary extensions for debugging
std::vector<const char *> Engine::getRequiredExtensions(){
    uint32_t glfw_extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if(enable_val_layers){
        extensions.push_back(vk::EXTDebugUtilsExtensionName); // debug messanger extension
    }

    return extensions;
}

void Engine::setupDebugMessanger(){
    if(!enable_val_layers){
        return;
    }

    vk::DebugUtilsMessageSeverityFlagsEXT severity_flags( vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError );
    vk::DebugUtilsMessageTypeFlagsEXT    message_type_flags( vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation );
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        {},
        severity_flags,
        message_type_flags,
        &debugCallback,
        nullptr
        };
    debug_messanger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}


// --- INITIALIZATION FUNCTIONS ---
void Engine::init(const std::string title, uint32_t &w, uint32_t &h)
{
    this -> title = title;
    this -> win_width = w;
    this -> win_height = h;

    initWindow();

    initVulkan();

    std::cout << "\nCOMPLETED INITIALIZATION" << std::endl;
}

// Initializes the window system using GLTF
void Engine::initWindow()
{
    std::cout << "\nWINDOW SETUP..." << std::endl;
    if(enable_val_layers){
        GLFWHelper::setErrorCallback();
    }
    window = GLFWHelper::initWindowGLFW(title.c_str(), win_width, win_height);

    glfwSetWindowUserPointer(window, &inputs);

    std::cout << "width: " << win_width << " height: " << win_height << std::endl;

    if(!window){
        throw std::runtime_error("Failed to create the GLFW window");
    }

    glfwSetKeyCallback(window, recordInput);
}

// Initialize all Vulkan Components
void Engine::initVulkan(){
    createInstance();
    setupDebugMessanger();
    createSurface();

    // Device setup
    std::cout << "\nDEVICE SETUP..." << std::endl;
    physical_device = Device::pickPhysicalDevice(instance);
    logical_device = Device::createLogicalDevice(physical_device, surface, queue_pool);

    queue_pool.graphics_queue = vk::raii::Queue(logical_device, queue_pool.graphics_family.value(), 0);
    queue_pool.transfer_queue = vk::raii::Queue(logical_device, queue_pool.transfer_family.value(), 0);
    queue_pool.present_queue = vk::raii::Queue(logical_device, queue_pool.present_family.value(), 0);
    if(!queue_pool.isQueueComplete()){
        throw std::runtime_error("Error during creation of queues!");
    }

    /**
     * eResetCommandBuffer -> allows to reset individual command buffers allocated from this pool without having to reset the entire pool at once
     * eTransient -> signals to the driver that the command buffers allocated here will live for a short time
     */
    queue_pool.graphics_command_pool = Device::createCommandPool(logical_device, vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_pool.graphics_family.value());
    queue_pool.transfer_command_pool = Device::createCommandPool(logical_device, vk::CommandPoolCreateFlagBits::eTransient, queue_pool.transfer_family.value());
    if(!queue_pool.isPoolComplete()){
        throw std::runtime_error("Error during creation of command pools!");
    }

    queue_pool.graphics_command_buffers = Device::createCommandBuffer(queue_pool.graphics_command_pool, vk::CommandBufferLevel::ePrimary, queue_pool.max_frames_in_flight, logical_device);
    
    // Swapchain setup
    std::cout << "\nSWAPCHAIN SETUP..." << std::endl;
    swapchain = Swapchain::createSwapchain(physical_device, logical_device, surface, window, queue_pool);

    // Memory Allocator setup
    std::cout << "\nMEMORY ALLOCATOR SETUP..." << std::endl;
    vma_allocator = MemoryAllocator::createMemoryAllocator(physical_device, logical_device, instance);

    // Color Image setup
    std::cout << "\nCOLOR IMAGE SETUP..." << std::endl;
    color_image = Image::createImage(swapchain.extent.width, swapchain.extent.height, vk::ImageType::e2D,
                                    1, msaa_samples, swapchain.format, 1, vk::ImageTiling::eOptimal,
                                vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, 
                                "color image", {}, vma_allocator);
    color_image.image_view = Image::createImageView(color_image, logical_device);

    depth_image = Image::createImage(swapchain.extent.width, swapchain.extent.height, vk::ImageType::e2D,
                                    1, msaa_samples, Image::findDepthFormat(physical_device), 1,
                                    vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal, "depth image", {}, vma_allocator);
    depth_image.image_view = Image::createImageView(depth_image, logical_device);

    // Pipeline Setup
    std::cout << "\nGENERAL SCENE RESOURCES SETUP..." << std::endl;
    createInitResources();

    // Synchronization objects Setup
    std::cout << "\nSYNCHRONIZATION OBJECTS SETUP..." << std::endl;
    createSyncObjects();

    prev_time = std::chrono::high_resolution_clock::now(); // To keep track of time


}

// Initialize a Vulkan Instance
void Engine::createInstance(){
    constexpr vk::ApplicationInfo app_info{
        "Engine",
        VK_MAKE_VERSION(1, 0, 0),
        "Engine Title",
        VK_MAKE_VERSION(1, 0, 0),
        vk::ApiVersion14 // API Version
    };

    // Get the required validation layers
    std::vector<char const *> required_layers;
    if(enable_val_layers){
        required_layers.assign(validation_layers.begin(), validation_layers.end());
    }

    // Check if the required layes are supported by the Vukan implementation
    auto layer_properties = context.enumerateInstanceLayerProperties();
    if (std::ranges::any_of(required_layers, [&layer_properties](auto const& required_layer) {
        return std::ranges::none_of(layer_properties,
                                   [required_layer](auto const& layerProperty)
                                   { return strcmp(layerProperty.layerName, required_layer) == 0; });
    }))
    {
        throw std::runtime_error("One or more required layers are not supported!");
    }

    // Get the required instance extensions from GLFW
    auto extensions = getRequiredExtensions();
    auto extensions_properties = context.enumerateInstanceExtensionProperties();
    for(auto const & required_ext : extensions){
        if(std::ranges::none_of(extensions_properties,
        [required_ext](auto const& extension_property){
            return strcmp(extension_property.extensionName, required_ext) == 0;
        })){
            throw std::runtime_error("Required extension not supported: " + std::string(required_ext));
        }
    }

    vk::InstanceCreateInfo createInfo(
        {},                              // flags
        &app_info,                        // application info
        static_cast<uint32_t>(required_layers.size()), // Size of validation layers
        required_layers.data(), // the actual validation layers
        static_cast<uint32_t>(extensions.size()),              // enabled extension count
        extensions.data()                   // enabled extension names
    );

    instance = vk::raii::Instance(context, createInfo);
    
}


void Engine::createSurface(){
    VkSurfaceKHR surface;
    if(glfwCreateWindowSurface(*instance, window, nullptr, &surface) != 0){
        throw std::runtime_error("Failed to create window surface");
    }

    this -> surface = vk::raii::SurfaceKHR(instance, surface);
}

void Engine::createInitResources(){

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

void Engine::createSyncObjects()
{
    present_complete_semaphores.clear();
    render_finished_semaphores.clear();
    in_flight_fences.clear();

    for(size_t i = 0; i < swapchain.images.size(); i++){
        present_complete_semaphores.emplace_back(vk::raii::Semaphore(logical_device, vk::SemaphoreCreateInfo()));
        render_finished_semaphores.emplace_back(vk::raii::Semaphore(logical_device, vk::SemaphoreCreateInfo()));
    }

    for(size_t i = 0; i < queue_pool.max_frames_in_flight; i++){
        in_flight_fences.emplace_back(vk::raii::Fence(logical_device, {vk::FenceCreateFlagBits::eSignaled}));
    }
}


// --- RUN FUNCTIONS ---

void Engine::run(){
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        drawFrame();
    }

    logical_device.waitIdle();
}

void Engine::drawFrame()
{
    // CPU block
    while(vk::Result::eTimeout == logical_device.waitForFences(*in_flight_fences[current_frame], vk::True, UINT64_MAX));

    // GPU block
    auto [result, image_index] = swapchain.swapchain.acquireNextImage(UINT64_MAX, *present_complete_semaphores[present_semaphore_index], nullptr);

    if(result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR){
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Resetting the synchronization components
    logical_device.resetFences(*in_flight_fences[current_frame]);
    queue_pool.graphics_command_buffers[current_frame].reset();

    std::chrono::_V2::system_clock::time_point current_time = std::chrono::high_resolution_clock::now();
    time = std::chrono::duration<float, std::chrono::milliseconds::period>(current_time - prev_time).count();
    prev_time = current_time;

    processInput();
    updateUniformBuffers(time, current_frame);
    recordCommandBuffer(image_index);

    vk::PipelineStageFlags wait_destination_stage_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo submit_info;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &*present_complete_semaphores[present_semaphore_index];
    submit_info.pWaitDstStageMask = &wait_destination_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &*queue_pool.graphics_command_buffers[current_frame];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &*render_finished_semaphores[image_index];

    queue_pool.graphics_queue.submit(submit_info, *in_flight_fences[current_frame]);

    vk::PresentInfoKHR present_info_KHR;
    present_info_KHR.waitSemaphoreCount = 1;
    present_info_KHR.pWaitSemaphores = &*render_finished_semaphores[image_index];
    present_info_KHR.swapchainCount = 1;
    present_info_KHR.pSwapchains = &*swapchain.swapchain;
    present_info_KHR.pImageIndices = &image_index;

    result = queue_pool.present_queue.presentKHR(present_info_KHR);
    switch(result){
        case vk::Result::eSuccess: break;
        case vk::Result::eSuboptimalKHR:
            std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n";
            break;
        default: break; // an unexpected result is returned
    }

    current_frame = (current_frame + 1) % queue_pool.max_frames_in_flight;
    present_semaphore_index = (present_semaphore_index + 1) % present_complete_semaphores.size();
}

void Engine::recordInput(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    std::map<int, InputState> &inputs = *reinterpret_cast<std::map<int, InputState> *>(glfwGetWindowUserPointer(window));

    inputs[key] = action == GLFW_PRESS ? InputState::PRESSED : (action == GLFW_REPEAT ? InputState::HOLD : InputState::RELEASED);
}

void Engine::processInput()
{
    if(inputs.count(GLFW_KEY_SPACE) && inputs[GLFW_KEY_SPACE] == InputState::PRESSED){
        std::cout << "You would have jumped, if there were a jumping feature!" << std::endl;
        inputs[GLFW_KEY_SPACE] = InputState::RELEASED; // to indicate input ahs been consumed
    }
}

void Engine::updateUniformBuffers(float dtime, int current_frame)
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

void Engine::recordCommandBuffer(uint32_t image_index)
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

// --- CLOSING FUNCTIONS ---

void Engine::cleanup(){
    std::cout << "\nCLEANING UP RESOURCES..." << std::endl;
    // Destroying the images -> this is needed since we need to destroy the allocator
    color_image.~AllocatedImage();
    depth_image.~AllocatedImage();

    // Destroying the gameobject buffers
    objects.clear();
    ubo_camera_mapped.clear();
    ubo_objects_mapped.clear();
    

    // Destroying the allocator
    vmaDestroyAllocator(vma_allocator);
}