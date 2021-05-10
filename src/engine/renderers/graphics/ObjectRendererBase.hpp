#pragma once

#include "GraphicsRendererBase.hpp"

#include "engine/graphics/Frustum.hpp"

#include <map>
#include <vector>


namespace Engine {
class ObjectRendererBase : public GraphicsRendererBase {
private:
	struct RenderInfo {
		uint pipelineIndex;

		vk::DescriptorSet material;

		vk::Buffer vertexBuffer;
		vk::Buffer indexBuffer;
		uint indexCount;

		glm::mat4 transformMatrix;
	};

	std::vector<RenderInfo> renderInfoCache {};

	std::map<uint64_t, uint> renderInfoIndices {};


public:
	ObjectRendererBase(uint inputCount, uint outputCount) : GraphicsRendererBase(inputCount, outputCount) {
	}


protected:
	void drawObjects(const vk::CommandBuffer* pSecondaryCommandBuffers, const Frustum& frustum);
};
} // namespace Engine