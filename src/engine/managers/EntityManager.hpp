#pragma once

#include "EntityManagerBase.hpp"

#include "engine/components/Components.hpp"


namespace Engine {
class EntityManager
	: public EntityManagerBase<TransformComponent, ModelComponent, ScriptComponent, LightComponent, CameraComponent> {
public:
	static int init();

private:
	EntityManager() {
	}
};
} // namespace Engine