#include "Importer.hpp"

#include <spdlog/spdlog.h>

#include <assimp/postprocess.h>
#include <assimp/scene.h>


namespace Engine::Utils {
Assimp::Importer Importer::assimpImporter {};


int Importer::importMesh(std::string filename, std::vector<Engine::Managers::MeshManager::Handle>& meshHandles) {
	spdlog::info("Importing '{}'", filename);

	const aiScene* scene =
		assimpImporter.ReadFile(filename,
								aiProcess_CalcTangentSpace | aiProcess_GenUVCoords | aiProcess_GenBoundingBoxes |
									aiProcess_PreTransformVertices | aiProcess_MakeLeftHanded);

	if (!scene) {
		spdlog::error("Failed to import '{}'", filename);
		return 1;
	}

	meshHandles.resize(scene->mNumMeshes);

	for (uint i = 0; i < meshHandles.size(); i++) {
		auto& meshHandle = meshHandles[i];
		meshHandle = Engine::Managers::MeshManager::createObject(0);

		auto assimpMesh = scene->mMeshes[i];

		meshHandle.apply([&assimpMesh](auto& mesh) {
			auto& vertexBuffer = mesh.getVertexBuffer();
			vertexBuffer.resize(assimpMesh->mNumVertices);

			for (int vIndex = 0; vIndex < assimpMesh->mNumVertices; vIndex++) {
				std::get<0>(vertexBuffer[vIndex]).x = assimpMesh->mVertices[vIndex].x;
				std::get<0>(vertexBuffer[vIndex]).y = assimpMesh->mVertices[vIndex].y;
				std::get<0>(vertexBuffer[vIndex]).z = assimpMesh->mVertices[vIndex].z;

				std::get<1>(vertexBuffer[vIndex]).x = 1.0f;
				std::get<1>(vertexBuffer[vIndex]).y = 1.0f;

				// std::get<1>(vertexBuffer[vIndex]).x = assimpMesh->mTextureCoords[0][vIndex].x;
				// std::get<1>(vertexBuffer[vIndex]).y = assimpMesh->mTextureCoords[0][vIndex].y;
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
}