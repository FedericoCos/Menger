#include "device.hpp"

// Helper function for VmaResults
const char* Device::VmaResultToString(VkResult r) {
    switch (r) {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        // add others if needed
        default: return "VkResult(unknown)";
    }
}

std::vector<const char *> device_extensions = {
    vk::KHRSwapchainExtensionName, // extension required for presenting rendered images to the window
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName,
};

vk::raii::PhysicalDevice Device::pickPhysicalDevice(const vk::raii::Instance &instance){
    std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

    if(devices.empty()){
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    // Run thorugh all devices and select the one with the highest score
    // using 8 bit int since it is almost impossible someone has more than 256 GPUs connected to a single device
    // max score set to 255 as well
    uint8_t score_best = 0;
    uint8_t index_best = -1;
    uint8_t current_score = 0;

    for(size_t i = 0; i < devices.size(); i++){
        std::cout << "Checking device: " << devices[i].getProperties().deviceName;

        current_score = calculateScore(devices[i]);

        std::cout << ". Score: " << (int)current_score << std::endl;

        if(current_score >  score_best){
            score_best = current_score;
            index_best = i;
        }

    }

    if(index_best < 0 || index_best >= devices.size()){
        throw std::runtime_error("Something went wrong with the GPU selection!");
    }

    std::cout << "Selected device: " << devices[index_best].getProperties().deviceName << std::endl;
    return std::move(devices[index_best]);
}

vk::raii::Device Device::createLogicalDevice(const vk::raii::PhysicalDevice &physical_device, vk::raii::SurfaceKHR &surface, QueuePool &indices){
    findQueueFamilies(physical_device, surface, indices);
    std::set<uint32_t> unique_queue_families = {
        indices.graphics_family.value(),
        indices.present_family.value(),
        indices.transfer_family.value()
    };

    // Enabling all required features
    vk::PhysicalDeviceFeatures2 deviceFeatures2 = {};
    deviceFeatures2.features.sampleRateShading = vk::True;

    vk::PhysicalDeviceVulkan12Features vulkan12features;
    vulkan12features.bufferDeviceAddress = true; // Memory can be referenced by a pointer rather than just a descriptor set
    vulkan12features.descriptorBindingPartiallyBound = true;
    vulkan12features.scalarBlockLayout = true;

    vk::PhysicalDeviceVulkan13Features vulkan13features;
    vulkan13features.synchronization2 = true;
    vulkan13features.dynamicRendering = true;

    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan12Features, 
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    > feature_chain{
        deviceFeatures2,
        vulkan12features,
        vulkan13features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{ VK_TRUE }
    };

    // create a Device
    float queue_priority = 1.0f; // priority goes from 0.0 to 1.0, it is required even for only 1
    std::vector<vk::DeviceQueueCreateInfo> device_queue_create_infos;
    for(const uint32_t index : unique_queue_families){
        device_queue_create_infos.push_back({
            {}, // flags (usually none)
            index, // queue family index
            1, // number of queues from this family
            &queue_priority // priority
        });
    };


    vk::DeviceCreateInfo device_create_info{
        {},                               // flags (usually none)
        static_cast<uint32_t>(device_queue_create_infos.size()),                                // queueCreateInfoCount
        device_queue_create_infos.data(),        // pQueueCreateInfos
        0,                                // enabledLayerCount (deprecated in Vulkan 1.0+)
        nullptr,                          // ppEnabledLayerNames (deprecated)
        static_cast<uint32_t>(device_extensions.size()), // enabledExtensionCount
        device_extensions.data(),         // ppEnabledExtensionNames
        nullptr,                          // pEnabledFeatures (null because we use pNext for feature chain)
        &feature_chain.get<vk::PhysicalDeviceFeatures2>()    // pNext -> features chain
    };

    return std::move(vk::raii::Device(physical_device, device_create_info));
}

uint8_t Device::calculateScore(vk::raii::PhysicalDevice &device){
    vk::PhysicalDeviceProperties device_properties = device.getProperties();
    vk::PhysicalDeviceFeatures device_features = device.getFeatures();

    uint8_t score = 0;

    if(device_properties.apiVersion < VK_API_VERSION_1_3){ // Select a device with a suitable Vulkan Version
        return score;
    }

    if(!device_features.geometryShader){ // Select a device with at least geometry shader capabilities
        return score;
    }

    // Select a device with graphics capabilities
    std::vector<vk::QueueFamilyProperties> queue_families = device.getQueueFamilyProperties();
    bool supports_graphics = false;
    for(const vk::QueueFamilyProperties &qfp : queue_families){
        if(!!(qfp.queueFlags & vk::QueueFlagBits::eGraphics)){
            supports_graphics = true;
            break;
        }
    }
    if(!supports_graphics){
        return score;
    }

    std::vector<vk::ExtensionProperties> available_device_extensions = device.enumerateDeviceExtensionProperties(); 
    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
    for(const vk::ExtensionProperties &extension : available_device_extensions){
        required_extensions.erase(extension.extensionName);
    }
    if(!required_extensions.empty()){
        return score;
    }

    // Check the features availability
    auto features = device.template getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    >();

    bool supports_required_features =
        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState && // Change pipeline states (cull mode, front face, ...) dynamically
        features.template get<vk::PhysicalDeviceFeatures2>().features.sampleRateShading; // fragment shader runs per-sample and not per-pixel, remove to increase performance 
    
    bool supports_vulkan_11_features = 
        true; // No base features required for a standard engine, change if needed

    bool supports_vulkan_12_properties =
        features.template get<vk::PhysicalDeviceVulkan12Features>().bufferDeviceAddress && // Allows for pointer to buffer, bypass the need to bind a buffer to a descriptor set
        features.template get<vk::PhysicalDeviceVulkan12Features>().descriptorBindingPartiallyBound && // Allows for partially constructed descriptor sets
        features.template get<vk::PhysicalDeviceVulkan12Features>().scalarBlockLayout; // relaxes alignment rules

    bool supports_vulkan_13_properties = 
        features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering && // Allows rendering without a render pass or frame buffer
        features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2; // Allows for synchronization of operations

    if( !(supports_required_features && supports_vulkan_11_features && supports_vulkan_12_properties && supports_vulkan_13_properties)){
        return score;
    }


    score = 1;
    if(device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu){
        score += 1;
    }

    return score;
}

void Device::findQueueFamilies(const vk::raii::PhysicalDevice &physical_device, const vk::raii::SurfaceKHR &surface, QueuePool &indices){
    std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();
    uint32_t i = 0;
    for(const vk::QueueFamilyProperties &queue_family : queue_family_properties){
        // Find a graphics queue
        if(queue_family.queueFlags & vk::QueueFlagBits::eGraphics){
            indices.graphics_family = i;
        }

        // Find a dedicated transfer queue (one that supports transfer but not graphics)
        // usually faster for transfers because of DMA engine
        if((queue_family.queueFlags & vk::QueueFlagBits::eTransfer) &&
            !(queue_family.queueFlags & vk::QueueFlagBits::eGraphics)){
            indices.transfer_family = i;
        }

        // Find a presentation queue
        if(physical_device.getSurfaceSupportKHR(i, *surface)){
            indices.present_family = i;
        }

        if(indices.isIndexComplete()){
            break;
        }
        i++;
    }

    // Falling back to merged transfer and graphics family 
    if(!indices.transfer_family.has_value() && indices.graphics_family.has_value()){
        indices.transfer_family = indices.graphics_family;
    }
}

vk::raii::CommandPool Device::createCommandPool(const vk::raii::Device &logical_device, vk::CommandPoolCreateFlagBits flags, uint32_t queue_index)
{
    vk::CommandPoolCreateInfo pool_info;
    pool_info.flags = flags;
    pool_info.queueFamilyIndex = queue_index;

    return std::move(vk::raii::CommandPool(logical_device, pool_info));
}

vk::raii::CommandBuffers Device::createCommandBuffer(vk::raii::CommandPool &command_pool, vk::CommandBufferLevel level, int max_frames_in_flight, vk::raii::Device &logical_device)
{
    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.commandPool = command_pool;
    alloc_info.level = level;
    alloc_info.commandBufferCount = max_frames_in_flight;

    return std::move(vk::raii::CommandBuffers(logical_device, alloc_info));

}

AllocatedBuffer Device::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, std::string name, VmaAllocator &vma_allocator)
{
    AllocatedBuffer buffer;

    // Prepare VMA alloc info
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    // Decide flags by host-visible property
    if ((properties & vk::MemoryPropertyFlagBits::eHostVisible) != vk::MemoryPropertyFlags{}) {
        // Staging-like buffer: request mapped & sequential write host access
        alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    } else {
        // Device-only buffer: no special flags (not mapped)
        alloc_info.flags = 0;
    }

    // Fill VkBufferCreateInfo (use raw Vulkan struct via cast)
    vk::BufferCreateInfo buffer_info{};
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = vk::SharingMode::eExclusive;


    VkBuffer temp_buffer;
    VkResult r = vmaCreateBuffer(vma_allocator, reinterpret_cast<VkBufferCreateInfo const*>(&buffer_info), 
            &alloc_info, &temp_buffer, &buffer.allocation, &buffer.info);
    if (r != VK_SUCCESS) {
        std::stringstream ss;
        ss << "vmaCreateBuffer failed: " << VmaResultToString(r) << " (" << r << ")";
        throw std::runtime_error(ss.str());
    }
    else{
        buffer.buffer = vk::Buffer(temp_buffer);
    }
    buffer.allocator = vma_allocator;
    buffer.name = name;
    buffer.size = size;

    return buffer;
}

