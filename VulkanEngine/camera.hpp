#pragma once

#include "../Helpers/GeneralLibraries.hpp"


class Camera {
public:
    Camera(
        glm::vec3 position = glm::vec3(0),
        glm::vec3 up = glm::vec3(0.f, 1.f, 0.f),
        float yaw = -90.f,
        float pitch = 0.f,
        float zoom = 65.f,
        glm::vec3 world_up = glm::vec3(0.f, 1.f, 0.f)
    ){
        this->position = position;
        this->up = up;
        this->yaw = yaw;
        this->pitch = pitch;
        this -> zoom = zoom;
        this -> world_up = world_up;

        updateCameraVectors();
    }

    // Matrix generation for graphics pipeline integration
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect_ratio, float near_plane = 0.1f, float far_plane = 100.f) const;

    // INput procesing methods for different interaction modalities
    void processKeyboard(CameraMovement direction, float dtime);
    void processMouseMovement(float x_offset, float y_offset, bool constrain_pitch = true);
    void processMouseScroll(float y_offset);

    // Property access methods for external systems
    glm::vec3 getPosition() const {return position; }
    glm::vec3 getFront() const { return front; }
    float getZoom() const { return zoom; }

private:
    // Spatial positioning and orientation vectors
    glm::vec3 position;
    glm::vec3 front; // Forward direction of where the camera is looking
    glm::vec3 up; // Camera's local up direction
    glm::vec3 right; // Camera's local right direction
    glm::vec3 world_up;

    // Rotation representation using Euler angles
    float yaw; // left/right
    float pitch; // up-down

    // User interaction and behavior parameters
    float movement_speed;
    float mouse_sensitivity;
    float zoom;


    // Internal coordinate system maintenance
    void updateCameraVectors();

};