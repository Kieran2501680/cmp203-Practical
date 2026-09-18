#pragma once
#include <map>
#include "Skateboard/Graphics/Resources/CommonResources.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Memory/VirtualAllocator.h"
#include "Skateboard/Scene/AABB.h"

using namespace Skateboard::MemoryUtils;

namespace Skateboard
{
    struct PrimitiveSuballocationSingleBuffer
    {
        BlockAllocator* ParentAllocator;
        VirtualAllocation PrimitiveData;

        //Release the Allocations on Deletion of the Primitive
		  ~PrimitiveSuballocationSingleBuffer()
        {
            if (ParentAllocator)
            {
                ParentAllocator->FreeAllocation(PrimitiveData);
            }
        }
    };

    struct PrimitiveBuffers
    {
        BufferRef VertexBuffer;
        BufferRef IndexBuffer;
    };


    struct Primitive
    {
        Primitive()
	        :
    	      Layout()
	        , Allocations()
	        , IndexBuffer(), BoundBox()
        {
        }

        Primitive(const Primitive& rhs)
	        :
	        Layout(rhs.Layout)
	        , Allocations(rhs.Allocations)
	        , VertexBuffers(rhs.VertexBuffers)
	        , IndexBuffer(rhs.IndexBuffer)
    		, BoundBox(rhs.BoundBox)
        {
        }

        auto operator=(const Primitive& rhs) noexcept -> Primitive&{
            Layout=rhs.Layout;
            VertexBuffers =rhs.VertexBuffers;
            IndexBuffer =rhs.IndexBuffer;
            Allocations = rhs.Allocations;
            BoundBox = rhs.BoundBox;
            return *this;
        }

        Primitive(Primitive&& rhs) noexcept
	        :
    			Layout(rhs.Layout)
			, Allocations(std::move(rhs.Allocations))
			, VertexBuffers(std::move(rhs.VertexBuffers))
	        , IndexBuffer(std::move(rhs.IndexBuffer))
    	    , BoundBox(rhs.BoundBox)
        {
        }

        auto operator=(Primitive&& rhs) noexcept -> Primitive& {
            Layout = rhs.Layout;
            VertexBuffers = std::move(rhs.VertexBuffers);
            IndexBuffer = std::move(rhs.IndexBuffer);
            Allocations = std::move(rhs.Allocations);
            BoundBox = rhs.BoundBox;
            return *this;
        }

        BufferLayout Layout;                        // Vertex data layout
		std::shared_ptr<PrimitiveSuballocationSingleBuffer>  Allocations;

        std::map<uint8_t,VertexBufferView> VertexBuffers;
        IndexBufferView IndexBuffer;

        AABB BoundBox;
    };

	class Mesh
	{
	public:
        virtual ~Mesh() = default;
        Mesh() = default;
        explicit Mesh(const wchar_t* filename);
        explicit Mesh(const std::vector<Primitive>& meshes);

        BufferLayout GetPrimitiveLayout(uint32_t primitiveIDX) { return m_Primitives[primitiveIDX].Layout; }

        VertexBufferView* GetVertexBuffer(uint32_t PrimtiveIDX, uint8_t inputSlot = 0)
        {
	        if(m_Primitives[PrimtiveIDX].Layout.GetSlotsAndStrides().contains(inputSlot))
                return &m_Primitives[PrimtiveIDX].VertexBuffers[inputSlot];
			else
				return nullptr;
        };

        IndexBufferView* GetIndexBuffer(uint32_t PrimtiveIDX) { return &m_Primitives[PrimtiveIDX].IndexBuffer; };

        Primitive* GetPrimitive(uint32_t meshElement) { return &m_Primitives[meshElement]; }
        const Primitive* GetPrimitive(uint32_t meshElement) const {return &m_Primitives[meshElement]; }

        uint32_t GetPrimitiveCount() const { return static_cast<uint32_t>(m_Primitives.size()); }
        const std::vector<Primitive>& GetPrimitives() const { return m_Primitives; }

        std::vector<Primitive>::iterator begin() { return m_Primitives.begin(); }
        std::vector<Primitive>::iterator end()   { return m_Primitives.end(); }

    public:
        Transform Offset;
    protected:
        std::vector<Primitive>       m_Primitives;
	};
}
