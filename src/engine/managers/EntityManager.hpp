#pragma once

#include "EntityManagerBase.hpp"

#include "engine/components/Camera.hpp"
#include "engine/components/Model.hpp"
#include "engine/components/Transform.hpp"

#include <spdlog/spdlog.h>


namespace Engine::Managers {
class EntityManager : public EntityManagerBase<Components::Transform, Components::Model, Components::Camera> {
public:
	static int init() {
		spdlog::info("Initializing EntityManager...");
		return EntityManagerBase::init();
	}

private:
	EntityManager() {
	}
};
} // namespace Engine::Managers