#include "TerrainRenderer.hpp"

#include "engine/managers/GlobalStateManager.hpp"

#include "engine/utils/Generator.hpp"

#include <glm/gtx/transform.hpp>

#include <queue>


namespace Engine {
int TerrainRenderer::init() {
	tileMeshHandle = MeshManager::createObject<TerrainMesh>("generated_terrain_tile");
	Generator::quad(tileMeshHandle);

	return 0;
}


void TerrainRenderer::drawTerrain(vk::PipelineLayout pipelineLayout, const vk::Pipeline* pPipelines,
								  const vk::CommandBuffer* pSecondaryCommandBuffers, const uint materialDescriptorSetId,
								  glm::vec2 cameraCenter, const Frustum& frustum) {

	const auto& commandBuffer = pSecondaryCommandBuffers[0];


	const auto& terrainState = GlobalStateManager::get<TerrainState>();

	const auto& meshInfo	 = MeshManager::getMeshInfo(tileMeshHandle);
	const auto& materialInfo = MaterialManager::getMaterialInfo(terrainState.materialHandles[0]);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pPipelines[terrainState.shaderHandles[0].getIndex()]);

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, materialDescriptorSetId, 1,
									 &materialInfo.descriptorSet, 0, nullptr);

	BoundingBox boundingBox {};
	boundingBox.points[0] = glm::vec3(-0.5f, 0.0f, -0.5f);
	boundingBox.points[1] = glm::vec3(-0.5f, 0.0f, 0.5f);
	boundingBox.points[2] = glm::vec3(0.5f, 0.0f, -0.5f);
	boundingBox.points[3] = glm::vec3(0.5f, 0.0f, 0.5f);
	boundingBox.points[4] = glm::vec3(-0.5f, terrainState.maxHeight, -0.5f);
	boundingBox.points[5] = glm::vec3(-0.5f, terrainState.maxHeight, 0.5f);
	boundingBox.points[6] = glm::vec3(0.5f, terrainState.maxHeight, -0.5f);
	boundingBox.points[7] = glm::vec3(0.5f, terrainState.maxHeight, 0.5f);

	auto transformMatrix = glm::mat4(1.0f);

	int maxLod = std::log2(terrainState.size) - 1;


	struct Node {
		int lod;
		int size;
		glm::vec2 pos;
	};

	std::queue<Node> nodes {};
	nodes.push({ maxLod, terrainState.size, glm::vec2(0.0f) });


	vk::DeviceSize offsets[] = { 0 };

	while (!nodes.empty()) {
		auto node = nodes.front();
		nodes.pop();

		transformMatrix[3] = glm::vec4(node.pos.x, 0.0, node.pos.y, 1.0f);

		transformMatrix[0][0] = node.size;
		transformMatrix[1][1] = node.size;
		transformMatrix[2][2] = node.size;

		int offset = node.size / 2;

		glm::vec2 localPos	   = cameraCenter - node.pos;
		glm::vec2 closestPoint = localPos;

		closestPoint = glm::max(closestPoint, glm::vec2(-offset));
		closestPoint = glm::min(closestPoint, glm::vec2(offset));

		auto getLod = [](float distance) {
			return std::max(0.0f, std::log2(0.25f * distance));
		};

		int preferredLod = getLod(glm::distance(localPos, closestPoint));


		if (frustum.intersects(boundingBox.transform(transformMatrix))) {
			if (node.lod > preferredLod) {
				int newSize	  = node.size / 2;
				int newOffset = newSize / 2;

				int newLod = node.lod - 1;

				nodes.push({ newLod, newSize, node.pos + glm::vec2(-newOffset, -newOffset) });
				nodes.push({ newLod, newSize, node.pos + glm::vec2(-newOffset, +newOffset) });
				nodes.push({ newLod, newSize, node.pos + glm::vec2(+newOffset, -newOffset) });
				nodes.push({ newLod, newSize, node.pos + glm::vec2(+newOffset, +newOffset) });

			} else {
				glm::vec2 localPosX = localPos;

				glm::vec4 factors = glm::vec4(1.0f);

				if (std::abs(localPos.x - node.size) < std::abs(localPos.x + node.size)) {
					localPosX.x = localPos.x - node.size;
					factors.x += 1.0f;
				} else {
					localPosX.x = localPos.x + node.size;
					factors.z += 1.0f;
				}

				glm::vec2 localPosY = localPos;

				if (std::abs(localPos.y - node.size) < std::abs(localPos.y + node.size)) {
					localPosY.y = localPos.y - node.size;
					factors.w += 1.0f;
				} else {
					localPosY.y = localPos.y + node.size;
					factors.y += 1.0f;
				}


				glm::vec2 closestPointX = localPosX;
				glm::vec2 closestPointY = localPosY;

				closestPointX = glm::max(closestPointX, glm::vec2(-offset));
				closestPointX = glm::min(closestPointX, glm::vec2(offset));

				closestPointY = glm::max(closestPointY, glm::vec2(-offset));
				closestPointY = glm::min(closestPointY, glm::vec2(offset));

				int lodX = getLod(glm::distance(localPosX, closestPointX));
				int lodY = getLod(glm::distance(localPosY, closestPointY));


				if ((node.lod - 1) != lodX) {
					factors.x = 1.0f;
					factors.z = 1.0f;
				}
				if ((node.lod - 1) != lodY) {
					factors.y = 1.0f;
					factors.w = 1.0f;
				}

				commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eAll, 0, 64, &transformMatrix);
				commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eAll, 64, 16, &factors);
				commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eAll, 80, 4, &node.lod);

				auto vertexBuffer = meshInfo.vertexBuffer.getVkBuffer();
				commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

				auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
				commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

				commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
			}
		}
	}
}
} // namespace Engine