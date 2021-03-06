#include "TransferRendererBase.hpp"

#include <spdlog/spdlog.h>


namespace Engine {
int TransferRendererBase::init() {
	return 0;
}

int TransferRendererBase::render(const vk::CommandBuffer* pPrimaryCommandBuffers,
								 const vk::CommandBuffer* pSecondaryCommandBuffers,
								 const vk::QueryPool& timestampQueryPool, double dt) {

	currentFrameInFlight = (currentFrameInFlight + 1) % framesInFlightCount;

	for (uint layerIndex = 0; layerIndex < getLayerCount(); layerIndex++) {
		currentLayer = layerIndex;

		vk::CommandBufferBeginInfo commandBufferBeginInfo {};

		auto& commandBuffer = pPrimaryCommandBuffers[layerIndex];

		auto result = commandBuffer.begin(&commandBufferBeginInfo);
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to record command buffer. Error code: {} ({})", result, vk::to_string(result));
			return 1;
		}

		auto queryIndex = layerIndex * 2;

		commandBuffer.resetQueryPool(timestampQueryPool, queryIndex, 2);
		commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, timestampQueryPool, queryIndex);


		vk::CommandBufferInheritanceInfo commandBufferInheritanceInfo {};
		commandBufferBeginInfo.pInheritanceInfo = &commandBufferInheritanceInfo;

		auto pSecondaryCommandBuffersForLayer = pSecondaryCommandBuffers + layerIndex * threadCount;

		for (uint threadIndex = 0; threadIndex < threadCount; threadIndex++) {
			result = pSecondaryCommandBuffersForLayer[threadIndex].begin(&commandBufferBeginInfo);
			if (result != vk::Result::eSuccess) {
				spdlog::error("Failed to record secondary command buffer. Error code: {} ({})", result,
							  vk::to_string(result));
				return 1;
			}
		}


		recordSecondaryCommandBuffers(pSecondaryCommandBuffersForLayer, dt);


		for (uint threadIndex = 0; threadIndex < threadCount; threadIndex++) {
			result = pSecondaryCommandBuffersForLayer[threadIndex].end();
			if (result != vk::Result::eSuccess) {
				spdlog::error("Failed to record secondary command buffer. Error code: {} ({})", result,
							  vk::to_string(result));
				return 1;
			}
		}

		commandBuffer.executeCommands(threadCount, pSecondaryCommandBuffersForLayer);


		commandBuffer.writeTimestamp(vk::PipelineStageFlagBits::eTransfer, timestampQueryPool, queryIndex + 1);

		result = commandBuffer.end();
		if (result != vk::Result::eSuccess) {
			spdlog::error("Failed to record command buffer. Error code: {} ({})", result, vk::to_string(result));
			return 1;
		}
	}

	return 0;
}
} // namespace Engine