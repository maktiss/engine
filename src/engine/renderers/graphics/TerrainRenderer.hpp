#pragma once

#include "engine/graphics/Frustum.hpp"

#include "engine/managers/MeshManager.hpp"

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include <map>
#include <vector>


namespace Engine {
class TerrainRenderer {
private:
	MeshManager::Handle tileMeshHandle {};


public:
	int init();


	void drawTerrain(vk::PipelineLayout pipelineLayout, const vk::Pipeline* pPipelines,
					 const vk::CommandBuffer* pSecondaryCommandBuffers, const uint materialDescriptorSetId,
					 glm::vec2 cameraCenter, const Frustum& frustum);
};
} // namespace Engine