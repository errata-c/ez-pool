#pragma once
#include "MemoryPool.hpp"

namespace ez {
	/*
	Live memory pool, does not allow allocation of uninitialized memory. Only allows construction in place, and destruction.

	*/
	template<template<typename K, typename V> typename map_template, typename T, std::size_t N>
	class BasicObjectPool  {
	public:
		using self_t = BasicObjectPool<map_template, T, N>;
		using parent_type = BasicMemoryPool<map_template, T, N>;
		using iterator = typename parent_type::iterator;
		using const_iterator = typename parent_type::const_iterator;

		BasicObjectPool()
		{}
		BasicObjectPool(BasicObjectPool&& other) noexcept
			: mpool(std::move(other.mpool))
		{}
		~BasicObjectPool() {
			mpool.destroy_clear();
		}
		BasicObjectPool& operator=(BasicObjectPool&& other) noexcept {
			mpool = std::move(other.mpool);
			return *this;
		}

		// forwards parameters to object constructor
		template<typename ... Ts>
		T* create(Ts&&... args) {
			return mpool.create(std::forward<Ts>(args)...);
		}

		void destroy(T* obj) {
			mpool.destroy(obj);
		}

		void shrink() {
			mpool.shrink();
		}

		void reserve(size_t cap) {
			mpool.reserve(cap);
		}

		void clear() {
			mpool.destroy_clear();
		}

		ptrdiff_t capacity() const noexcept {
			return mpool.capacity();
		}
		ptrdiff_t size() const noexcept {
			return mpool.size();
		}

		bool empty() const noexcept {
			return mpool.empty();
		}

		iterator begin() noexcept {
			return mpool.begin();
		}
		iterator end() noexcept {
			return mpool.end();
		}

		const_iterator begin() const noexcept {
			return mpool.begin();
		}
		const_iterator end() const noexcept {
			return mpool.end();
		}

		const_iterator cbegin() const noexcept {
			return mpool.cbegin();
		}
		const_iterator cend() const noexcept {
			return mpool.cend();
		}

		iterator erase(const_iterator pos) {
			return mpool.erase(pos);
		}

		void swap(self_t& other) noexcept {
			mpool.swap(other.mpool);
		}

		bool contains(const T* obj) const {
			return mpool.contains(obj);
		}
		iterator find(const T* obj) {
			return mpool.find(obj);
		}
		const_iterator find(const T* obj) const {
			return mpool.find(obj);
		}


	private:
		parent_type mpool;
	};

	template<typename T, std::size_t N = 256>
	using ObjectPool = BasicObjectPool<std::unordered_map, T, N>;
};