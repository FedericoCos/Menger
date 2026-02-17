#pragma once

#include "../Helpers/GeneralLibraries.hpp"
#include "../Helpers/GLFWhelper.hpp"

#include "device.hpp"
#include "swapchain.hpp"
#include "memory.hpp"
#include "image.hpp"
#include "pipeline.hpp"
#include "gameobject.hpp"
#include "camera.hpp"



// Debug Settiings
const std::vector validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};
#ifdef NDEBUG
constexpr bool enable_val_layers = false;
#else
constexpr bool enable_val_layers = true;
#endif



class Engine{
public:

    // Entry point of the engine. Initialize window and Vulkan components
    void init(const std::string title, uint32_t &w, uint32_t &h);
    
    // Closing functions: cleans the non-raii resources
    void cleanup();

    //loop function
    void run();

protected:
    // Window variables
    GLFWwindow * window;
    std::string title;
    uint32_t win_width;
    uint32_t win_height;

    // Instance variables
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;

    // Debugging variables
    vk::raii::DebugUtilsMessengerEXT debug_messanger = nullptr;

    // Surface related components
    vk::raii::SurfaceKHR surface = nullptr;

    // Device related components
    vk::raii::PhysicalDevice physical_device = nullptr;
    vk::raii::Device logical_device = nullptr;
    QueuePool queue_pool;

    // Swapchain related components
    SwapchainBundle swapchain;

    // Memory allocator components
    VmaAllocator vma_allocator;

    // Images components
    vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1;
    AllocatedImage color_image; // The image we write onto
    AllocatedImage depth_image;

    // Pipeline components
    std::vector<RasterPipelineBundle> raster_pipelines;
    std::vector<Gameobject> objects;
    std::vector<std::vector<MappedUBO>> ubo_objects_mapped;
    std::map<RasterPipelineBundle *, std::vector<Gameobject *>> pip_to_obj; // This allows me to connect all the objects using the same pipeline

    // Synchronization components
    uint32_t current_frame = 0;
    std::vector<vk::raii::Semaphore> present_complete_semaphores; // Used to synchronize the images being actually displayed
    uint32_t present_semaphore_index = 0;
    std::vector<vk::raii::Semaphore> render_finished_semaphores; // Used to synchronize operations on frames by the GPU before presenting them
    std::vector<vk::raii::Fence> in_flight_fences; // Used to synchronize operations on the CPU

    // FPS tracker components
    float time = 0.0;
    std::chrono::_V2::system_clock::time_point prev_time; 

    // Camera components
    Camera camera;
    std::vector<MappedUBO> ubo_camera_mapped;

    // Input variables
    std::map<int, InputState> inputs;

    // --- HELPER FUNCTIONS ---

    /**
     * Function to setup the debugger if validation layers are enabled. Allows for better error messages, though it comes at a perfomance cost
     */
    void setupDebugMessanger();

    /**
     * Retrieve any validation error. Used by the debugger
     */
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
        std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

        return vk::False;
    }

    // Gets GLFW extensions for Vulkan and necessary extensions for debugging
    std::vector<const char *> getRequiredExtensions();


    // --- INITIALIZATION FUNCTIONS ---

    // Initializes the window system using GLTF
    void initWindow();
    // Initiliazes all Vulkkan Components
    void initVulkan();

    // Initialize a Vulkan Instance
    void createInstance();
    // Initialize a surface. It queries GLFW to do so (NOTE: maybe decouple from engine to make it agnostic from glfw?)
    void createSurface();
    // Initializes Pipelines and scene objects
    virtual void createInitResources();
    // Initializes Synchronization objects
    void createSyncObjects();


    // --- RUN FUNCTIONS ---

    // Function meant to update Uniform Buffer
    virtual void updateUniformBuffers(float dtime, int current_frame);

    // Main functions to register commands to the GPU
    virtual void recordCommandBuffer(uint32_t image_index);

    // main function for rendering
    void drawFrame();

    // Input function. Maps inputs to a dictionary for later usage
    static void recordInput(GLFWwindow *window, int key, int scancode, int action, int mods);

    // Actual function that process keyboard input accordingly
    virtual void processInput();

    // --- CLOSING FUNCTIONS ---

};