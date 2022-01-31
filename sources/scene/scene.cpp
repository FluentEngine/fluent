#include "scene/entity.hpp"
#include "scene/scene.hpp"
#include "scene/components.hpp"

namespace fluent
{
Entity::Entity(entt::entity handle, Scene* scene) : m_handle(handle), m_scene(scene)
{
}

void Scene::create_entity(Entity& entity, const std::string& name)
{
    entity = Entity(m_registry.create(), this);
    entity.add_component<TransformComponent>();
}

void Scene::destroy_entity(Entity& entity)
{
    m_registry.destroy(entity);
}

} // namespace fluent