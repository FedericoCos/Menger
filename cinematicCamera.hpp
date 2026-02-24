#pragma once

#include "VulkanEngine/camera.hpp"

class C_camera : public Camera{
public:
    C_camera(
        glm::vec3 position = glm::vec3(0),
        float max_speed = 0.0f,
        float thrust = 0.0f,
        float friction = 0.f,
        float rot_speed = 0.f,
        float mouse_sensitivity = 0.0f,
        glm::vec3 up = glm::vec3(0.f, 1.f, 0.f),
        float yaw = -90.f,
        float pitch = 0.f,
        float zoom = 65.f,
        glm::vec3 world_up = glm::vec3(0.f, 1.f, 0.f)
    ) : Camera(position, max_speed, mouse_sensitivity, up, yaw, pitch, zoom, world_up){
        this -> thrust = thrust;
        this -> friction = friction;
        this -> rot_speed = rot_speed;
        this -> current_rot = rot_speed;

        velocity = glm::vec3(0.f);
        acceleration = glm::vec3(0.f);
        orientation = glm::quatLookAt(front, up);

        gen.seed(std::random_device{}());
    }

    void processKeyboard(CameraMovement direction, float dtime) override;

    void update(float dtime) override;

    void createGrid(std::vector<glm::vec4> &centers, uint32_t max_level);

    bool isAutomatic(){
        return automatic;
    }

private:
    glm::vec3 velocity;
    glm::vec3 acceleration;

    float thrust;
    float friction;
    float epsilon = 0.001f;
    float rot_speed;
    float current_rot;

    bool dirty_front = false;
    glm::quat orientation;

    // Space grid
    std::vector<glm::vec4> objs;
    std::vector<uint32_t> levels;
    bool automatic = false;
    uint32_t index_obj = 0;
    uint32_t max_level;

    std::default_random_engine gen;


    void updateCameraVectors() override;

    bool isFree(uint32_t x, uint32_t y, uint32_t z);

    void chooseNext();
};