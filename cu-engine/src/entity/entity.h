#pragma once 

#include <vector>
#include <memory>

class Entity {
public:
    static std::shared_ptr<Entity> create();
    void ready(); 
    void update();
    void add_child(std::shared_ptr<Entity> p_entity);
    void free();

    std::string id;
private:
    Entity* parent = nullptr;
    std::vector<std::shared_ptr<Entity>> children = {};
};