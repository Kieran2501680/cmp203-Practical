#pragma once
#include "Skateboard/Graphics/InternalFormats.h"
#include "Skateboard/Graphics/RHI/GraphicsSettingsDefines.h"
#include "Skateboard/Core.h"


namespace Skateboard
{
	// A 'GpuResource' is a resource that will be transferred to the platform GPU in some way

	class GPUResource
	{
		//DISABLE_COPY_AND_MOVE(GPUResource);
	public:
		GPUResource() = default;

		constexpr virtual GPUResourceType_ GetResourceType() = 0;
		virtual ~GPUResource() = default;

	protected:
#ifndef SKTBD_SHIP
		std::wstring DebugName;
	public:
		virtual void SetDebugName(const std::wstring& debug_name) { DebugName = debug_name; }
#else
	public:
		virtual void SetDebugName(const std::wstring& debug_name) { }
#endif
	};

	template<typename T>
	concept IsPointerLike = requires(const T & t) {
		// 1. Check if it's a raw pointer type. This is a primary check.
		// We use std::is_pointer_v for a direct type trait check.
		{ std::is_pointer_v<T> } -> std::convertible_to<bool>;

		// 2. OR, check if it behaves like a pointer via operator->.
		// This expression checks if `t.operator->()` is a valid operation.
		// This part of the disjunction is essential for smart pointers.
		//{ t.operator->() } -> std::convertible_to<std::remove_reference_t<decltype(t.operator->())>>;
			
		// 3. AND, check that dereferencing is possible.
		// This ensures we can get to the underlying object.
		{ *t };
	};

	template<typename Resource, uint32_t ResourceCount = GRAPHICS_SETTINGS_NUMFRAMERESOURCES>
	class RingArray : public std::array<Resource, ResourceCount>
	{
		static_assert(ResourceCount > 0);
		using Base = std::array<Resource, ResourceCount>;

	public:
		template<typename... Args>
		RingArray() : m_internalCounter(0) { };

		static constexpr uint32_t GetHandleCount() { return  ResourceCount; }

		template<IsPointerLike T>
		auto operator->() { return Base::operator->(); }
		
		auto& Get() const { return Base::_Elems[m_internalCounter]; }
		auto& Get() { return Base::_Elems[m_internalCounter]; }

		auto& GetPrevious() const { return Base::_Elems[(m_internalCounter) ? (m_internalCounter - 1) % ResourceCount : ResourceCount - 1]; }
		auto& GetNext() const { return Base::_Elems[(m_internalCounter + 1) % ResourceCount]; }

		auto& GetPrevious() { return Base::_Elems[(m_internalCounter) ? (m_internalCounter - 1) % ResourceCount : ResourceCount - 1]; }
		auto& GetNext() { return Base::_Elems[(m_internalCounter + 1) % ResourceCount]; }

		void SetCounter(uint32_t new_idx)
		{
			ASSERT_SIMPLE(new_idx < ResourceCount, "Frame Index Out of Range for This Resource");
			m_internalCounter = new_idx;
		}

		void IncrementCounter()
		{
			m_internalCounter = (m_internalCounter + 1) % ResourceCount;
		}

		void DecrementCounter()
		{
			m_internalCounter = (m_internalCounter) ? ( m_internalCounter - 1) % ResourceCount : ResourceCount - 1;
		}

		void ForEach(std::function<void(Resource&)>func)
		{
			for (auto& i : *this) func(i);
		}

		auto& operator[](size_t idx) { return Base::_Elems[idx % ResourceCount]; }
		const auto& operator[](size_t idx) const { return Base::_Elems[idx % ResourceCount]; }

		Resource& operator++()
		{
			IncrementCounter();
			return Base::_Elems[m_internalCounter];
		}

		Resource& operator++(int)
		{
			IncrementCounter();
			return GetPrevious();
		}

		Resource& operator--()
		{
			DecrementCounter();
			return Base::_Elems[m_internalCounter];
		}

		Resource& operator--(int)
		{
			DecrementCounter();
			return GetNext();
		}

	protected:
		uint32_t m_internalCounter;
	};

	

	template <class Resource, class Allocator = std::allocator<Resource>>
	class RingVector: public std::vector<Resource,Allocator>
	{
		
		using Base = std::vector<Resource, Allocator>;

	public:
		template<typename... Args>
		RingVector() : m_internalCounter(0) {}; 

		template<IsPointerLike T>
		auto operator->() { return Base::operator->(); }

		auto& Get() const { return Base::_Elems[m_internalCounter]; }
		auto& Get() { return Base::_Elems[m_internalCounter]; }

		auto& GetPrevious() const { return Base::_Elems[(m_internalCounter) ? (m_internalCounter - 1) % Base::size() : Base::size() - 1]; }
		auto& GetNext() const { return Base::_Elems[(m_internalCounter + 1) % Base::size()]; }

		auto& GetPrevious() { return Base::_Elems[(m_internalCounter) ? (m_internalCounter - 1) % Base::size() : Base::size() - 1]; }
		auto& GetNext() { return Base::_Elems[(m_internalCounter + 1) % Base::size()]; }

		void SetCounter(uint32_t new_idx)
		{
			ASSERT_SIMPLE(new_idx < Base::size(), "Frame Index Out of Range for This Resource");
			m_internalCounter = new_idx;
		}

		void IncrementCounter()
		{
			m_internalCounter = (m_internalCounter + 1) % Base::size();
		}

		void DecrementCounter()
		{
			m_internalCounter = (m_internalCounter) ? (m_internalCounter - 1) % Base::size() : Base::size() - 1;
		}

		void ForEach(std::function<void(Resource&)>func)
		{
			for (auto& i : *this) func(i);
		}

		auto& operator[](size_t idx) { return Base::_Elems[idx % Base::size()]; }
		const auto& operator[](size_t idx) const { return Base::_Elems[idx % Base::size()]; }

		constexpr void resize(size_t newsize)
		{
			Base::resize(newsize);
			m_internalCounter = m_internalCounter % newsize;
		}

		constexpr void pop_back()
		{
			if (m_internalCounter == Base::size() - 1)
				DecrementCounter();
			Base::pop_back();
		}

		Resource& operator++()
		{
			IncrementCounter();
			return Base::_Elems[m_internalCounter];
		}

		Resource& operator++(int)
		{
			IncrementCounter();
			return GetPrevious();
		}

		Resource& operator--()
		{
			DecrementCounter();
			return Base::_Elems[m_internalCounter];
		}

		Resource& operator--(int)
		{
			DecrementCounter();
			return GetNext();
		}

	protected:
		uint32_t m_internalCounter;
	};
};