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

mat4 translate(vec4 pos){
    return mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(pos.x, pos.y, pos.z, 1.0)
    );
}


layout(binding = 1) uniform UniformBufferGameObject{
    vec4 position;
}obj_ubo[160000];

layout(binding = 2) uniform UniformBufferCube{
    mat4 rotate_matrix;
    mat4 scale_matrix;
    mat4 center_translation_matrix;
}cube_ubo;

void main(){
    mat4 model = cube_ubo.center_translation_matrix * 
                    cube_ubo.rotate_matrix * 
                    translate(obj_ubo[gl_InstanceIndex].position) * 
                    cube_ubo.scale_matrix;

    vec4 position = cam_ubo.proj * cam_ubo.view * model * vec4(inPosition, 1.0);
    gl_Position = position;
    fragPos = position.xyz;
    fragNorm = (model * vec4(inNormal, 0.0)).xyz;
    fragColor = inColor;
}