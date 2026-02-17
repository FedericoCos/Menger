#include "camera.hpp"

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect_ratio, float near_plane, float far_plane) const
{
    return glm::perspective(glm::radians(zoom), aspect_ratio, near_plane, far_plane);
}

void Camera::processKeyboard(CameraMovement direction, float dtime)
{
    float velocity = movement_speed * dtime;

    switch (direction) {
        case CameraMovement::FORWARD:
            position += front * velocity;
            break;
        case CameraMovement::BACKWARD:
            position -= front * velocity;
            break;
        case CameraMovement::LEFT:
            position -= right * velocity;
            break;
        case CameraMovement::RIGHT:
            position += right * velocity;
            break;
        case CameraMovement::UP:
            position += up * velocity;
            break;
        case CameraMovement::DOWN:
            position -= up * velocity;
            break;
    }
}

void Camera::processMouseMovement(float x_offset, float y_offset, bool constrain_pitch)
{
    x_offset *= mouse_sensitivity;
    y_offset *= mouse_sensitivity;

    yaw += x_offset;
    pitch += y_offset;

    if (constrain_pitch) {
        pitch = std::clamp(pitch, -89.f, 89.f);
    }

    updateCameraVectors();
}

void Camera::processMouseScroll(float y_offset)
{
    zoom += y_offset;
}

void Camera::updateCameraVectors()
{
    // Calculate the new front vector
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    front = glm::normalize(front);

    right = glm::normalize(glm::cross(front, world_up));
    up = glm::normalize(glm::cross(right, front));
}
