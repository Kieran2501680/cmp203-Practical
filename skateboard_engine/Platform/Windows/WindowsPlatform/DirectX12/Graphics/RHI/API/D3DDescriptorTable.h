#pragma once

//#include "Graphics/D3D.h"
#include "D3DDescriptorHeap.h"
#include "Skateboard/Memory/DescriptorTable.h"

namespace Skateboard::D3D
{
	class D3DDescriptorTable final : public DescriptorTable
	{
	public:
		DISABLE_C_M_D(D3DDescriptorTable)
		D3DDescriptorTable(const DescriptorTableDesc& desc);

		virtual ~D3DDescriptorTable() final override;

		virtual void GenerateTable() final override;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle();

	private:
		virtual void UpdateTableEntry(uint64_t hash) final override;
		__forceinline void InitTable(std::array<SHADER_VIEW_HANDLE, GRAPHICS_SETTINGS_NUMFRAMERESOURCES>& table);
		__forceinline void CheckFrameIndex();

	private:
		std::vector<std::array<SHADER_VIEW_HANDLE, GRAPHICS_SETTINGS_NUMFRAMERESOURCES>> va_DescriptorTablesStart;
		std::array<SHADER_VIEW_HANDLE, GRAPHICS_SETTINGS_NUMFRAMERESOURCES>* p_CurrentTable;
		uint32_t m_CurrentTableIndex;
		uint32_t m_DescriptorSize;
		bool m_TableDirty;
	};
}