#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

#include <glm/glm.hpp>

#include <map>
#include <typeindex>
#include <vector>


#define VERTEX_LAYOUT(...)                  \
	using Vertex = std::tuple<__VA_ARGS__>; \
	std::vector<Vertex> vertexBuffer;

#define VERTEX_FORMATS(...)                       \
	static constexpr auto getAttributeFormats() { \
		return std::array { __VA_ARGS__ };        \
	}


namespace Engine::Graphics::Meshes {
template <typename MeshType>
class MeshBase {
public:
	std::vector<uint32_t> indexBuffer;


public:
	inline auto& getVertexBuffer() {
		return static_cast<MeshType*>(this)->vertexBuffer;
	}

	inline auto& getIndexBuffer() {
		return indexBuffer;
	}

	inline auto getVertexBufferSize() {
		return getVertexBuffer().size() * getVertexSize();
	}

	inline auto getIndexBufferSize() {
		return getIndexBuffer().size() * sizeof(uint32_t);
	}

	static constexpr auto getVertexSize() {
		return sizeof(typename MeshType::Vertex);
	}

	static constexpr auto getNumAttributes() {
		return std::tuple_size<typename MeshType::Vertex>::value;
	}

	template <uint Index>
	static constexpr auto getAttributeOffset() {
		typename MeshType::Vertex vertex;
		return static_cast<uint32_t>(reinterpret_cast<char*>(&std::get<Index>(vertex)) -
									 reinterpret_cast<char*>(&vertex));
	}

	// TODO: should MeshManager be responsible for populating vulkan related structures?
	static constexpr auto getVertexInputBindingDescriptions() {
		std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions(1);
		vertexInputBindingDescriptions[0].binding	= 0;
		vertexInputBindingDescriptions[0].stride	= getVertexSize();
		vertexInputBindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;

		return vertexInputBindingDescriptions;
	}

	static constexpr auto getVertexInputAttributeDescriptions() {
		return getVertexInputAttributeDescriptionsImpl(std::make_index_sequence<getNumAttributes()>());
	}

private:
	template <std::size_t... Indices>
	static constexpr auto getVertexInputAttributeDescriptionsImpl(std::index_sequence<Indices...>) {
		std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions(getNumAttributes());

		static_assert(MeshType::getAttributeFormats().size() == getNumAttributes());

		typename MeshType::Vertex vertex;

		((vertexInputAttributeDescriptions[Indices].binding = 0), ...);
		((vertexInputAttributeDescriptions[Indices].location = Indices), ...);
		((vertexInputAttributeDescriptions[Indices].format = MeshType::getAttributeFormats()[Indices]), ...);
		((vertexInputAttributeDescriptions[Indices].offset = getAttributeOffset<Indices>()), ...);

		return vertexInputAttributeDescriptions;
	}
};
} // namespace Engine::Graphics
