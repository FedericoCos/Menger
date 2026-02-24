#include "cinematicCamera.hpp"

void C_camera::processKeyboard(CameraMovement direction, float dtime)
{
    float angle = glm::radians(rot_speed * dtime);

    switch (direction) {
        case CameraMovement::FORWARD:
            acceleration += front * thrust;
            break;
        case CameraMovement::BACKWARD:
            acceleration += -front * thrust;
            break;
        case CameraMovement::LEFT:
            acceleration += -right * thrust;
            break;
        case CameraMovement::RIGHT:
            acceleration += right * thrust;
            break;
        case CameraMovement::UP:
            acceleration += up * thrust;
            break;
        case CameraMovement::DOWN:
            acceleration -= up * thrust;
            break;


        case CameraMovement::T_LEFT:
            orientation = glm::normalize(orientation * glm::angleAxis(angle, glm::vec3(0.f, 1.f, 0.f)));
            dirty_front = true;
            break;
        case CameraMovement::T_RIGHT:
            orientation = glm::normalize(orientation * glm::angleAxis(-angle, glm::vec3(0.f, 1.f, 0.f)));
            dirty_front = true;
            break;
        case CameraMovement::T_UP:
            orientation = glm::normalize(orientation * glm::angleAxis(-angle, glm::vec3(1.f, 0.f, 0.f)));
            dirty_front = true;
            break;
        case CameraMovement::T_DOWN:
            orientation = glm::normalize(orientation * glm::angleAxis(angle, glm::vec3(1.f, 0.f, 0.f)));
            dirty_front = true;
            break;
        case CameraMovement::T_ROLL_LEFT:
            orientation = glm::normalize(orientation * glm::angleAxis(angle, glm::vec3(0.f, 0.f, -1.f)));
            dirty_front = true;
            break;

        case CameraMovement::T_ROLL_RIGHT:
            orientation = glm::normalize(orientation * glm::angleAxis(-angle, glm::vec3(0.f, 0.f, -1.f)));
            dirty_front = true;
            break;
    }
}

void C_camera::update(float dtime)
{   
    if(automatic){
        glm::vec3 dir = glm::vec3(objs[index_obj]) - position;
        float dist_to_target = glm::length(dir);

        if(dist_to_target < 0.001f){
            chooseNext();
            dir = glm::vec3(objs[index_obj]) - position;
            dist_to_target = glm::length(dir);
        }

        if (dist_to_target > 0.001f) {
            glm::vec3 desired_dir = dir / dist_to_target;

            glm::vec3 rotation_axis = glm::cross(front, desired_dir);
            float axis_length = glm::length(rotation_axis);

            if (axis_length > 0.001f) {
                rotation_axis /= axis_length;
                
                float full_angle = glm::acos(glm::clamp(glm::dot(front, desired_dir), -1.0f, 1.0f));
                
                float turn_amount = glm::min(full_angle, glm::radians(current_rot * dtime));
                
                glm::quat gentle_turn = glm::angleAxis(turn_amount, rotation_axis);
                orientation = glm::normalize(gentle_turn * orientation);
                dirty_front = true;
            } 
            else if (glm::dot(front, desired_dir) < -0.99f) {
                glm::quat gentle_turn = glm::angleAxis(glm::radians(current_rot * dtime), up);
                orientation = glm::normalize(gentle_turn * orientation);
                dirty_front = true;
            }

            if(dirty_front){
                updateCameraVectors();
            }

            acceleration = desired_dir * thrust;

            velocity += acceleration * dtime;
            if(glm::length2(velocity) > max_speed * max_speed){
                velocity = glm::normalize(velocity) * max_speed;
            }
            glm::vec3 displacement = velocity * dtime;

            if(glm::length2(displacement) > glm::length2(dir)){
                displacement = dir;
            }

            position += displacement;

            return;
        }

        velocity += acceleration * dtime;

        if(glm::length2(velocity) > max_speed * max_speed){
            velocity = glm::normalize(velocity) * max_speed;
        }

        position += velocity * dtime;

        return;
    }

    if(dirty_front){
        dirty_front = false;
        updateCameraVectors();
    }

    float acc_mag_sq = glm::length2(acceleration);
    float vel_mag_sq = glm::length2(velocity);

    // Apply friction if moving
    if(vel_mag_sq > 0){
        glm::vec3 friction_vec = glm::normalize(velocity) * friction * dtime;
        if(vel_mag_sq <= glm::length2(friction_vec)){
            velocity = glm::vec3(0.f);
        }
        else{
            velocity -= friction_vec;
        }
    }

    // Apply acceleration
    if(acc_mag_sq > 0){
        velocity += acceleration * dtime;
        if(glm::length2(velocity) > max_speed * max_speed){
            velocity = glm::normalize(velocity) * max_speed;
        }
    }

    // Move position
    position += velocity * dtime;

    // Reset acceleration
    acceleration = glm::vec3(0.f);
}

