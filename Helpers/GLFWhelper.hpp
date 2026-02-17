#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdio.h>

namespace GLFWHelper{
    // Initializes the window of the Engine using GLFW
    GLFWwindow * initWindowGLFW(const char * window_title, uint32_t& out_width, uint32_t& out_height);

    // Debug error callback
    void setErrorCallback();
};