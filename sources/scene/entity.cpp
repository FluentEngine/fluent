#include "scene/entity.hpp"

namespace fluent
{
Entity::Entity(entt::entity handle, Scene* scene) : m_handle(handle), m_scene(scene)
{
}
} // namespace fluent
