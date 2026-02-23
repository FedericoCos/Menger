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

void C_camera::updateCameraVectors()
{
    front = glm::normalize(orientation * glm::vec3(0.0f, 0.0f, -1.0f));
    up    = glm::normalize(orientation * glm::vec3(0.0f, 1.0f, 0.0f));
    right = glm::normalize(orientation * glm::vec3(1.0f, 0.0f, 0.0f));
}
