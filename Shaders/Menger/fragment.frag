#version 450

// Input from vertex shader
layout(location = 10) in vec3 fragPos;
layout(location = 11) in vec3 fragNorm;
layout(location = 12) in vec3 fragColor;


struct Pointlight{
    vec4 position;
    vec4 color;
};

layout(std430, binding = 3) readonly buffer PointlightSSBO {
    vec4 num; // only first value used for current number of pointlights
    Pointlight lights[]; 
} pointlights;


// Ouput of fragment shader
layout(location = 0) out vec4 outColor;

// Light info
vec3 ambient = vec3(0.1, 0.1, 0.1);
vec3 light_pos = vec3(10, 10, 10);
vec3 light_col = vec3(0.4);

void main(){
   vec3 norm = normalize(fragNorm);
    vec3 total_diffuse = vec3(0.0);
    
    int num_lights = int(pointlights.num.x);

    const float THRESHOLD = 0.01; 

    for (int i = 0; i < num_lights; i++) {
        vec3 diff = pointlights.lights[i].position.xyz - fragPos;
        float dist_sq = dot(diff, diff);

        if (dist_sq > pointlights.lights[i].position.w) {
            continue; 
        }

        float dist = sqrt(dist_sq);
        vec3 light_dir = diff / dist;
        
        float attenuation = pointlights.lights[i].color.w / (dist_sq + 1.0);
        
        float window = clamp(1.0 - (dist_sq / pointlights.lights[i].position.w), 0.0, 1.0);
        attenuation *= (window * window);

        float diff_coeff = max(dot(norm, light_dir), 0.0);
        total_diffuse += diff_coeff * pointlights.lights[i].color.rgb * attenuation;
    }

    vec3 finalColor = (ambient + total_diffuse) * fragColor;
    outColor = vec4(finalColor, 1.0);
}