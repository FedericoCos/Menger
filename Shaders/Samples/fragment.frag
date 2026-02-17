#version 450

// Input from vertex shader
layout(location = 10) in vec3 fragColor;

// Ouput of fragment shader
layout(location = 0) out vec4 outColor;

void main(){
    outColor = vec4(fragColor, 1.0);
}