#pragma once

#include "engine/renderers/RendererBase.hpp"


namespace Engine {
class ComputeRendererBase : public RendererBase {
public:
	ComputeRendererBase(uint inputCount = 0, uint outputCount = 0) : RendererBase(inputCount, outputCount) {
	}
};
} // namespace Engine