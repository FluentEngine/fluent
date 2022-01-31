#pragma once

#include "entt.hpp"
#include "core/base.hpp"

namespace fluent
{
class Entity;

class Scene
{
    friend class Entity;

private:
public:
    entt::registry m_registry;

    Scene()  = default;
    ~Scene() = default;

    void create_entity(Entity& entity, const std::string& name);
    void destroy_entity(Entity& entity);
};
} // namespace fluent
