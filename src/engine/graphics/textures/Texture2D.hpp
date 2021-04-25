#pragma once

#include "TextureBase.hpp"


namespace Engine {
class Texture2D : public TextureBase<Texture2D, vk::ImageType::e2D> {};
} // namespace Engine