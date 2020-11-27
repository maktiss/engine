#pragma once

#include "MeshBase.hpp"

#include <array>
#include <tuple>
#include <vector>


namespace Engine::Graphics {
class Mesh : public MeshBase<Mesh> {
public:
	// TODO: test performance impact due to elements of a tuple being reversed
	// using Vertex = std::tuple<glm::vec3, glm::vec2>;
	VERTEX_LAYOUT(glm::vec3, glm::vec2);
	VERTEX_FORMATS(vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat);

	// struct Vertex {
	// 	glm::vec3 position;
	// 	glm::vec2 texCoord;
	// };

	// std::vector<Vertex> vertexData;

public:
	Mesh() {
		getVertexBuffer() = {
			{ { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f } },
			{ { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f } },
			{ { 0.0f, -0.5f, 0.5f }, { 0.5f, 1.0f } },
		};

		getIndexBuffer() = { 0, 1, 2 };
	}


	// static constexpr auto getAttributeFormats() {
	// 	return std::array { vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat };
	// }

	// static constexpr auto getAttributeOffsets() {
	// 	return std::array { offsetof(Vertex, position), offsetof(Vertex, texCoord) };
	// }
};
} // namespace Engine::Graphics