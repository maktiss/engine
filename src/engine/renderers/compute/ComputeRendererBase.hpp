#pragma once

#include "engine/renderers/RendererBase.hpp"


namespace Engine {
class ComputeRendererBase : public RendererBase {
public:
	ComputeRendererBase(uint inputCount, uint outputCount) : RendererBase(inputCount, outputCount) {
	}
};
} // namespace Engine