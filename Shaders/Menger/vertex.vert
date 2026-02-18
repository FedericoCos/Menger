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

layout(binding = 1) uniform UniformBufferGameObject{
    mat4 model;
}obj_ubo[160000];

void main(){
    vec4 position = cam_ubo.proj * cam_ubo.view * obj_ubo[gl_InstanceIndex].model * vec4(inPosition, 1.0);
    gl_Position = position;
    fragPos = position.xyz;
    fragNorm = (obj_ubo[gl_InstanceIndex].model * vec4(inNormal, 0.0)).xyz;
    fragColor = inColor;
}