void C_camera::createGrid(std::vector<glm::vec4> &centers, uint32_t max_level)
{
    this -> max_level = max_level;
    objs.resize(centers.size());
    levels.resize(centers.size());
    for(size_t i = 0; i < centers.size(); i++){
        objs[i] = centers[i];
        levels[i] = objs[i].w;
        if(objs[i].w == max_level && max_level > 1){
            objs[i].w = 0.5 / std::pow(6, max_level - 2);
        }
        else{
            objs[i].w = 1.0 / std::pow(6, objs[i].w - 1);
        }
    }

    automatic = true;
}

void C_camera::updateCameraVectors()
{
    front = glm::normalize(orientation * glm::vec3(0.0f, 0.0f, -1.0f));
    up    = glm::normalize(orientation * glm::vec3(0.0f, 1.0f, 0.0f));
    right = glm::normalize(orientation * glm::vec3(1.0f, 0.0f, 0.0f));
}

bool C_camera::isFree(uint32_t x, uint32_t y, uint32_t z)
{
     while (x > 0 || y > 0 || z > 0) {
        if ((x % 3 == 1 && y % 3 == 1) || 
            (x % 3 == 1 && z % 3 == 1) || 
            (y % 3 == 1 && z % 3 == 1)) {
            return false;
        }
        x /= 3; y /= 3; z /= 3;
    }
    return true;
}

void C_camera::chooseNext()
{
    glm::vec3 current_target = objs[index_obj];
    
    std::vector<uint32_t> valid_centers;
    std::vector<uint32_t> fallback_centers;

    float min_dist = 100.0f; 
    float max_dist = 1200.0f;

    for(size_t i = 0; i < objs.size(); i++){
        if(index_obj == i) continue;

        bool match_x = std::abs(objs[i].x - current_target.x) < 0.01f;
        bool match_y = std::abs(objs[i].y - current_target.y) < 0.01f;
        bool match_z = std::abs(objs[i].z - current_target.z) < 0.01f;
        
        if((match_x && match_y) || (match_x && match_z) || (match_y && match_z)){
            fallback_centers.push_back(i);

            float dist = glm::distance(current_target, glm::vec3(objs[i]));
            if(dist >= min_dist && dist <= max_dist){
                
                glm::vec3 potential_dir = glm::normalize(glm::vec3(objs[i]) - current_target);
                
                if(glm::dot(front, potential_dir) > -0.5f){
                    valid_centers.push_back(i);
                }
            }
        }
    }
    double accumulation = 0;
    if(!valid_centers.empty()){
        for(size_t i =0; i < valid_centers.size(); i++){
            accumulation += objs[valid_centers[i]].w;
        } 
        std::uniform_real_distribution<double> distribution(0.0, accumulation); 
        double random_val = distribution(gen);
        size_t i =0;
        while(i < valid_centers.size() - 1 && random_val - objs[valid_centers[i]].w > 0.f){
            random_val -= objs[valid_centers[i]].w;
            i++;
        }
        index_obj = valid_centers[i];
        current_rot = rot_speed / std::pow(2, max_level - levels[index_obj]);
    } 
    else if(!fallback_centers.empty()){
       for(size_t i =0; i < fallback_centers.size(); i++){
            accumulation += objs[fallback_centers[i]].w;
        } 
        std::uniform_real_distribution<double> distribution(0.0, accumulation); 
        double random_val = distribution(gen);
        size_t i =0;
        while(i < fallback_centers.size() - 1 && random_val - objs[fallback_centers[i]].w > 0.f){
            random_val -= objs[fallback_centers[i]].w;
            i++;
        }
        index_obj = fallback_centers[i];
        current_rot = rot_speed / std::pow(2, max_level - levels[index_obj]);
    }
}
