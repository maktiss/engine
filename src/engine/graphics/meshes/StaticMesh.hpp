#pragma once

#include "MeshBase.hpp"

#include <array>
#include <string>
#include <tuple>
#include <vector>


namespace Engine {
class StaticMesh : public MeshBase<StaticMesh> {
public:
	// TODO: test performance impact due to elements of a tuple being reversed
	VERTEX_LAYOUT(glm::vec3, glm::vec2, glm::vec3, glm::vec3, glm::vec3);
	VERTEX_FORMATS(vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat, vk::Format::eR32G32B32Sfloat,
				   vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Sfloat);

public:
	static const std::string getMeshTypeString() {
		return "MESH_TYPE_STATIC";
	}
};
} // namespace Engine