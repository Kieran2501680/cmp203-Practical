#include <sktbdpch.h>
#include "AssetManager.h"
//#include "Skateboard/Renderer/Renderer.h"

#include "Skateboard/Graphics/RHI/ResourceFactory.h"

#define SKTBD_LOG_COMPONENT "ASSET_MANAGER"
#include "Skateboard/Log.h"
#include "Skateboard/Platform.h"

namespace Skateboard
{
	AssetManager& AssetManager::Singleton()
	{
		static AssetManager* singleton = Platform::GetAssetManager();
		return *singleton;
	}

	AssetManager::~AssetManager()
	{
		//Meshes store the sub allocations
		m_Meshes.clear();
		m_VertexBufferSuballocator->Clear();
		m_VertexBufferSuballocator->Release();
	}

	Mesh* AssetManager::CreateModelFromBuffersImpl(const std::string& modelTag, const std::vector<PrimitiveData>& Primitives)
	{
		if (m_Meshes.contains(modelTag)) return m_Meshes[modelTag].get();

		SKTBD_LOG_ASSERT(!Primitives.empty(), "AssetManager", "provided Empty Primitive collection")

		std::vector<Primitive> primitives(Primitives.size());
		//std::vector<Texture*> meshTextures(ass.textures.size());
		uint32_t index = 0;

		for (auto& vb : Primitives) {

			auto& prim = primitives[index]; 

			//Check Vertex and index sizes

			if( vb.IndexCount == 0 || vb.VertexCount == 0)
			{
				SKTBD_MSG_INFO("LoadModel::{3} Loading mesh {0} within model failed, index or vertex buffer contain no vertices, indices {1}, vertices {2}", index, vb.IndexCount, vb.VertexCount, modelTag)
					continue;
			}
			else
			{
				SKTBD_MSG_WARN("LoadModel::{0} Loading mesh {1}, indices {2}, vertices {3}", modelTag, index, vb.IndexCount, vb.VertexCount)
			}

			//IndexSize
			size_t indexSize = vb.IndexLayout;
			prim.IndexBuffer.m_Format = vb.IndexLayout;

			size_t IBSize = ROUND_UP(vb.IndexCount * indexSize, GraphicsConstants::BUFFER_ALIGNMENT);
			size_t VBsize = ROUND_UP(vb.VertexCount * vb.VertexLayout.GetStride(), GraphicsConstants::BUFFER_ALIGNMENT);

			size_t PrimitiveSize = IBSize + VBsize;

			MemoryUtils::VIRTUAL_ALLOCATION_DESC desc{};
			desc.Size = PrimitiveSize;
			desc.Alignment = GraphicsConstants::BUFFER_ALIGNMENT;

			prim.Allocations = std::make_shared<PrimitiveSuballocationSingleBuffer>();
			prim.Allocations->ParentAllocator = m_VertexBufferSuballocator;

			uint64_t Offset;
			
			m_VertexBufferSuballocator->Allocate(&desc, &prim.Allocations->PrimitiveData, &Offset);
			SKTBD_LOG_INFO("AssetManager", "Allocated {} bytes for primitive {} at offset {}", PrimitiveSize, index, Offset)


			prim.IndexBuffer.m_IndexCount = vb.IndexCount;
			prim.IndexBuffer.m_Offset = Offset;
			prim.IndexBuffer.m_ParentResource = m_StaticMeshVertexBuffer;

			for (auto [slot, Stride] : vb.VertexLayout.GetSlotsAndStrides())
			{
				prim.VertexBuffers[slot].m_Offset = Offset + IBSize;
				prim.VertexBuffers[slot].m_ParentResource = m_StaticMeshVertexBuffer;
				prim.VertexBuffers[slot].m_VertexCount = vb.VertexCount;
				prim.VertexBuffers[slot].m_VertexStride = Stride;
			}

			prim.Layout = vb.VertexLayout;
			prim.BoundBox = vb.BoundingBox;

			auto res = GraphicsContext::CopyDataToBuffer(m_StaticMeshVertexBuffer.get(), Offset, PrimitiveSize, [&](void* dest)
				{
					//copy indices
					memcpy(dest, vb.IndexData.data(), IBSize);

					//copy vertices
					memcpy((uint8_t*)dest + IBSize, vb.VertexData.data(), VBsize);
				}
			);

			res.m_CompletionFuture.wait();
			++index;
		}

		m_Meshes[modelTag].reset();
		m_Meshes[modelTag] = std::make_unique<Mesh>(primitives);

		return m_Meshes[modelTag].get();
	}

	bool AssetManager::ReleaseTexture(const std::string& name)
	{
		if (!Singleton().m_Textures.contains(name))
		{
			SKTBD_MSG_WARN("Asset manager doesnt contain a texture with this name")
			return false;
		}

		// Do not attempt to delete the texture from memory by force!
		// Let the internal API clean up resources.
		//Singleton().m_Textures[name]->Release();
		Singleton().m_Textures.erase(name);


		return true;
	}

	BufferSRVRef AssetManager::GetBackingByteAddressBufferSRV()
	{
		if (Singleton().m_ByteAddressViewOfMeshData) return Singleton().m_ByteAddressViewOfMeshData;
		else
		{
			BufferViewDesc desc; desc.InitAsByteBuffer(0, ASSET_MANAGER_VERTEX_BUFFER_SIZE);
			Singleton().m_ByteAddressViewOfMeshData = ResourceFactory::CreateBufferShaderResourceView(desc, Singleton().m_StaticMeshVertexBuffer, L"AssetManagerBufferSRV");
			return Singleton().m_ByteAddressViewOfMeshData;
		}
	}

	void AssetManager::Init()
	{
		BufferDesc desc;
		desc.Init(ASSET_MANAGER_VERTEX_BUFFER_SIZE, ResourceAccessFlag_GpuRead);

		m_StaticMeshVertexBuffer = ResourceFactory::CreateBuffer(desc);

		MemoryUtils::VIRTUAL_BLOCK_DESC aldesc{};
		aldesc.Size = ASSET_MANAGER_VERTEX_BUFFER_SIZE;

		MemoryUtils::CreateBlock(&aldesc, &m_VertexBufferSuballocator);
	}
}
