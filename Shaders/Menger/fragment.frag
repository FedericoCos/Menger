#version 450

#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require

// Input from vertex shader
layout(location = 10) in vec3 fragPos;
layout(location = 11) in vec3 fragNorm;
layout(location = 12) in vec3 fragColor;
layout(location = 13) flat in int instance_id;


struct Pointlight{
    vec4 position;
    vec4 color;
};

layout(std430, binding = 3) readonly buffer PointlightSSBO {
    vec4 num; // only first value used for current number of pointlights
    Pointlight lights[]; 
} pointlights;

layout(std430, binding = 4) readonly buffer LightIndicesSSBO {
    uint indices[]; 
} light_indices;

layout(std430, binding = 5) readonly buffer LightIndicesSizeSSBO {
    uint indices[]; 
} light_indices_size;


// Ouput of fragment shader
layout(location = 0) out vec4 outColor;

// Light info
vec3 ambient = vec3(0.1, 0.1, 0.1);
vec3 light_pos = vec3(10, 10, 10);
vec3 light_col = vec3(0.4);

void main(){
    vec3 norm = normalize(fragNorm);
    vec3 total_diffuse = vec3(0.0);

    for(uint i = light_indices_size.indices[instance_id]; i < light_indices_size.indices[instance_id + 1]; i++){
        uint light_index = light_indices.indices[i];

        vec3 diff = pointlights.lights[light_index].position.xyz - fragPos;
        float dist_sq = dot(diff, diff);

        float dist = sqrt(dist_sq);
        vec3 light_dir = diff / dist;
        
        float attenuation = pointlights.lights[light_index].color.w / (dist_sq + 1.0);
        
        float window = clamp(1.0 - (dist_sq / pointlights.lights[light_index].position.w), 0.0, 1.0);
        attenuation *= (window * window);

        float diff_coeff = max(dot(norm, light_dir), 0.0);
        total_diffuse += diff_coeff * pointlights.lights[light_index].color.rgb * attenuation;
    }

    vec3 finalColor = (ambient + total_diffuse) * fragColor;
    outColor = vec4(finalColor, 1.0);
}