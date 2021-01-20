#pragma once

#include "engine/renderers/RendererBase.hpp"


namespace Engine::Renderers::Compute {
class ComputeRendererBase : public Engine::Renderers::RendererBase {
public:
	ComputeRendererBase(uint inputCount = 0, uint outputCount = 0) :
		Engine::Renderers::RendererBase(inputCount, outputCount) {
	}
};
} // namespace Engine::Renderers::Compute