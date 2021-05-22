#pragma once

#include "MeshBase.hpp"

#include <array>
#include <string>
#include <tuple>
#include <vector>


namespace Engine {
class TerrainMesh : public MeshBase<TerrainMesh> {
public:
	VERTEX_LAYOUT(glm::vec3);
	VERTEX_FORMATS(vk::Format::eR32G32B32Sfloat);


public:
	inline bool usesTessellation() override {
		return true;
	}


	static const std::string getMeshTypeString() {
		return "MESH_TYPE_TERRAIN";
	}
};
} // namespace Engine