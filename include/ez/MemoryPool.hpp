#pragma once
#include <new>
#include <array>
#include <memory>
#include <vector>
#include <algorithm>
#include <cinttypes>
#include <cassert>
#include <bitset>
#include <parallel_hashmap/phmap.h>
#include "intern/MemoryBlock.hpp"

namespace ez {
	// Can allocate without referencing the map at all, only one pointer indirection to the topmost block.
	// Can deallocate by accessing the map just once. That should be fairly performant.
	template<typename T, std::size_t BlockSize = 256>
	class MemoryPool {
	private:
		using Block = ez::intern::MemoryBlock<T, BlockSize>;
		using Alloc = std::array<Block*, 2>;

		using map_t = phmap::flat_hash_map<void*, Alloc>;
		using map_iterator = typename map_t::iterator;
		using const_map_iterator = typename map_t::const_iterator;
		using block_iterator = typename Block::iterator;

		//static constexpr std::ptrdiff_t align = alignof(T);
		static constexpr std::ptrdiff_t BlockBytes = sizeof(Block);
		
		std::vector<Block*> freeList;
		map_t map;

		std::ptrdiff_t count, bcount;

		// Top pointer to avoid vector indirection for most allocations.
		Block* top;
	public:
		class iterator;
		class const_iterator;

		MemoryPool()
			: count(0)
			, bcount(0)
			, top(nullptr)
		{}
		MemoryPool(MemoryPool && other) noexcept
			: count(other.count)
			, bcount(other.bcount)
			, freeList(std::move(other.freeList))
			, map(std::move(other.map))
			, top(other.top)
		{
			other.count = 0;
			other.bcount = 0;
			other.top = nullptr;
		}
		~MemoryPool() {
			for (auto && iter : map) {
				if (iter.second[0] != nullptr) {
					delete iter.second[0];
				}
			}
		}

		MemoryPool& operator=(MemoryPool&& other) noexcept {
			count = other.count;
			bcount = other.bcount;
			freeList = std::move(other.freeList);
			map = std::move(other.map);
			other.count = 0;
			other.bcount = 0;
			other.top = nullptr;
			return *this;
		}
		
		T * alloc() {
			if (top == nullptr) {
				top = createBlock();
				if (top == nullptr) {
					return nullptr;
				}
				
				freeList.push_back(top);
			}
			
			T* obj = top->alloc();

			if (top->numFree == 0) {
				freeList.pop_back();
				if (!freeList.empty()) {
					top = freeList.back();
				}
				else {
					top = nullptr;
				}
			}

			++count;
			return obj;
		}
		void free(T* obj) {
			void* id = blockId(obj);
			map_iterator iter = map.find(id);
			assert(iter != map.end()); // This assertion triggers if the obj is not from this pool

			for (Block * block : iter->second) {
				if ((reinterpret_cast<std::uintptr_t>(block) < reinterpret_cast<std::uintptr_t>(obj)) && 
					((reinterpret_cast<std::uintptr_t>(block)+BlockBytes) > reinterpret_cast<std::uintptr_t>(obj))) {
					block->free(obj);
					if (block->numFree == 1) {
						freeList.push_back(block);
						top = block;
					}
					--count;
					return;
				}
			}
			assert(false); // This assertion triggers if the obj is not from this pool
		}

		template<typename ... Ts>
		T* create(Ts&&... args) {
			T* obj = alloc();
			new (obj) T{std::forward<Ts>(args)...};
			return obj;
		}

		void destroy(T * obj) {
			obj->~T();
			free(obj);
		}

		void destroy_clear() {
			for (T & obj : *this) {
				obj.~T();
			}
			clear();
		}

		void shrink() {
			auto iter = freeList.begin();
			while (iter != freeList.end()) {
				Block* block = *iter;
				if (block->numFree == BlockSize) {
					destroyBlock(block);
					--bcount;
					*iter = freeList.back();
					freeList.pop_back();
				}
				else {
					++iter;
				}
			}

			if (!freeList.empty()) {
				top = freeList.back();
			}
			else {
				top = nullptr;
			}
		}

		void clear() {
			freeList.clear();
			for (auto&& iter : map) {
				if (iter.second[0] != nullptr) {
					delete iter.second[0];
				}
			}
			map.clear();
			count = 0;
			bcount = 0;
			top = nullptr;
		}

		std::ptrdiff_t capacity() const {
			return static_cast<std::ptrdiff_t>(bcount * BlockSize);
		}
		std::ptrdiff_t size() const {
			return count;
		}

		iterator begin() {
			return iterator(map.begin(), map.end());
		}
		iterator end() {
			return iterator(map.end(), map.end());
		}

		const_iterator begin() const {
			return const_iterator(const_cast<MemoryPool*>(this)->begin());
		}
		const_iterator end() const {
			return const_iterator(const_cast<MemoryPool*>(this)->end());
		}

		const_iterator cbegin() const {
			return const_iterator(const_cast<MemoryPool*>(this)->begin());
		}
		const_iterator cend() const {
			return const_iterator(const_cast<MemoryPool*>(this)->end());
		}

