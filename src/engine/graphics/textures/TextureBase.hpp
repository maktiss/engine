#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include <vector>


namespace Engine {
template <typename DerivedTextureType, vk::ImageType ImageType>
class TextureBase {
public:
	vk::Extent3D size {};
	uint layerCount = 1;

	vk::Format format {};

	bool useMipMapping {};

	vk::ImageUsageFlags usage {};
	vk::ImageAspectFlags imageAspect = vk::ImageAspectFlagBits::eColor;

	vk::ImageCreateFlags flags {};


private:
	std::vector<uint8_t> pixelData;


public:
	inline auto& getPixelData() {
		return pixelData;
	}

	inline void setPixelData(void* data, uint64_t size) {
		pixelData.resize(size);
		memcpy(pixelData.data(), data, size);
	}

	// Releases pixel data from RAM
	inline void releasePixelData() {
		pixelData = std::vector<uint8_t>();
	}

	inline constexpr vk::ImageType getImageType() {
		return ImageType;
	}
};
} // namespace Engine