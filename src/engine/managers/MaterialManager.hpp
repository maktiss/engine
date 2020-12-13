#pragma once

#include "ResourceManagerBase.hpp"

#include "engine/graphics/materials/SimpleMaterial.hpp"


namespace Engine::Managers {
class MaterialManager : public ResourceManagerBase<MaterialManager, Engine::Graphics::Materials::SimpleMaterial> {
public:
	static void postCreate(Handle handle) {

	}
	
	static void update(Handle handle) {

	}

private:
	MaterialManager() {};
};
}