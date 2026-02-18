#version 450

// Locations defined by Vertex struct
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

// Output locations (to fragment shader)
layout(location = 10) out vec3 fragPos;
layout(location = 11) out vec3 fragNorm;
layout(location = 12) out vec3 fragColor;

layout(binding = 0) uniform UniformBufferCamera {
    mat4 view;
    mat4 proj;
} cam_ubo;

layout(std430, binding = 1) readonly buffer CubesSSBO {
    vec4 positions[]; 
} obj_buffer;

layout(binding = 2) uniform UniformBufferCube{
    mat4 rotate_matrix;
    vec4 center_and_scale;
}cube_ubo;

void main(){
    vec3 pos = inPosition * cube_ubo.center_and_scale.w;
    pos += obj_buffer.positions[gl_InstanceIndex].xyz;

    mat3 rotate_matrix = mat3(cube_ubo.rotate_matrix);
    pos = rotate_matrix * pos;
    pos += cube_ubo.center_and_scale.xyz;

    gl_Position = cam_ubo.proj * cam_ubo.view * vec4(pos, 1.0);
    fragPos = pos;
    fragNorm = rotate_matrix * inNormal;
    fragColor = inColor;
}