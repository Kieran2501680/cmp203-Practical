#pragma once
#include "Windows/WindowsPlatform/DirectX12/Graphics/RHI/API/D3DDescriptorHeap.h"
#include "Skateboard/Graphics/Resources/View.h"

namespace Skateboard::D3D
{
	class D3DConstantBufferView final : public ConstantBufferView
	{
	public:
		D3DConstantBufferView(const BufferViewDesc& viewDesc, BufferRef Parent = BufferRef());
		uint32_t GetViewIndex() override { return m_Descriptor.GetIndex(); }

		~D3DConstantBufferView() override;
	
		SHADER_VIEW_HANDLE m_Descriptor;
	};

	class D3DShaderResourceBufferView final : public ShaderResourceBufferView
	{
	public:
		D3DShaderResourceBufferView(const BufferViewDesc& viewDesc, BufferRef Parent = BufferRef());
		uint32_t GetViewIndex() override { return m_Descriptor.GetIndex(); }

		~D3DShaderResourceBufferView() override;
	
		SHADER_VIEW_HANDLE m_Descriptor;
	};

	class D3DUnorderedAccessBufferView final : public UnorderedAccessBufferView
	{
	public:
		D3DUnorderedAccessBufferView(const BufferViewDesc& viewDesc, BufferRef parent = BufferRef(), BufferRef counterResource = BufferRef());
		uint32_t GetViewIndex() override { return m_Descriptor.GetIndex(); }

		~D3DUnorderedAccessBufferView() override;
	
		SHADER_VIEW_HANDLE m_Descriptor;
	};

	class D3DShaderResourceTextureView final : public ShaderResourceTextureView
	{
	public:
		D3DShaderResourceTextureView(const TextureViewDesc& viewDesc, TextureBufferRef parent = TextureBufferRef());
		uint32_t GetViewIndex() override { return m_Descriptor.GetIndex(); }
		ImTextureID GetImTextureID() override { return m_Descriptor.GetGPUPointer(); }

		~D3DShaderResourceTextureView() override;
		
		SHADER_VIEW_HANDLE m_Descriptor;
	};

	class D3DUnorderedAccessTextureView final : public UnorderedAccessTextureView
	{
	public:
		D3DUnorderedAccessTextureView(const TextureViewDesc& viewDesc, TextureBufferRef parent = TextureBufferRef());
		uint32_t GetViewIndex() override { return m_Descriptor.GetIndex(); }
		ImTextureID GetImTextureID() override { return m_Descriptor.GetGPUPointer(); }

		~D3DUnorderedAccessTextureView() override;
		
		SHADER_VIEW_HANDLE m_Descriptor;
	};

	class D3DRenderTargetView final : public RenderTargetView
	{
	public:
		D3DRenderTargetView(const RenderTargetDesc* viewDesc, TextureBufferRef parent = TextureBufferRef());
		D3DRenderTargetView(const RTV_HANDLE& Descriptor) : RenderTargetView(nullptr) { m_Descriptor = Descriptor; }
		uint32_t GetViewIndex() override { return m_Descriptor.GetIndex(); }

		~D3DRenderTargetView() override;

	
		RTV_HANDLE m_Descriptor;
	};

	class D3DDepthStencilView final : public DepthStencilView
	{
	public:
		D3DDepthStencilView(const DepthStencilDesc* viewDesc, TextureBufferRef parent = TextureBufferRef());
		D3DDepthStencilView(const DSV_HANDLE& Descriptor) : DepthStencilView(nullptr) { m_Descriptor = Descriptor; }
		uint32_t GetViewIndex() override { return m_Descriptor.GetIndex(); }

		~D3DDepthStencilView() override;
	
		DSV_HANDLE m_Descriptor;
	};

	

	class D3DAccelerationStructView final : public AccelerationStructureView
	{
	public:
		D3DAccelerationStructView(AccelerationStructureData AS_handle);

		uint32_t GetViewIndex() override { return m_Descriptor.GetIndex(); }

		~D3DAccelerationStructView() override;

		SHADER_VIEW_HANDLE m_Descriptor;
	};

}