#pragma once

#include "engine/renderers/RendererBase.hpp"


namespace Engine {
class TransferRendererBase : public RendererBase {
public:
	TransferRendererBase(uint inputCount, uint outputCount) : RendererBase(inputCount, outputCount) {
	}


	int init() override;

	virtual void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) = 0;

	virtual int render(const vk::CommandBuffer* pPrimaryCommandBuffers,
					   const vk::CommandBuffer* pSecondaryCommandBuffers, const vk::QueryPool& timestampQueryPool,
					   double dt) override;
};
} // namespace Engine