#pragma once
#include "MemoryPool.hpp"

namespace ez {
	// Live memory pool, does not allow allocation of uninitialized memory. Only allows construction in place, and destruction.
	template<typename T>
	class LMemoryPool: public MemoryPool<T> {
	public:
		using iterator = typename MemoryPool<T>::iterator;
		using const_iterator = typename MemoryPool<T>::const_iterator;

		LMemoryPool()
		{}
		LMemoryPool(LMemoryPool && other) noexcept
			: MemoryPool<T>(std::move(other))
		{}
		LMemoryPool(MemoryPool<T>&& other) noexcept
			: MemoryPool<T>(std::move(other))
		{}
		~LMemoryPool() {
			MemoryPool<T>::destroy_clear();
		}
		LMemoryPool& operator=(LMemoryPool&& other) noexcept {
			MemoryPool<T>::operator=(std::move(other));
			return *this;
		}

		T* alloc() = delete;
		void free(T* obj) = delete;
		void destroy_clear() = delete;

		void clear() {
			MemoryPool<T>::destroy_clear();
		}
	};
};