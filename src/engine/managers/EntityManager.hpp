#pragma once

#include "EntityManagerBase.hpp"

#include "engine/components/Camera.hpp"
#include "engine/components/Light.hpp"
#include "engine/components/Model.hpp"
#include "engine/components/Script.hpp"
#include "engine/components/Transform.hpp"


namespace Engine::Managers {
class EntityManager : public EntityManagerBase<Components::Transform, Components::Model, Components::Script,
											   Components::Light, Components::Camera> {
public:
	static int init();

private:
	EntityManager() {
	}
};
} // namespace Engine::Managers