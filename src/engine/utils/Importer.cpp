#include "Importer.hpp"

#include <spdlog/spdlog.h>

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "stb_image.h"


namespace Engine {
Assimp::Importer Importer::assimpImporter {};


int Importer::importMesh(std::string filename, std::vector<MeshManager::Handle>& meshHandles) {
	spdlog::info("Importing mesh '{}'...", filename);

	const aiScene* scene =
		assimpImporter.ReadFile(filename, aiProcess_Triangulate | aiProcess_CalcTangentSpace |
											  aiProcess_GenBoundingBoxes | aiProcess_MakeLeftHanded |
											  aiProcess_GenUVCoords | aiProcess_TransformUVCoords | aiProcess_FlipUVs);

	if (!scene) {
		spdlog::error("Failed to import '{}'", filename);
		return 1;
	}

	meshHandles.resize(scene->mNumMeshes);

	for (uint i = 0; i < meshHandles.size(); i++) {
		auto assimpMesh = scene->mMeshes[i];

		auto& meshHandle = meshHandles[i];
		meshHandle		 = MeshManager::createObject(0, assimpMesh->mName.C_Str());

		meshHandle.apply([&assimpMesh](auto& mesh) {
			for (int i = 0; i < 8; i++) {
				mesh.boundingBox.points[i].x	  = i & 1 ? assimpMesh->mAABB.mMax.x : assimpMesh->mAABB.mMin.x;
				mesh.boundingBox.points[i].y = i & 2 ? assimpMesh->mAABB.mMax.y : assimpMesh->mAABB.mMin.y;
				mesh.boundingBox.points[i].z = i & 4 ? assimpMesh->mAABB.mMax.z : assimpMesh->mAABB.mMin.z;
			}


			auto& vertexBuffer = mesh.getVertexBuffer();
			vertexBuffer.resize(assimpMesh->mNumVertices);

			for (int vIndex = 0; vIndex < assimpMesh->mNumVertices; vIndex++) {
				std::get<0>(vertexBuffer[vIndex]).x = assimpMesh->mVertices[vIndex].x;
				std::get<0>(vertexBuffer[vIndex]).y = assimpMesh->mVertices[vIndex].y;
				std::get<0>(vertexBuffer[vIndex]).z = assimpMesh->mVertices[vIndex].z;

				if (assimpMesh->mTextureCoords[0] != nullptr) {
					std::get<1>(vertexBuffer[vIndex]).x = assimpMesh->mTextureCoords[0][vIndex].x;
					std::get<1>(vertexBuffer[vIndex]).y = assimpMesh->mTextureCoords[0][vIndex].y;
				} else {
					std::get<1>(vertexBuffer[vIndex]).x = 0.0f;
					std::get<1>(vertexBuffer[vIndex]).y = 0.0f;
				}

				std::get<2>(vertexBuffer[vIndex]).x = assimpMesh->mNormals[vIndex].x;
				std::get<2>(vertexBuffer[vIndex]).y = assimpMesh->mNormals[vIndex].y;
				std::get<2>(vertexBuffer[vIndex]).z = assimpMesh->mNormals[vIndex].z;

				std::get<3>(vertexBuffer[vIndex]).x = assimpMesh->mTangents[vIndex].x;
				std::get<3>(vertexBuffer[vIndex]).y = assimpMesh->mTangents[vIndex].y;
				std::get<3>(vertexBuffer[vIndex]).z = assimpMesh->mTangents[vIndex].z;

				std::get<4>(vertexBuffer[vIndex]).x = -assimpMesh->mBitangents[vIndex].x;
				std::get<4>(vertexBuffer[vIndex]).y = -assimpMesh->mBitangents[vIndex].y;
				std::get<4>(vertexBuffer[vIndex]).z = -assimpMesh->mBitangents[vIndex].z;
			}

			auto& indexBuffer = mesh.getIndexBuffer();
			indexBuffer.resize(assimpMesh->mNumFaces * 3);

			for (int fIndex = 0; fIndex < assimpMesh->mNumFaces; fIndex++) {
				indexBuffer[fIndex * 3 + 0] = assimpMesh->mFaces[fIndex].mIndices[0];
				indexBuffer[fIndex * 3 + 1] = assimpMesh->mFaces[fIndex].mIndices[1];
				indexBuffer[fIndex * 3 + 2] = assimpMesh->mFaces[fIndex].mIndices[2];
			}
		});

		meshHandle.update();
	}
	return 0;
}

int Importer::importTexture(std::string filename, TextureManager::Handle& textureHandle, bool srgb) {
	spdlog::info("Importing texture '{}'...", filename);
	int width;
	int height;
	int channels;

	auto image = stbi_load(filename.c_str(), &width, &height, &channels, 4);
	if (image == nullptr) {
		spdlog::error("Failed to import '{}'", filename);
		return 1;
	}

	textureHandle.apply([image, width, height, srgb](auto& texture) {
		texture.size = vk::Extent3D(width, height, 1);

		texture.setPixelData(image, width * height * 4);

		if (srgb) {
			texture.format = vk::Format::eR8G8B8A8Srgb;
		} else {
			texture.format = vk::Format::eR8G8B8A8Unorm;
		}
		texture.usage		= vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
		texture.imageAspect = vk::ImageAspectFlagBits::eColor;

		texture.useMipMapping = true;
	});
	textureHandle.update();

	stbi_image_free(image);

	return 0;
}
} // namespace Engine