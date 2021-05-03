#pragma once

#include "engine/renderers/RendererBase.hpp"


namespace Engine {
class TransferRendererBase : public RendererBase {
public:
	TransferRendererBase(uint inputCount = 0, uint outputCount = 0) : RendererBase(inputCount, outputCount) {
	}


	int init() override;

	virtual void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
											   double dt) = 0;

	virtual int render(const vk::CommandBuffer* pPrimaryCommandBuffers,
					   const vk::CommandBuffer* pSecondaryCommandBuffers, const vk::QueryPool& timestampQueryPool,
					   double dt) override;
};
} // namespace Engine