		iterator erase(const_iterator _pos) {
			iterator pos = _pos._inner;
			assert(pos != end());

			Block* block = pos.blockIter.getBlock();
			pos.blockIter = block->erase(pos.blockIter);
			if (block->numFree == 1) {
				freeList.push_back(block);
				top = block;
			}
			--count;

			if (pos.blockIter.atEnd()) {
				pos.nextBlock();
			}

			return pos;
		}

		void swap(MemoryPool& other) noexcept {
			map.swap(other.map);
			freeList.swap(other.freeList);
			std::swap(count, other.count);
			std::swap(top, other.top);
		}
	private:
		/// Utility methods
		static void* blockId(T* base) {
			std::uintptr_t offset = reinterpret_cast<std::uintptr_t>(base) % BlockBytes;
			return reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(base) - offset);
		}

		Block * createBlock() {
			Block* block = new Block{};
			if (!block) {
				return nullptr;
			}
			++bcount;

			void* low = blockId(block->basePtr());
			void* high = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(low) + BlockBytes);

			auto iter = map.find(low);
			if (iter == map.end()) {
				map.insert(iter, { low, Alloc{ nullptr, block } });
			}
			else {
				iter->second[1] = block;
			}

			iter = map.find(high);
			if (iter == map.end()) {
				map.insert(iter, {high, Alloc{ block, nullptr }});
			}
			else {
				iter->second[0] = block;
			}

			return block;
		}
		void destroyBlock(Block * block) {
			void * low = blockId(block->basePtr());
			void * high = low + BlockBytes;

			auto iter = map.find(low);
			assert(iter != map.end()); // The block must exist
			iter->second[1] = nullptr;
			if (iter->second[0] == nullptr) {
				map.erase(iter);
			}

			iter = map.find(high);
			assert(iter != map.end()); // The block must exist
			iter->second[0] = nullptr;
			if (iter->second[1] == nullptr) {
				map.erase(iter);
			}

			--bcount;
			delete block;
		}

		
	public:
		class iterator {
			bool validBlock() const {
				return (mapIter->second[0] != nullptr) && (mapIter->second[0]->size() != 0);
			}
		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = T;
			using reference = value_type&;
			using pointer = value_type*;
			using difference_type = std::ptrdiff_t;

			iterator()
			{}
			iterator(map_iterator it, map_iterator end)
				: mapIter(it)
				, mapEnd(end)
			{
				if (mapIter != mapEnd) {
					if (validBlock()) {
						blockIter = mapIter->second[0]->begin();
					}
					else {
						nextBlock();
					}
				}
			}
			iterator(map_iterator it, map_iterator end, block_iterator bit)
				: mapIter(it)
				, mapEnd(end)
				, blockIter(bit)
			{}
			~iterator()
			{}

			iterator(const iterator& other) = default;

			iterator& operator=(const iterator& other) = default;

			iterator& operator++() {
				++blockIter;
				if (blockIter.atEnd()) {
					nextBlock();
				}
				return *this;
			}
			iterator operator++(int) {
				iterator copy = *this;
				++(*this);
				return copy;
			}

			bool operator==(const iterator& other) const {
				return (mapIter == other.mapIter) && (blockIter == other.blockIter);
			}
			bool operator!=(const iterator& other) const {
				return (mapIter != other.mapIter) || (blockIter != other.blockIter);
			}

			reference operator*() {
				return *blockIter;
			}
			pointer operator->() {
				return &*blockIter;
			}
		protected:
			friend class MemoryPool;
			map_iterator mapIter, mapEnd;
			block_iterator blockIter;

			void nextBlock() {
				blockIter = block_iterator{};

				++mapIter;
				while (mapIter != mapEnd) {
					if (validBlock()) {
						blockIter = mapIter->second[0]->begin();
						break;
					}
					++mapIter;
				}
			}
		}; // End iterator
		
		class const_iterator {
		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = const T;
			using reference = value_type&;
			using pointer = value_type*;
			using difference_type = std::ptrdiff_t;

			const_iterator() = default;
			const_iterator(const const_iterator&) = default;
			const_iterator& operator=(const const_iterator&) = default;

			const_iterator(map_iterator it, map_iterator end)
				: _inner(it, end)
			{}
			const_iterator(map_iterator it, map_iterator end, typename Block::iterator bit)
				: _inner(it, end, bit)
			{}

			const_iterator(const iterator & other) 
				: _inner(other)
			{}
			const_iterator& operator=(const iterator& other) {
				_inner = other;
				return *this;
			}

			const_iterator& operator++() {
				++_inner;
				return *this;
			}
			const_iterator operator++(int) {
				const_iterator copy = *this;
				++(*this);
				return copy;
			}

			bool operator==(const const_iterator& other) const {
				return _inner == other._inner;
			}
			bool operator!=(const const_iterator& other) const {
				return _inner != other._inner;
			}

			reference operator*() {
				return *_inner;
			}
			pointer operator->() {
				return &*_inner;
			}
		private:
			friend class MemoryPool;
			iterator _inner;
		};
	};
};