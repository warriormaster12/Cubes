#include "entity.h"
#include "logger.h"

std::shared_ptr<Entity> Entity::create(){
    std::shared_ptr<Entity> entity = std::make_shared<Entity>();
    return entity;
}

void Entity::add_child(std::shared_ptr<Entity> p_entity){
    p_entity->parent = this;
    children.push_back(p_entity);
}

void Entity::ready() {
    for (int i = 0; i < children.size(); ++i) {
        children[i]->ready();
    }
    ENGINE_INFO("Node: {} ready", id);
}

void Entity::update() {
    for (int i = 0; i < children.size(); ++i) {
        children[i]->update();
    }
}

void Entity::free(){
    
}