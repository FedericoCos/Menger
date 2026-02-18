#version 450

// Input from vertex shader
layout(location = 10) in vec3 fragPos;
layout(location = 11) in vec3 fragNorm;
layout(location = 12) in vec3 fragColor;

// Ouput of fragment shader
layout(location = 0) out vec4 outColor;

// Light info
vec3 ambient = vec3(0.1, 0.1, 0.1);
vec3 light_pos = vec3(10, 10, 10);
vec3 light_col = vec3(0.4);

void main(){
    vec3 norm = normalize(fragNorm);
    vec3 light_dir = normalize(light_pos - fragPos);
    vec3 diffuse = max(dot(norm, light_dir), 0.0) * light_col;

    outColor = vec4((ambient + diffuse) * fragColor, 1.0);
}