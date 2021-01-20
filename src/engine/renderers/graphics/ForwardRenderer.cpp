#include "ForwardRenderer.hpp"

#include "engine/managers/EntityManager.hpp"

#include <glm/glm.hpp>


namespace Engine::Renderers::Graphics {
int ForwardRenderer::init() {
	spdlog::info("Initializing ForwardRenderer...");

	assert(vkDevice != vk::Device());
	assert(outputSize != vk::Extent2D());


	uniformBuffers.resize(3);
	uniformBuffers[0].init(vkDevice, vmaAllocator, 4);
	uniformBuffers[1].init(vkDevice, vmaAllocator, 256);
	uniformBuffers[2].init(vkDevice, vmaAllocator, 16);


	return GraphicsRendererBase::init();
}


void ForwardRenderer::recordCommandBuffer(double dt, vk::CommandBuffer& commandBuffer) {
	// commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkGraphicsPipelines[1]);

	for (uint setIndex = 0; setIndex < uniformBuffers.size(); setIndex++) {
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipelineLayout, setIndex, 1,
										 &uniformBuffers[setIndex].getVkDescriptorSet(), 0, nullptr);
	}

	struct {
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;

		glm::mat4 invViewMatrix;
		glm::mat4 invProjectionMatrix;
	} cameraBlock;

	Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Camera>(
		[&cameraBlock](auto& transform, auto& camera) {
			glm::vec4 viewVector   = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
			viewVector			   = glm::rotate(transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * viewVector;
			viewVector			   = glm::rotate(transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * viewVector;
			viewVector			   = glm::rotate(transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * viewVector;
			cameraBlock.viewMatrix = glm::lookAtLH(
				transform.position, transform.position + glm::vec3(viewVector.x, viewVector.y, viewVector.z),
				glm::vec3(0.0f, 1.0f, 0.0f));

			cameraBlock.projectionMatrix = camera.getProjectionMatrix();
		});

	cameraBlock.invViewMatrix		= glm::inverse(cameraBlock.viewMatrix);
	cameraBlock.invProjectionMatrix = glm::inverse(cameraBlock.projectionMatrix);

	uniformBuffers[1].update(&cameraBlock, sizeof(cameraBlock));


	Engine::Managers::EntityManager::forEach<Engine::Components::Transform, Engine::Components::Model>(
		[&commandBuffer, pipelineLayout = vkPipelineLayout, &graphicsPipelines = vkPipelines](auto& transform,
																							  auto& model) {
			const auto& meshInfo	 = Engine::Managers::MeshManager::getMeshInfo(model.meshHandles[0]);
			const auto& materialInfo = Engine::Managers::MaterialManager::getMaterialInfo(model.materialHandles[0]);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
									   graphicsPipelines[model.shaderHandles[0].getIndex()]);


			auto transformMatrix = transform.getTransformMatrix();
			commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eAll, 0, 64, &transformMatrix);

			auto vertexBuffer		 = meshInfo.vertexBuffer.getVkBuffer();
			vk::DeviceSize offsets[] = { 0 };
			commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);

			auto indexBuffer = meshInfo.indexBuffer.getVkBuffer();
			commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 3, 1,
											 &materialInfo.descriptorSet, 0, nullptr);

			commandBuffer.drawIndexed(meshInfo.indexCount, 1, 0, 0, 0);
		});
}
} // namespace Engine::Renderers::Graphics