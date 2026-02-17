#include "GLFWhelper.hpp"

// Initializes the window of the Engine using GLFW
GLFWwindow * GLFWHelper::initWindowGLFW(const char *window_title, uint32_t &out_width, uint32_t &out_height)
{
    if(!glfwInit()){
        return nullptr;
    }

    const bool wants_whole_area = !out_width || !out_height;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, wants_whole_area ? GLFW_FALSE : GLFW_TRUE);

    GLFWmonitor * monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode * mode = glfwGetVideoMode(monitor);

    int x = 0;
    int y = 0;
    int w = mode -> width;
    int h = mode -> height;

    if(wants_whole_area){
        glfwGetMonitorWorkarea(monitor, &x, &y, &w, &h);
    }
    else{
        w = out_width;
        h = out_height;
    }

    GLFWwindow * window = glfwCreateWindow(w, h, window_title, nullptr, nullptr);

    if(!window){
        glfwTerminate();
        return nullptr;
    }

    if(wants_whole_area){
        glfwSetWindowPos(window, x, y);
    }
    out_width = (uint32_t)w;
    out_height = (uint32_t)h;

    return window;
}

// Debug Error Callback
void GLFWHelper::setErrorCallback()
{
    glfwSetErrorCallback([](int error, const char* description){
        printf("GLFW Error (%i): %s\n", error, description);
    });
}
