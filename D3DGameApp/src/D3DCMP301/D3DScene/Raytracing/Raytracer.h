#pragma once

#include "../PostProcess/PostProcessChain.h"


// Pretty-print a state object tree.
inline void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc)
{
	std::wstringstream wstr;
	wstr << L"\n";
	wstr << L"--------------------------------------------------------------------\n";
	wstr << L"| D3D12 State Object 0x" << static_cast<const void*>(desc) << L": ";
	if (desc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION) wstr << L"Collection\n";
	if (desc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) wstr << L"Raytracing Pipeline\n";

	auto ExportTree = [](UINT depth, UINT numExports, const D3D12_EXPORT_DESC* exports)
		{
			std::wostringstream woss;
			for (UINT i = 0; i < numExports; i++)
			{
				woss << L"|";
				if (depth > 0)
				{
					for (UINT j = 0; j < 2 * depth - 1; j++) woss << L" ";
				}
				woss << L" [" << i << L"]: ";
				if (exports[i].ExportToRename) woss << exports[i].ExportToRename << L" --> ";
				woss << exports[i].Name << L"\n";
			}
			return woss.str();
		};

	for (UINT i = 0; i < desc->NumSubobjects; i++)
	{
		wstr << L"| [" << i << L"]: ";
		switch (desc->pSubobjects[i].Type)
		{
		case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
			wstr << L"Global Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
			break;
		case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
			wstr << L"Local Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
			break;
		case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
			wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8) << *static_cast<const UINT*>(desc->pSubobjects[i].pDesc) << std::setw(0) << std::dec << L"\n";
			break;
		case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
		{
			wstr << L"DXIL Library 0x";
			auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << lib->DXILLibrary.pShaderBytecode << L", " << lib->DXILLibrary.BytecodeLength << L" bytes\n";
			wstr << ExportTree(1, lib->NumExports, lib->pExports);
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
		{
			wstr << L"Existing Library 0x";
			auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << collection->pExistingCollection << L"\n";
			wstr << ExportTree(1, collection->NumExports, collection->pExports);
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
		{
			wstr << L"Subobject to Exports Association (Subobject [";
			auto association = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
			UINT index = static_cast<UINT>(association->pSubobjectToAssociate - desc->pSubobjects);
			wstr << index << L"])\n";
			for (UINT j = 0; j < association->NumExports; j++)
			{
				wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
			}
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
		{
			wstr << L"DXIL Subobjects to Exports Association (";
			auto association = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
			wstr << association->SubobjectToAssociate << L")\n";
			for (UINT j = 0; j < association->NumExports; j++)
			{
				wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
			}
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG:
		{
			wstr << L"Raytracing Shader Config\n";
			auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(desc->pSubobjects[i].pDesc);
			wstr << L"|  [0]: Max Payload Size: " << config->MaxPayloadSizeInBytes << L" bytes\n";
			wstr << L"|  [1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes << L" bytes\n";
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG:
		{
			wstr << L"Raytracing Pipeline Config\n";
			auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(desc->pSubobjects[i].pDesc);
			wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth << L"\n";
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1:
		{
			wstr << L"Raytracing Pipeline Config\n";
			auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG1*>(desc->pSubobjects[i].pDesc);
			wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth << L"\n";
			wstr << L"|  [1]: Flags: " << config->Flags << L"\n";
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
		{
			wstr << L"Hit Group (";
			auto hitGroup = static_cast<const D3D12_HIT_GROUP_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]") << L")\n";
			wstr << L"|  [0]: Any Hit Import: " << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport : L"[none]") << L"\n";
			wstr << L"|  [1]: Closest Hit Import: " << (hitGroup->ClosestHitShaderImport ? hitGroup->ClosestHitShaderImport : L"[none]") << L"\n";
			wstr << L"|  [2]: Intersection Import: " << (hitGroup->IntersectionShaderImport ? hitGroup->IntersectionShaderImport : L"[none]") << L"\n";
			break;
		}
		}
		wstr << L"|--------------------------------------------------------------------\n";
	}
	wstr << L"\n";

	SKTBD_LOG_INFO("Raytracer", L"{}" , wstr.str().c_str())

	//OutputDebugStringW(wstr.str().c_str());
}

struct DXRTracing
{
	struct Description
	{
		using HitGroup = std::tuple<D3D12_HIT_GROUP_TYPE, const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*>;

		enum class HG : uint8_t
		{
			TYPE = 0,
			NAME = 1,
			AnyHit = 2,
			ClosestHit = 3,
			InterSection = 4
		};

		template<HG T>
		auto& Get(HitGroup& hitgroup) { return std::get<static_cast<size_t>(T)>(hitgroup); }

		void SetLibrary(const wchar_t* lib)
		{
			Library = lib;
		}

		void AddRaygen(const wchar_t* raygen)
		{
			Raygen.push_back(raygen);
		}

		void AddHitGroup(D3D12_HIT_GROUP_TYPE type, const wchar_t* name, const wchar_t* anyhit, const wchar_t* closesthit, const wchar_t* intersection = nullptr)
		{
			HitGroups.emplace_back(type, name, anyhit,closesthit, intersection);
		}

		void AddMiss(const wchar_t* miss)
		{
			Miss.push_back(std::move(miss));
		}

		void AddCallableShader(const wchar_t* callable)
		{
			CallableShaders.push_back(callable);
		}

		void SetConfigSetConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize, uint32_t maxRecursionDepth, uint32_t maxCallableShaderRecursionDepth = 1u, D3D12_RAYTRACING_PIPELINE_FLAGS flags = D3D12_RAYTRACING_PIPELINE_FLAG_NONE)
		{
			//set config
			MaxPayloadSize = maxPayloadSize;
			MaxAttributeSize = maxAttributeSize;
			MaxTraceRecursionDepth = maxRecursionDepth;
			MaxCallableShaderRecursionDepth = maxCallableShaderRecursionDepth;
			Flags = flags;
		}

		const wchar_t* Library;
		std::vector<const wchar_t*>  Raygen;
		std::vector<HitGroup>	     HitGroups;
		std::vector<const wchar_t*>  Miss;
		std::vector<const wchar_t*>  CallableShaders;

		uint32_t MaxPayloadSize;
		uint32_t MaxAttributeSize;
		uint32_t MaxTraceRecursionDepth;
		uint32_t MaxCallableShaderRecursionDepth;
		D3D12_RAYTRACING_PIPELINE_FLAGS Flags;
	};

	static std::vector<D3D12_GPU_VIRTUAL_ADDRESS> BuildBottomLevelAccelerationStructures(const std::vector<Skateboard::Mesh*>& meshes, ID3D12GraphicsCommandList10* list)
	{
		auto device = Skateboard::D3D::gD3DContext->GetDevice();

		if (m_BottomAccelerationStructureBuffer.Get())
		{
			Skateboard::D3D::gD3DContext->DeferredRelease(m_BottomAccelerationStructureBuffer.Get().Detach());

			Skateboard::D3D::gD3DContext->DeferredRelease(std::get<Resource>(m_PrimitiveData.Get()).Detach());
			Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().DeferredFree(std::get<D3D::SHADER_VIEW_HANDLE>(m_PrimitiveData.Get()));
		}

		uint32_t TotalSize = 0;
		uint32_t TotalScratchSize = 0;

		std::vector<std::tuple<std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>, D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO>> geometryDescriptions(meshes.size());
		std::vector<PrimitiveStridesAndOffsets> Primitives;


		for(auto i = 0; auto& m : meshes)
		{
			auto& [geometries,input,info] = geometryDescriptions[i];

			geometries.resize(m->GetPrimitiveCount());

			for(auto j =0; auto& p : *m)
			{
				D3D12_RAYTRACING_GEOMETRY_DESC Geometry{};
				Geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
				Geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

				Geometry.Triangles.IndexCount = p.IndexBuffer.m_IndexCount;
				Geometry.Triangles.IndexBuffer = static_cast<Skateboard::D3D::D3DBuffer*>(p.IndexBuffer.m_ParentResource.get())->GetResource()->GetGPUVirtualAddress() + p.IndexBuffer.m_Offset;

				uint index_size = 0;

				switch(p.IndexBuffer.m_Format)
				{
				case bit8:	Geometry.Triangles.IndexFormat = DXGI_FORMAT_R8_UINT;  index_size = 1; break;
				case bit16: Geometry.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT; index_size = 2; break;
				case bit32: Geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT; index_size = 4; break;
				}

				//this assumes slot 0 of the buffer layout contains the vertex position as the first element.
				Geometry.Triangles.VertexBuffer = { static_cast<Skateboard::D3D::D3DBuffer*>(p.VertexBuffers[0].m_ParentResource.get())->GetResourceGPUAddress() + p.VertexBuffers[0].m_Offset, p.VertexBuffers[0].m_VertexStride };
				Geometry.Triangles.VertexCount = p.VertexBuffers[0].m_VertexCount;
				Geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

				Geometry.Triangles.Transform3x4 = 0; //we are not using a per geometry transform

				PrimitiveStridesAndOffsets Primitive{ p.IndexBuffer.m_Offset,index_size, p.IndexBuffer.m_IndexCount, p.VertexBuffers[0].m_Offset, p.VertexBuffers[0].m_VertexStride, p.VertexBuffers[0].m_VertexCount};
				Primitives.push_back(Primitive);

				geometries[j] = Geometry;
				j++;
			}

			input.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			input.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
			input.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			input.NumDescs = uint32_t(geometries.size());
			input.pGeometryDescs = geometries.data();

			//get the prebuild info
			device->GetRaytracingAccelerationStructurePrebuildInfo(&input, &info);

			info.ResultDataMaxSizeInBytes = ROUND_UP(info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
			info.ScratchDataSizeInBytes = ROUND_UP(info.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

			//
			TotalSize += info.ResultDataMaxSizeInBytes;
			TotalScratchSize += info.ScratchDataSizeInBytes;

			i++;
		}

		auto allocator = Skateboard::D3D::gD3DContext->GetMemoryAllocator();

		auto resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer(Primitives.size()* sizeof(PrimitiveStridesAndOffsets));
		auto allocDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_UPLOAD };

		//create the resource
		allocator->CreateResource3(
			&allocDesc,
			&resourceDesc,
			D3D12_BARRIER_LAYOUT_UNDEFINED,
			0,
			0,
			NULL,
			&std::get<Resource>(m_PrimitiveData.Get()),
			IID_NULL,
			nullptr);

		std::get<Resource>(m_PrimitiveData.Get())->GetResource()->SetName(L"PrimitiveDataOffsets");

		//map
		std::get<Resource>(m_PrimitiveData.Get())->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&std::get<PrimitiveStridesAndOffsets*>(m_PrimitiveData.Get())));

		//copy primtive data over
		memcpy(std::get<PrimitiveStridesAndOffsets*>(m_PrimitiveData.Get()), Primitives.data(), Primitives.size() * sizeof(PrimitiveStridesAndOffsets));

		//create structured buffer view;
		std::get<D3D::SHADER_VIEW_HANDLE>(m_PrimitiveData.Get()) = Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().Allocate(1);
		auto PrimViewDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer(Primitives.size(), sizeof(PrimitiveStridesAndOffsets));
		device->CreateShaderResourceView(std::get<Resource>(m_PrimitiveData.Get())->GetResource(), &PrimViewDesc, std::get<D3D::SHADER_VIEW_HANDLE>(m_PrimitiveData.Get()).GetCPUHandle());

		//Blas storage
		resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer( TotalSize , D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		allocDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_DEFAULT };

		//create the resource
		allocator->CreateResource3(
				&allocDesc,
				&resourceDesc,
				D3D12_BARRIER_LAYOUT_UNDEFINED,
				0,
				0,
				NULL,
				&m_BottomAccelerationStructureBuffer.Get(),
				IID_NULL,
				nullptr);

		m_BottomAccelerationStructureBuffer.Get()->GetResource()->SetName(L"BLAS");

		//Scratch
		D3D12MA::Allocation* ScratchBuffer;
		resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer( TotalScratchSize , D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS );

		allocator->CreateResource3(
			&allocDesc,
			&resourceDesc,
			D3D12_BARRIER_LAYOUT_UNDEFINED,
			0,
			0,
			NULL,
			&ScratchBuffer,
			IID_NULL,
			nullptr);

		std::vector<D3D12_GPU_VIRTUAL_ADDRESS> Addresses(geometryDescriptions.size());

		uint64_t Address = m_BottomAccelerationStructureBuffer.Get()->GetResource()->GetGPUVirtualAddress();
		uint64_t ScratchAddress = ScratchBuffer->GetResource()->GetGPUVirtualAddress();

		for (auto idx = 0; auto& [Geometries, inputs, info] : geometryDescriptions)
		{
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
			asDesc.Inputs = inputs;

			asDesc.DestAccelerationStructureData = Address;
			asDesc.ScratchAccelerationStructureData = ScratchAddress;

			Addresses[idx++] = asDesc.DestAccelerationStructureData;

			Address += info.ResultDataMaxSizeInBytes;
			ScratchAddress += info.ScratchDataSizeInBytes;

			list->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);
		}

		//clean up
		Skateboard::D3D::gD3DContext->DeferredRelease(ScratchBuffer);

		return Addresses;
	}

	static void BuildTopLevelAccelerationStructures(const std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instances, ID3D12GraphicsCommandList10* list)
	{
		auto device = Skateboard::D3D::gD3DContext->GetDevice();
		auto allocator = Skateboard::D3D::gD3DContext->GetMemoryAllocator();

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS input{};

		input.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		input.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		input.NumDescs = uint32_t(instances.size());

		//get the prebuild info
		device->GetRaytracingAccelerationStructurePrebuildInfo(&input, &info);

		auto resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto allocDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_DEFAULT };

		auto& [resource, handle] = m_TopAccelerationStructureBuffer.Get();

		if (resource)
		{
			Skateboard::D3D::gD3DContext->DeferredRelease(resource.Detach());
			Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().DeferredFree(handle);
		}

		//create the resource
		allocator->CreateResource3(
			&allocDesc,
			&resourceDesc,
			D3D12_BARRIER_LAYOUT_UNDEFINED,
			0,
			0,
			NULL,
			&resource,
			IID_NULL,
			nullptr);

		resource->GetResource()->SetName(L"TLAS");

		handle = Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().Allocate();

		auto srvdesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::RaytracingAccelStruct(resource->GetResource()->GetGPUVirtualAddress());

		device->CreateShaderResourceView(nullptr, &srvdesc, handle.GetCPUHandle());

		D3D12MA::Allocation* ScratchBuffer;

		resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		allocator->CreateResource3(
			&allocDesc,
			&resourceDesc,
			D3D12_BARRIER_LAYOUT_UNDEFINED,
			0,
			0,
			NULL,
			&ScratchBuffer,
			IID_NULL,
			nullptr);

		D3D12MA::Allocation* UploadBuffer;

		auto upload_size = instances.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

		resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer(upload_size);

		allocDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_UPLOAD };

		allocator->CreateResource3(
			&allocDesc,
			&resourceDesc,
			D3D12_BARRIER_LAYOUT_UNDEFINED,
			0,
			0,
			NULL,
			&UploadBuffer,
			IID_NULL,
			nullptr);

		D3D12_RAYTRACING_INSTANCE_DESC* instanceData;
		UploadBuffer->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&instanceData));

		memcpy(instanceData, instances.data(), upload_size);

		input.InstanceDescs = UploadBuffer->GetResource()->GetGPUVirtualAddress();
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
		asDesc.Inputs = input;
		asDesc.DestAccelerationStructureData =	resource->GetResource()->GetGPUVirtualAddress();
		asDesc.ScratchAccelerationStructureData = ScratchBuffer->GetResource()->GetGPUVirtualAddress();

		list->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

		//clean up
		Skateboard::D3D::gD3DContext->DeferredRelease(ScratchBuffer);
		Skateboard::D3D::gD3DContext->DeferredRelease(UploadBuffer);
	}

	static void Prepare(const Description& desc,  ID3D12RootSignature* Root)
	{
		CD3DX12_STATE_OBJECT_DESC SO{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };
		ComPtr<IDxcBlobEncoding> Blob;

		//global root signature
		{
			SO.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>()->SetRootSignature(Root);
		}

		auto libSU = SO.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

		//DXIL library
		{
			Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(desc.Library, Blob.ReleaseAndGetAddressOf());
			D3D12_SHADER_BYTECODE lib{ Blob->GetBufferPointer(), Blob->GetBufferSize() };
			libSU->SetDXILLibrary(&lib);
			//exports for miss and raygen
			{
				//raygens
				libSU->DefineExports(desc.Raygen.data(), desc.Raygen.size());
				libSU->DefineExports(desc.Miss.data(), desc.Miss.size());
				libSU->DefineExports(desc.CallableShaders.data(), desc.CallableShaders.size());
			}
		}

		//hit groups are special 
		for (const auto& [type, name, any, closest, intersection] : desc.HitGroups)
		{
			auto hitGroup = SO.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
			hitGroup->SetHitGroupType(type);

			if (any)
			{
				hitGroup->SetAnyHitShaderImport(any);
				libSU->DefineExport(any);
			}

			if(closest)
			{
				hitGroup->SetClosestHitShaderImport(closest);
				libSU->DefineExport(closest);
			}

			if(intersection)
			{
				hitGroup->SetIntersectionShaderImport(intersection);
				libSU->DefineExport(intersection);
			}

			hitGroup->SetHitGroupExport(name);
		}

		//shader config
		{
			SO.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>()->Config(desc.MaxPayloadSize, desc.MaxAttributeSize);
		}

		//pipeline config
		{
			SO.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG1_SUBOBJECT>()->Config(desc.MaxTraceRecursionDepth, desc.Flags);
		}

		auto device = Skateboard::D3D::gD3DContext->GetDevice();

		D3D_CHECK_FAILURE(device->CreateStateObject(SO, IID_PPV_ARGS(&m_DXRState)), L"Couldn't create DirectX Raytracing state object.\n");

		PrintStateObjectDesc(SO);
	}

	static void BuildShaderTable(const Description& desc, uint raygen_shader_to_use = 0)
	{
		if (m_ShaderTables.Get())
		{
			Skateboard::D3D::gD3DContext->DeferredRelease(m_ShaderTables.Detach());
		}

		//one raygenshader
		
		uint raygentablesize =	D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;																							//Table for Raygen Record (single)
		uint misstablesize = desc.Miss.empty() ? 0 : ROUND_UP(desc.Miss.size() * D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);					//Table for Miss
		uint hittablesize = desc.HitGroups.empty() ? 0 : ROUND_UP(desc.HitGroups.size() * D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);				//Table for Hit
		uint callabletablesize = desc.CallableShaders.empty() ? 0 : ROUND_UP(desc.CallableShaders.size() * D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);	//Table for Callables

		uint32_t ShaderTableSizeTotal = raygentablesize + misstablesize + hittablesize + callabletablesize;

		auto allocator = Skateboard::D3D::gD3DContext->GetMemoryAllocator();

		//ShaderTable storage
		auto resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer(64*1024);

		D3D12_RESOURCE_DESC1 buffdesc = {};
		buffdesc.Alignment = 0;//(desc.Alignment + SKTBD_DESKTOP_PLATFORM_MIN_BUFFER_ALIGNMENT) & ~SKTBD_DESKTOP_PLATFORM_MIN_BUFFER_ALIGNMENT;
		buffdesc.DepthOrArraySize = 1;
		buffdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		buffdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		buffdesc.Format = DXGI_FORMAT_UNKNOWN;
		buffdesc.Height = 1;
		buffdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		buffdesc.MipLevels = 1;
		buffdesc.SampleDesc.Count = 1;
		buffdesc.SampleDesc.Quality = 0;
		buffdesc.Width = ShaderTableSizeTotal;

		auto allocDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_UPLOAD };

		//create the resource
		allocator->CreateResource3(
			&allocDesc,
			&buffdesc,
			D3D12_BARRIER_LAYOUT_UNDEFINED,
			0,
			0,
			NULL,
			m_ShaderTables.GetAddressOf(),
			IID_NULL,
			nullptr);

		uint8_t* Dest{};
		D3D_CHECK_FAILURE(m_ShaderTables->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&Dest)));

		m_ShaderTables->GetResource()->SetName(L"ShaderTables");

		//write records

		ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		D3D_CHECK_FAILURE(m_DXRState.As(&stateObjectProperties));

		m_DispatchInfo = {};

		auto StartAddr = m_ShaderTables->GetResource()->GetGPUVirtualAddress();

		if (raygentablesize)
		{
			//raygen
			m_DispatchInfo.RayGenerationShaderRecord.StartAddress = StartAddr;
			m_DispatchInfo.RayGenerationShaderRecord.SizeInBytes = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;

			{
				auto identifier = stateObjectProperties->GetShaderIdentifier(desc.Raygen[raygen_shader_to_use]);
				memcpy(Dest, identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			}

			Dest += raygentablesize;
		}

		if (misstablesize)
		{
			//miss
			m_DispatchInfo.MissShaderTable = { .StartAddress = StartAddr + raygentablesize, .SizeInBytes = misstablesize, .StrideInBytes = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, };
			auto tablestart = Dest;
			for (auto& s : desc.Miss) {
				auto identifier = stateObjectProperties->GetShaderIdentifier(s);
				memcpy(tablestart, identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				tablestart += D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
			}

			Dest += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
		}

		if (hittablesize)
		{
			//hit
			m_DispatchInfo.HitGroupTable = { .StartAddress = StartAddr + raygentablesize + misstablesize, .SizeInBytes = hittablesize, .StrideInBytes = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT };
			auto tablestart = Dest;
			for (auto& [type, name, any, closest, intersection] : desc.HitGroups) {
				auto identifier = stateObjectProperties->GetShaderIdentifier(name);
				memcpy(tablestart, identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				tablestart += D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
			}

			Dest += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
		}

		if (callabletablesize)
		{
			//callable
			m_DispatchInfo.CallableShaderTable = { .StartAddress = StartAddr + raygentablesize + misstablesize + hittablesize, .SizeInBytes = callabletablesize,.StrideInBytes = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT };
			auto tablestart = Dest;
			for (auto& s : desc.CallableShaders) {
				auto identifier = stateObjectProperties->GetShaderIdentifier(s);
				memcpy(tablestart, identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				tablestart += D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
			}
		}
	}

	static void SetInOutAndDispatch(ID3D12GraphicsCommandList10* list, const D3D::SHADER_VIEW_HANDLE Input, const D3D::SHADER_VIEW_HANDLE& outUAV, uint3 DispatchSize)
	{
		list->SetPipelineState1(m_DXRState.Get());

		uint2 RootConst{Input.GetIndex(), outUAV.GetIndex() };
		list->SetComputeRoot32BitConstants(1, 2, &RootConst, 0);

		m_DispatchInfo.Width = DispatchSize.x;
		m_DispatchInfo.Height = DispatchSize.y;
		m_DispatchInfo.Depth = DispatchSize.z;

		list->DispatchRays(&m_DispatchInfo);
	}

	static auto GetBLAS() { return m_BottomAccelerationStructureBuffer.Get(); }
	static auto GetTLAS() { return m_TopAccelerationStructureBuffer.Get(); }

	//Acceleration structure buffer paired with its shader view handle
	inline static Skateboard::RingArray<Resource,1> m_BottomAccelerationStructureBuffer;
	inline static Skateboard::RingArray<std::pair<Resource, Skateboard::D3D::SHADER_VIEW_HANDLE>,1> m_TopAccelerationStructureBuffer;

	//primitive info
	inline static Skateboard::RingArray<std::tuple<Resource, D3D::SHADER_VIEW_HANDLE, PrimitiveStridesAndOffsets*>,1> m_PrimitiveData;

	//shaderTables 
	inline static Resource m_ShaderTables;
	inline static D3D12_DISPATCH_RAYS_DESC   m_DispatchInfo;

	//state
	inline static ComPtr<ID3D12StateObject> m_DXRState;

};


