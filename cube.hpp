#pragma once

#include "VulkanEngine/gameobject.hpp"

class Cube : public Gameobject {
public:
    Cube(
        glm::vec3 position = glm::vec3(0),
        glm::vec3 scale = glm::vec3(1),
        glm::vec3 rotation = glm::vec3(0),
        glm::vec3 dis_speed = glm::vec3(0),
        glm::vec3 rot_speed = glm::vec3(0),
        glm::vec3 scale_speed = glm::vec3(0),
        glm::vec3 center = glm::vec3(0),
        bool isVertexHolder = true// This is used since instancing require the definition of vertices and indices just one time
    ): Gameobject(position, scale, rotation, dis_speed, rot_speed, scale_speed){

        this -> center = center;

        if(isVertexHolder){
            vertices = {
                // FRONT FACE (Normal 0, 0, 1)
                {glm::vec3(-0.5, 0.5, 0.5), glm::vec3(0, 0, 1), glm::vec3(0.5)}, // TL
                {glm::vec3(0.5, 0.5, 0.5),  glm::vec3(0, 0, 1), glm::vec3(0.5)}, // TR
                {glm::vec3(0.5, -0.5, 0.5), glm::vec3(0, 0, 1), glm::vec3(0.5)}, // BR
                {glm::vec3(-0.5, -0.5, 0.5),glm::vec3(0, 0, 1), glm::vec3(0.5)}, // BL

                // BACK FACE (Normal 0, 0, -1)
                {glm::vec3(0.5, 0.5, -0.5),  glm::vec3(0, 0, -1), glm::vec3(0.5)}, // TL
                {glm::vec3(-0.5, 0.5, -0.5), glm::vec3(0, 0, -1), glm::vec3(0.5)}, // TR
                {glm::vec3(-0.5, -0.5, -0.5),glm::vec3(0, 0, -1), glm::vec3(0.5)}, // BR
                {glm::vec3(0.5, -0.5, -0.5), glm::vec3(0, 0, -1), glm::vec3(0.5)}, // BL

                // RIGHT FACE (Normal 1, 0, 0)
                {glm::vec3(0.5, 0.5, 0.5),  glm::vec3(1, 0, 0), glm::vec3(0.5)}, // TL
                {glm::vec3(0.5, 0.5, -0.5), glm::vec3(1, 0, 0), glm::vec3(0.5)}, // TR
                {glm::vec3(0.5, -0.5, -0.5),glm::vec3(1, 0, 0), glm::vec3(0.5)}, // BR
                {glm::vec3(0.5, -0.5, 0.5), glm::vec3(1, 0, 0), glm::vec3(0.5)}, // BL

                // LEFT FACE (Normal -1, 0, 0)
                {glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-1, 0, 0), glm::vec3(0.5)}, // TL
                {glm::vec3(-0.5, 0.5, 0.5),  glm::vec3(-1, 0, 0), glm::vec3(0.5)}, // TR
                {glm::vec3(-0.5, -0.5, 0.5), glm::vec3(-1, 0, 0), glm::vec3(0.5)}, // BR
                {glm::vec3(-0.5, -0.5, -0.5),glm::vec3(-1, 0, 0), glm::vec3(0.5)}, // BL

                // TOP FACE (Normal 0, 1, 0)
                {glm::vec3(-0.5, 0.5, -0.5), glm::vec3(0, 1, 0), glm::vec3(0.5)}, // TL
                {glm::vec3(0.5, 0.5, -0.5),  glm::vec3(0, 1, 0), glm::vec3(0.5)}, // TR
                {glm::vec3(0.5, 0.5, 0.5),   glm::vec3(0, 1, 0), glm::vec3(0.5)}, // BR
                {glm::vec3(-0.5, 0.5, 0.5),  glm::vec3(0, 1, 0), glm::vec3(0.5)}, // BL

                // BOTTOM FACE (Normal 0, -1, 0)
                {glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0, -1, 0), glm::vec3(0.5)}, // TL
                {glm::vec3(0.5, -0.5, 0.5),  glm::vec3(0, -1, 0), glm::vec3(0.5)}, // TR
                {glm::vec3(0.5, -0.5, -0.5), glm::vec3(0, -1, 0), glm::vec3(0.5)}, // BR
                {glm::vec3(-0.5, -0.5, -0.5),glm::vec3(0, -1, 0), glm::vec3(0.5)}, // BL
            };

            indices = {
                // Front
                0, 1, 2,  0, 2, 3,
                // Back
                4, 5, 6,  4, 6, 7,
                // Right
                8, 9, 10, 8, 10, 11,
                // Left
                12, 13, 14, 12, 14, 15,
                // Top
                16, 17, 18, 16, 18, 19,
                // Bottom
                20, 21, 22, 20, 22, 23
            };
        }
    }


    // Assign new position and scale for menger_sponge
    void modifyCube(glm::vec3 new_position, glm::vec3 new_scale){
        this -> position = new_position;
        this -> scale = new_scale;

        dirty_model = true;
    }

    // Need to modify how to extract the model matrix since we need rotation around the center
    const glm::mat4 &getModelMat() override{
        if(dirty_model){
            // Recalculate model matrix when needed
            model = glm::mat4(1.0);
            model = glm::translate(model, center);
            if(rotation.x != 0.0){
                model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
            }
            if(rotation.y != 0.0){
                model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0,1, 0));
            }
            if(rotation.z != 0.0){
                model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
            }


            model = glm::translate(model, position - center);
            model = glm::scale(model, scale);
        }
        return model;
    }

    const glm::vec3 &getRotationVector(){
        return rotation;
    }

private:
    glm::vec3 center;

};