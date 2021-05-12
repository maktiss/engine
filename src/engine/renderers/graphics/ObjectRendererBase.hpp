#pragma once

#include "GraphicsRendererBase.hpp"

#include "engine/graphics/Frustum.hpp"

#include "engine/utils/ThreadPool.hpp"

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

	std::vector<std::vector<RenderInfo>> renderInfoCachePerThread {};
	std::vector<std::map<uint64_t, uint>> renderInfoIndicesPerThread {};

	ThreadPool threadPool {};

	struct DrawObjectsThreadInfo {
		ObjectRendererBase* renderer;
		const vk::CommandBuffer* pSecondaryCommandBuffers;

		Frustum frustum;

		uint fragmentIndex;
		uint fragmentCount;
	};
	std::vector<DrawObjectsThreadInfo> drawObjectsThreadInfos {};


public:
	ObjectRendererBase(uint inputCount, uint outputCount) : GraphicsRendererBase(inputCount, outputCount) {
	}

	int init() override;


protected:
	void drawObjects(const vk::CommandBuffer* pSecondaryCommandBuffers, const Frustum& frustum);


private:
	static void drawObjectsThreadFunc(uint threadIndex, void* pData);
};
} // namespace Engine