#pragma once

#include "entt.hpp"
#include "core/base.hpp"
#include "scene/scene.hpp"

namespace fluent
{
class Entity
{
private:
    entt::entity m_handle{ entt::null };
    Scene*       m_scene = nullptr;

public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene);
    Entity(const Entity& other) = default;

    template <typename T, typename... Args>
    T& add_component(Args&&... args)
    {
        FT_ASSERT(!has_component<T>() && "Entity already has component!");
        T& component = m_scene->m_registry.emplace<T>(m_handle, std::forward<Args>(args)...);
        return component;
    }

    template <typename T>
    T& get_component()
    {
        FT_ASSERT(has_component<T>() && "Entity does not have component!");
        return m_scene->m_registry.get<T>(m_handle);
    }

    template <typename T>
    b32 has_component()
    {
        return b32(m_scene->m_registry.try_get<T>(m_handle) != nullptr);
    }

    template <typename T>
    void remove_component()
    {
        FT_ASSERT(has_component<T>() && "Entity does not have component!");
        m_scene->m_registry.remove<T>(m_handle);
    }

    operator bool() const
    {
        return m_handle != entt::null;
    }

    operator entt::entity() const
    {
        return m_handle;
    }

    operator u32() const
    {
        return ( u32 ) m_handle;
    }

    b32 operator==(const Entity& other) const
    {
        return b32(m_handle == other.m_handle && m_scene == other.m_scene);
    }

    b32 operator!=(const Entity& other) const
    {
        return b32(!(*this == other));
    }
};
} // namespace fluent
