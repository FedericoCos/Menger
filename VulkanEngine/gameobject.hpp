#pragma once

#include "../Helpers/GeneralLibraries.hpp"

#include "device.hpp"

class Gameobject{
public:
    Gameobject(
        glm::vec3 position = glm::vec3(0),
        glm::vec3 scale = glm::vec3(1),
        glm::vec3 rotation = glm::vec3(0),
        glm::vec3 dis_speed = glm::vec3(0),
        glm::vec3 rot_speed = glm::vec3(0),
        glm::vec3 scale_speed = glm::vec3(0)
    ){
        this -> position = position,
        this -> scale = scale;
        this -> rotation = rotation;
        this -> dis_speed = dis_speed;
        this -> rot_speed = rot_speed;
        this -> scale_speed = scale_speed;

        vertices = {
            {glm::vec3(-0.5, 0.5, 0.5), glm::vec3(0), glm::vec3(1.0, 0.0, 0.0)},
            {glm::vec3(0.5, 0.5, 0.5), glm::vec3(0), glm::vec3(0.0, 1.0, 0.0)},
            {glm::vec3(0.5, -0.5, 0.5), glm::vec3(0), glm::vec3(0.0, 0.0, 1.0)},
            {glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0), glm::vec3(0.0, 1.0, 0.0)},

            {glm::vec3(-0.5, 0.5, -0.5), glm::vec3(0), glm::vec3(1.0, 0.0, 0.0)},
            {glm::vec3(0.5, 0.5, -0.5), glm::vec3(0), glm::vec3(0.0, 1.0, 0.0)},
            {glm::vec3(0.5, -0.5, -0.5), glm::vec3(0), glm::vec3(0.0, 0.0, 1.0)},
            {glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0), glm::vec3(0.0, 1.0, 0.0)},
        };

        indices = {
            0, 1, 2,
            0, 2, 3,

            1, 5, 6,
            1, 6, 2,

            5, 4, 7,
            5, 7, 6,

            4, 0, 3, 
            4, 3, 7,

            3, 2, 6,
            3, 6, 7,

            4, 5, 1,
            4, 1, 0
        };
    }

    ~Gameobject() = default;

    // Delete Copying
    Gameobject(const Gameobject&) = delete;
    Gameobject& operator=(const Gameobject&) = delete;

    // Enable moving
    Gameobject(Gameobject&& other) noexcept 
        : vertices(std::move(other.vertices)), 
          vertex_buffer(std::move(other.vertex_buffer)),
          indices(std::move(other.indices)),
          index_buffer(std::move(other.index_buffer)),
          position(other.position),
          rotation(other.rotation),
          scale(other.scale),
          dis_speed(other.dis_speed),
          rot_speed(other.rot_speed),
          scale_speed(other.scale_speed) {
    }

    Gameobject& operator=(Gameobject&& other) noexcept {
        if (this != &other) {
            vertices = std::move(other.vertices);
            vertex_buffer = std::move(other.vertex_buffer);

            indices = std::move(other.indices);
            index_buffer = std::move(other.index_buffer);

            position = other.position;
            scale = other.scale;
            rotation = other.rotation;
            dis_speed = other.dis_speed;
            rot_speed = other.rot_speed;
            scale_speed = other.scale_speed;
        }
        return *this;
    }


    // Initializes the object
    virtual void start(VmaAllocator& vma_allocator, vk::raii::Device& logical_device, QueuePool& queue_pool){
        loadBuffers(vma_allocator, logical_device, queue_pool);
    }

    // Updates the object
    virtual void update(const float dtime){
        if(dis_speed.length() > 0){
            position += dis_speed * dtime;
            dirty_model = true;
        }

        if(rot_speed.length() > 0){
            rotation += rot_speed * dtime;
            dirty_model = true;
        }

        if(scale_speed.length() > 0){
            scale += scale_speed * dtime;
            dirty_model = true;
        }


    }


    uint32_t getVertexSize(){
        return vertices.size();
    }

    uint32_t getIndexSize(){
        return indices.size();
    }

    const vk::Buffer& getVertexBuffer(){
        return vertex_buffer.buffer;
    }

    const vk::Buffer& getIndexBuffer(){
        return index_buffer.buffer;
    }

    const glm::mat4 &getModelMat(){
        if(dirty_model){
            // Recalculate model matrix when needed
            dirty_model = false;
            model = glm::mat4(1);
            model = glm::translate(model, position);
            if(rotation.x != 0.0){
                model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
            }
            if(rotation.y != 0.0){
                model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0,1, 0));
            }
            if(rotation.z != 0.0){
                model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
            }
            model = glm::scale(model, scale);
        }
        return model;
    }

protected:
    std::vector<Vertex> vertices;
    AllocatedBuffer vertex_buffer;
    std::vector<uint32_t> indices;
    AllocatedBuffer index_buffer;

    // Spatial information
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::mat4 model;
    bool dirty_model = true; // Tracks whether model needs to be recalculated or not

    // Velocity information
    glm::vec3 dis_speed; // Displacement speed
    glm::vec3 rot_speed; // Rotational speed
    glm::vec3 scale_speed; // Scale change speed

    // loads the necessary buffers for the object
    void loadBuffers(VmaAllocator &vma_allocator, vk::raii::Device &logical_device, QueuePool &queue_pool){
        vk::DeviceSize vertex_size = sizeof(Vertex) * vertices.size();
        vk::DeviceSize index_size = sizeof(uint32_t) * indices.size();
        vk::DeviceSize total_size = vertex_size + index_size;

        AllocatedBuffer staging_buffer = Device::createBuffer(total_size, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, "vertex+indices staging buffer", vma_allocator);

        void * data;
        vmaMapMemory(vma_allocator, staging_buffer.allocation, &data);
        memcpy(data, vertices.data(), (size_t)vertex_size);
        memcpy((char *)data + vertex_size, indices.data(), (size_t)index_size);
        vmaUnmapMemory(vma_allocator, staging_buffer.allocation);

        vertex_buffer = Device::createBuffer(vertex_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal, "vertex buffer", vma_allocator);

        Device::copyBuffer(staging_buffer, vertex_buffer, vertex_size, logical_device, queue_pool, 0);

        index_buffer = Device::createBuffer(index_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal, "index buffer", vma_allocator);

        Device::copyBuffer(staging_buffer, index_buffer, index_size, logical_device, queue_pool, vertex_size);
    }

};