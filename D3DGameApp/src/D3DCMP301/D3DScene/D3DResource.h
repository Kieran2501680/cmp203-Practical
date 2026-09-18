#pragma once

#include "Windows/WindowsPlatform/DirectX12/Graphics/RHI/D3DGraphicsContext.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DBuffer.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DView.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DPipeline.h"

using Resource = ComPtr<D3D12MA::Allocation>;

struct TextureResourceState
{
    D3D12_BARRIER_SYNC Sync;
    D3D12_BARRIER_ACCESS Access;
    D3D12_BARRIER_LAYOUT Layout;
};

namespace Transition
{
    static void BarrierToSRV(ID3D12Resource** Resource, TextureResourceState& stateBefore, ID3D12GraphicsCommandList10* List, uint32_t ResourceCount = 1)
    {
		std::vector<CD3DX12_TEXTURE_BARRIER> barriers(ResourceCount);

        //we need to change the resource state of the render target to be able to read from it in the shader

        for(auto i = 0; auto& barrier : barriers)
        {
	        barrier = CD3DX12_TEXTURE_BARRIER(
                stateBefore.Sync,
                D3D12_BARRIER_SYNC_ALL_SHADING,
                stateBefore.Access,
                D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
                stateBefore.Layout,
                D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,
                Resource[i++],
                { 0xffffffff ,0 }
            );
        }

        stateBefore.Access = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
        stateBefore.Layout = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
        stateBefore.Sync = D3D12_BARRIER_SYNC_DRAW;

        CD3DX12_BARRIER_GROUP SRV = CD3DX12_BARRIER_GROUP(ResourceCount, barriers.data());

        List->Barrier(1, &SRV);
    }

    static void BarrierToRTV(ID3D12Resource** Resource, TextureResourceState& stateBefore, ID3D12GraphicsCommandList10* List, uint32_t ResourceCount = 1)
    {
        //we need to change the resource state of the render target to be able to read from it in the shader

        std::vector<CD3DX12_TEXTURE_BARRIER> barriers(ResourceCount);

        //we need to change the resource state of the render target to be able to read from it in the shader

        for (auto i = 0; auto& barrier : barriers)
        {
            barrier = CD3DX12_TEXTURE_BARRIER(
                stateBefore.Sync,
                D3D12_BARRIER_SYNC_RENDER_TARGET,
                stateBefore.Access,
                D3D12_BARRIER_ACCESS_RENDER_TARGET,
                stateBefore.Layout,
                D3D12_BARRIER_LAYOUT_RENDER_TARGET,
                Resource[i++],
                { 0xffffffff,0 }
            );
        }

        stateBefore.Access = D3D12_BARRIER_ACCESS_RENDER_TARGET;
        stateBefore.Layout = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
        stateBefore.Sync = D3D12_BARRIER_SYNC_RENDER_TARGET;

        CD3DX12_BARRIER_GROUP RTV = CD3DX12_BARRIER_GROUP( ResourceCount, barriers.data());

        List->Barrier(1, &RTV);
    }

    static void BarrierToUAV(ID3D12Resource** Resource, TextureResourceState& stateBefore, ID3D12GraphicsCommandList10* List, uint32_t ResourceCount = 1)
    {
        //we need to change the resource state of the render target to be able to read from it in the shader

        std::vector<CD3DX12_TEXTURE_BARRIER> barriers(ResourceCount);

        //we need to change the resource state of the render target to be able to read from it in the shader

        for (auto i = 0; auto& barrier : barriers)
        {
            barrier = CD3DX12_TEXTURE_BARRIER(
                stateBefore.Sync,
                D3D12_BARRIER_SYNC_ALL_SHADING,
                stateBefore.Access,
                D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
                stateBefore.Layout,
                D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
                Resource[i++],
                { 0xffffffff, 0 }
            );
        }

        stateBefore.Access = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
        stateBefore.Layout = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
        stateBefore.Sync = D3D12_BARRIER_SYNC_ALL_SHADING;

        CD3DX12_BARRIER_GROUP UAV = CD3DX12_BARRIER_GROUP(ResourceCount, barriers.data());

        List->Barrier(1, &UAV);
    }

    static void BarrierToDSV_WRITE(ID3D12Resource** Resource, TextureResourceState& stateBefore, ID3D12GraphicsCommandList10* List, uint32_t ResourceCount = 1)
    {
        //we need to change the resource state of the render target to be able to read from it in the shader

        std::vector<CD3DX12_TEXTURE_BARRIER> barriers(ResourceCount);

        //we need to change the resource state of the render target to be able to read from it in the shader

        for (auto i = 0; auto& barrier : barriers)
        {
            barrier = CD3DX12_TEXTURE_BARRIER(
                stateBefore.Sync,
                D3D12_BARRIER_SYNC_DEPTH_STENCIL,
                stateBefore.Access,
                D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
                stateBefore.Layout,
                D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE,
                Resource[i++],
                { 0xffffffff, 0 }
            );
        }

        stateBefore.Access = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
        stateBefore.Layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
        stateBefore.Sync = D3D12_BARRIER_SYNC_DEPTH_STENCIL;

        CD3DX12_BARRIER_GROUP UAV = CD3DX12_BARRIER_GROUP(ResourceCount, barriers.data());

        List->Barrier(1, &UAV);
    }

    static void BarrierToDSV_READ(ID3D12Resource** Resource, TextureResourceState& stateBefore, ID3D12GraphicsCommandList10* List, uint32_t ResourceCount = 1)
    {
        //we need to change the resource state of the render target to be able to read from it in the shader

        std::vector<CD3DX12_TEXTURE_BARRIER> barriers(ResourceCount);

        //we need to change the resource state of the render target to be able to read from it in the shader

        for (auto i = 0; auto& barrier : barriers)
        {
            barrier = CD3DX12_TEXTURE_BARRIER(
                stateBefore.Sync,
                D3D12_BARRIER_SYNC_DEPTH_STENCIL,
                stateBefore.Access,
                D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ,
                stateBefore.Layout,
                D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,
                Resource[i++],
                { 0xffffffff, 0 }
            );
        }

        stateBefore.Access = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
        stateBefore.Layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
        stateBefore.Sync = D3D12_BARRIER_SYNC_DEPTH_STENCIL;

        CD3DX12_BARRIER_GROUP UAV = CD3DX12_BARRIER_GROUP(ResourceCount, barriers.data());

        List->Barrier(1, &UAV);
    }

}
