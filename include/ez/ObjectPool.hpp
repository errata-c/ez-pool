#pragma once
#include "MemoryPool.hpp"

namespace ez {
	// Live memory pool, does not allow allocation of uninitialized memory. Only allows construction in place, and destruction.
	template<template<typename K, typename V> typename map_template, typename T, std::size_t N>
	class BasicObjectPool: public BasicMemoryPool<map_template, T, N> {
	public:
		using parent_type = BasicMemoryPool<map_template, T, N>;
		using iterator = typename parent_type::iterator;
		using const_iterator = typename parent_type::const_iterator;

		BasicObjectPool()
		{}
		BasicObjectPool(BasicObjectPool&& other) noexcept
			: parent_type(std::move(other))
		{}
		~BasicObjectPool() {
			parent_type::destroy_clear();
		}
		BasicObjectPool& operator=(BasicObjectPool&& other) noexcept {
			parent_type::operator=(std::move(other));
			return *this;
		}

		T* alloc() = delete;
		void free(T* obj) = delete;
		void destroy_clear() = delete;

		void clear() {
			parent_type::destroy_clear();
		}
	};

	template<typename T, std::size_t N = 256>
	using ObjectPool = BasicObjectPool<phmap::flat_hash_map, T, N>;

	template<typename T, std::size_t N = 256>
	using ParallelObjectPool = BasicObjectPool<phmap::parallel_flat_hash_map, T, N>;
};