void Device::copyBuffer(AllocatedBuffer &source_buffer, AllocatedBuffer &destination_buffer, 
    vk::DeviceSize size, vk::raii::Device &logical_device, QueuePool &queue_pool, vk::DeviceSize src_offset)
{
    vk::raii::CommandBuffer command_buffer_copy = beginSingleTimeCommands(queue_pool.transfer_command_pool, logical_device);
    command_buffer_copy.copyBuffer(source_buffer.buffer, destination_buffer.buffer, vk::BufferCopy(src_offset, 0, size));
    endSingleTimeCommands(command_buffer_copy, queue_pool.transfer_queue);
}

vk::raii::CommandBuffer Device::beginSingleTimeCommands(vk::raii::CommandPool& command_pool, vk::raii::Device &logical_device){
    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.commandPool = command_pool;
    alloc_info.level = vk::CommandBufferLevel::ePrimary;
    alloc_info.commandBufferCount = 1;

    vk::raii::CommandBuffer command_buffer = std::move(logical_device.allocateCommandBuffers(alloc_info).front());

    vk::CommandBufferBeginInfo command_buffer_begin_info;
    command_buffer_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    command_buffer.begin(command_buffer_begin_info);

    return command_buffer;
}

void Device::endSingleTimeCommands(vk::raii::CommandBuffer &command_buffer, vk::raii::Queue& queue)
{
    command_buffer.end();

    vk::SubmitInfo submit_info;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &*command_buffer;
    queue.submit(submit_info, nullptr);
    queue.waitIdle();
    
}
