# Menger Sponge Vulkan Renderer

A high-performance, Vulkan-based 3D rendering engine written in C++ that procedurally generates and visualizes the Menger Sponge fractal.

## 🚀 Features

* **Procedural Fractal Generation:** Dynamically generates the Menger Sponge geometry up to level 5, calculating free spaces and cube coordinates iteratively.
* **High-Performance Instancing:** Renders up to ~3.2 million cubes seamlessly by utilizing Vulkan instanced rendering to draw a single cube mesh multiple times. The vertex shader calculates precise world coordinates using `gl_InstanceIndex`.
* **Simulated Deferred Shading (Light Culling):** Implements a highly optimized point-light system using OpenMP (`#pragma omp parallel for`) to compute light-to-cube intersections on the CPU. The engine stores active light indices per cube via Storage Buffers (SSBOs) and processes accurate lighting, attenuation, and windowing in the fragment shader.
* **Cinematic Auto-Camera:** Features a specialized `C_camera` system that can be toggled to automatically fly and navigate through the calculated empty grid spaces of the Menger Sponge.
* **Multi-threaded CPU Operations:** Utilizes OpenMP to parallelize heavy distance calculations between hundreds of thousands of light sources and millions of cubes.

## 🛠️ Requirements & Dependencies

To build and run the project, the following dependencies are required:
* **C++20** compatible compiler (e.g., `g++`)
* **Vulkan SDK** (`-lvulkan`)
* **GLFW** (`-lglfw`) and **SDL2** (`sdl2-config`)
* **OpenMP** (`-fopenmp`)
* **GLSL Compiler** (`glslc`) for offline shader compilation

## 🏗️ Build & Run

A `Makefile` is provided for easy compilation.

To compile the shaders and run the application in default mode (1920x1080):
```bash
make run
```

To run the testing environment:
```bash
make test
```

To clean the environment:
```bash
make clean
```

Note: The make run and make test targets automatically compile the GLSL shaders (vertex.vert and fragment.frag) into SPIR-V before executing.

## 🎮 Controls

The engine relies on standard keyboard inputs to interact with the environment and the fractal generation:

* **SPACE:** Execute the next Menger Step (subdivides the sponge further, up to level 5).
* **W / A / S / D:** Move the camera Forward / Left / Backward / Right.
* **Arrow Keys (Up/Down/Left/Right):** Rotate camera (Pitch and Yaw).
* **Left Shift / Right Shift:** Roll the camera left or right.
* **R:** Toggle the Automatic Cinematic Camera (starts flying through the fractal's free blocks).
