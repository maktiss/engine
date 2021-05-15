#include "ReflectionRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine {
int ReflectionRenderer::init() {
	spdlog::info("Initializing ReflectionRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	// TODO: move to Engine::Generator

	mesh = MeshManager::createObject(0, "generated_screen_triangle");
	mesh.apply([](auto& mesh) {
		auto& vertexBuffer = mesh.getVertexBuffer();
		vertexBuffer.resize(8);

		std::get<0>(vertexBuffer[0]) = glm::vec3(-1.0f, 1.0f, 0.5f);
		std::get<0>(vertexBuffer[1]) = glm::vec3(-1.0f, -3.0f, 0.5f);
		std::get<0>(vertexBuffer[2]) = glm::vec3(3.0f, 1.0f, 0.5f);

		std::get<1>(vertexBuffer[0]) = glm::vec2(0.0f, 0.0f);
		std::get<1>(vertexBuffer[1]) = glm::vec2(0.0f, 2.0f);
		std::get<1>(vertexBuffer[2]) = glm::vec2(2.0f, 0.0f);

		auto& indexBuffer = mesh.getIndexBuffer();

		indexBuffer = {
			0,
			1,
			2,
		};
	});
	mesh.update();

	shaderHandle = GraphicsShaderManager::getHandle<ReflectionShader>(mesh);


	if (GraphicsRendererBase::init()) {
		return 1;
	}

	descriptorSetArrays[0].updateImage(0, 1, 0, inputVkSamplers[0], inputVkImageViews[0]);
	descriptorSetArrays[0].updateImage(0, 2, 0, inputVkSamplers[1], inputVkImageViews[1]);
	descriptorSetArrays[0].updateImage(0, 3, 0, inputVkSamplers[2], inputVkImageViews[2]);

	return 0;
}


void ReflectionRenderer::recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers,
													   uint layerIndex, double dt) {
	const auto& commandBuffer = pSecondaryCommandBuffers[0];

	bindDescriptorSets(commandBuffer, vk::PipelineBindPoint::eGraphics);


	ParamBlock paramBlock;

	EntityManager::forEach<TransformComponent, CameraComponent>([&](auto& transform, auto& camera) {
		// TODO: Check if active camera
		glm::vec4 viewVector = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		viewVector			 = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
		viewVector			 = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;

		auto viewMatrix = glm::lookAtLH(glm::vec3(0.0f), glm::vec3(viewVector.x, viewVector.y, viewVector.z),
										glm::vec3(0.0f, 1.0f, 0.0f));

		auto projectionMatrix = camera.getProjectionMatrix();

		paramBlock.invViewMatrix	   = glm::inverse(viewMatrix);
		paramBlock.invProjectionMatrix = glm::inverse(projectionMatrix);
	});

	// EntityManager::forEach<CameraComponent>([&](const auto& camera) {
	// 	// FIXME: active camera
	// 	invProjectionMatrix = glm::inverse(camera.getProjectionMatrix());
	// });

	descriptorSetArrays[0].updateBuffer(0, 0, &paramBlock, sizeof(paramBlock));


	const auto& meshInfo = MeshManager::getMeshInfo(mesh);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipelines[shaderHandle.getIndex()]);


	auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
	vk::DeviceSize offsets[] = { 0 };
	commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

	auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
	commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

	commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
}
} // namespace Engine