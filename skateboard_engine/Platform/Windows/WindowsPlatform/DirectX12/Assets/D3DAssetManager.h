#pragma once
#include "Graphics/D3D.h"
#include "Skateboard/Assets/AssetManager.h"
#include "Graphics/RHI/API/D3DDescriptorTable.h"

namespace Skateboard::D3D
{
	class D3DAssetManager final : public AssetManager
	{
		virtual const Texture LoadTextureImpl(const wchar_t* filename, const std::string& textureTag, TextureDimension_ resType) override;
		virtual Texture GetDefaultTextureImpl(TextureDimension_ Dimension) override;
		virtual Mesh* LoadModelImpl(const wchar_t* filename, const std::string& modelTag, const BufferLayout& bufferLayout, bool loadIndexBuffer) override;

	protected:
		Animation* LoadAnimationImpl(const wchar_t* filename, const std::string& animationTag) override;
		//SkinnedMesh* LoadSkeletalMeshImpl(const wchar_t* filename, const std::string& meshTag,
		//	bool flipNormals) override;
		Skeleton* LoadSkeletonImpl(const wchar_t* filename, const std::string& skeletonTag) override;
		Texture CreateTextureFromDataImpl(const std::string& textureTag, const TextureDesc& desc, void* data) override;
		ImFont* LoadFontImpl(const wchar_t* filename, uint32_t sizeInPixels, const std::string& modelTag) override;
		std::vector<PrimitiveData> LoadModelCPUImpl(const wchar_t* filename, const BufferLayout& Layout, bool loadIndexBuffer) override;
	};
}