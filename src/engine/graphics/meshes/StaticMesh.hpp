#pragma once

#include "MeshBase.hpp"

#include <array>
#include <string>
#include <tuple>
#include <vector>


namespace Engine::Graphics::Meshes {
class StaticMesh : public MeshBase<StaticMesh> {
public:
	// TODO: test performance impact due to elements of a tuple being reversed
	VERTEX_LAYOUT(glm::vec3, glm::vec2);
	VERTEX_FORMATS(vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat);

public:
	StaticMesh() {
		getVertexBuffer() = {
			{ { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f } },
			{ { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f } },
			{ { 0.0f, -0.5f, 0.5f }, { 0.5f, 1.0f } },
		};

		getIndexBuffer() = { 0, 1, 2 };
	}

	static const std::string getMeshTypeString() {
		return "MESH_TYPE_STATIC";
	}
};
} // namespace Engine::Graphics