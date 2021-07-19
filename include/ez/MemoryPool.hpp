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
	// Can allocate most of the time without referencing the map at all, only one pointer indirection to the topmost block.
	// Can deallocate by accessing the map just once. That should be fairly performant.
	// Note that the block size is not bytes, but a number of elements to store.
	// This means if you need to conserve memory, you may want to change the size from its default.
	template<typename T, std::size_t BlockSize = 256>
	class MemoryPool {
	private:
		static_assert(BlockSize <= 256, "BlockSize currently must be less than or equal to 256!");

		using Block = ez::intern::MemoryBlock<T, BlockSize>;
		using Alloc = std::array<Block*, 2>;

		using map_t = phmap::flat_hash_map<const void*, Alloc>;
		using map_iterator = typename map_t::iterator;
		using const_map_iterator = typename map_t::const_iterator;
		using block_iterator = typename Block::iterator;

		//static constexpr std::ptrdiff_t align = alignof(T);
		
		// Block size in bytes
		static constexpr std::ptrdiff_t BlockBytes = sizeof(Block);
		
		// List of blocks with openings
		std::vector<Block*> freeList;
		// Map of all allocated blocks
		map_t map;

		// count is the number of allocated objects
		// bcount is the number of allocated blocks
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

			// Block used up completely
			if (top->numFree == 0) {
				freeList.pop_back();
				if (!freeList.empty()) {
					// We have a block with openings available
					top = freeList.back();
				}
				else {
					// no empty places, next block will be allocated next call to alloc
					top = nullptr;
				}
			}
			
			++count;
			return obj;
		}

		// Frees an object from the pool. Note that it is always possible to check if object is from this pool.
		void free(T* obj) {
			const void* id = blockId(obj);
			map_iterator iter = map.find(id);

			// If this triggers, then the obj pointer is not within any valid block range
			assert(iter != map.end());

			// Check both blocks in the mapping
			for (Block * block : iter->second) {
				if (Block::contains(block, obj)) {

					// If this triggers, the obj pointer has already been freed.
					assert(block->isAllocated(obj) && "The object pointer has already been freed!");

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

		// forwards parameters to object constructor
		template<typename ... Ts>
		T* create(Ts&&... args) {
			T* obj = alloc();
			if (obj == nullptr) {
				return obj;
			}
			new (obj) T{std::forward<Ts>(args)...};
			return obj;
		}

		// destroys the object and frees its data.
		void destroy(T * obj) {
			assert(obj != nullptr);
			obj->~T();
			free(obj);
		}

		// destroy all, then clear
		void destroy_clear() {
			for (T & obj : *this) {
				obj.~T();
			}
			clear();
		}

		// eliminate all allocated blocks with no active elements.
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

		// attempt to reserve storage for 'cap' elements. Automatically rounds up to multiple of BlockSize
		void reserve(std::size_t cap) {
			std::size_t mod = cap % BlockSize;
			if (mod != 0) {
				cap += BlockSize - mod;
			}
			
			std::size_t nblocks = cap / BlockSize;
			while (bcount < nblocks) {
				Block* block = createBlock();
				if (block == nullptr) {
					break;
				}
				freeList.push_back(block);
			}
			top = freeList.back();
		}

		// Free all blocks WITHOUT calling destructors for the contained elements.
		// It is undefined behavior to call this method when elements have been constructed via calls to 'create'
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

		// Total capacity available
		std::ptrdiff_t capacity() const noexcept {
			return static_cast<std::ptrdiff_t>(bcount * BlockSize);
		}
		// Total number of allocations
		std::ptrdiff_t size() const noexcept {
			return count;
		}

		bool empty() const noexcept {
			return size() == 0;
		}

		iterator begin() noexcept {
			return iterator(map.begin(), map.end());
		}
		iterator end() noexcept {
			return iterator(map.end(), map.end());
		}

		const_iterator begin() const noexcept {
			return const_iterator(const_cast<MemoryPool*>(this)->begin());
		}
		const_iterator end() const noexcept {
			return const_iterator(const_cast<MemoryPool*>(this)->end());
		}

		const_iterator cbegin() const noexcept {
			return const_iterator(const_cast<MemoryPool*>(this)->begin());
		}
		const_iterator cend() const noexcept {
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

		bool contains(const T* obj) const {
			const void * block = blockId(obj);
			const_map_iterator iter = map.find(block);

			if (iter != map.end()) {
				return Block::contains(iter->second[0], obj) || Block::contains(iter->second[1], obj);
			}
			else {
				return false;
			}
		}

		iterator find(const T* obj) {
			void * block = blockId(obj);
			map_iterator iter = map.find(block);

			if (iter != map.end()) {
				if (Block::contains(iter->second[0], obj)) {
					return iterator(iter, map.end(), obj - iter->second[0]->basePtr());
				}
				else if (Block::contains(iter->second[1], obj)) {
					return iterator(iter, map.end(), obj - iter->second[1]->basePtr());
				}
				else {
					return end();
				}
			}
			else {
				return end();
			}
		}
		const_iterator find(const T* obj) const {
			const void* block = blockId(obj);
			const_map_iterator iter = map.find(block);

			if (iter != map.end()) {
				if (Block::contains(iter->second[0], obj)) {
					return const_iterator(iter, map.end(), obj - iter->second[0]->basePtr());
				}
				else if (Block::contains(iter->second[1], obj)) {
					return const_iterator(iter, map.end(), obj - iter->second[1]->basePtr());
				}
				else {
					return end();
				}
			}
			else {
				return end();
			}
		}
	private:
		/// Utility methods
		static const void* blockId(const T* base) noexcept {
			std::uintptr_t offset = reinterpret_cast<std::uintptr_t>(base) % BlockBytes;
			return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(base) - offset);
		}

		Block * createBlock() {
			Block* block = new Block{};
			if (!block) {
				return nullptr;
			}
			++bcount;

			const void* low = blockId(block->basePtr());
			const void* high = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(low) + BlockBytes);

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
			void * high = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(low) + BlockBytes);

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
			bool validBlock() const noexcept {
				return (mapIter->second[0] != nullptr) && (mapIter->second[0]->size() != 0);
			}
		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = T;
			using reference = value_type&;
			using pointer = value_type*;
			using difference_type = std::ptrdiff_t;

			iterator() noexcept = default;
			~iterator() = default;
			iterator(const iterator& other) noexcept = default;
			iterator& operator=(const iterator & other) noexcept = default;

			iterator(map_iterator it, map_iterator end) noexcept
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
			iterator(map_iterator it, map_iterator end, block_iterator bit) noexcept
				: mapIter(it)
				, mapEnd(end)
				, blockIter(bit)
			{}


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

			bool operator==(const iterator& other) const noexcept {
				return (mapIter == other.mapIter) && (blockIter == other.blockIter);
			}
			bool operator!=(const iterator& other) const noexcept {
				return (mapIter != other.mapIter) || (blockIter != other.blockIter);
			}

			reference operator*() noexcept {
				return *blockIter;
			}
			pointer operator->() noexcept {
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

			const_iterator() noexcept = default;
			~const_iterator() = default;
			const_iterator(const const_iterator&) noexcept = default;
			const_iterator& operator=(const const_iterator&) noexcept = default;

			const_iterator(map_iterator it, map_iterator end) noexcept
				: _inner(it, end)
			{}
			const_iterator(map_iterator it, map_iterator end, typename Block::iterator bit) noexcept
				: _inner(it, end, bit)
			{}

			const_iterator(const iterator & other) noexcept
				: _inner(other)
			{}
			const_iterator& operator=(const iterator& other) noexcept {
				_inner = other;
				return *this;
			}

			const_iterator& operator++() noexcept {
				++_inner;
				return *this;
			}
			const_iterator operator++(int) noexcept {
				const_iterator copy = *this;
				++(*this);
				return copy;
			}

			bool operator==(const const_iterator& other) const noexcept {
				return _inner == other._inner;
			}
			bool operator!=(const const_iterator& other) const noexcept {
				return _inner != other._inner;
			}

			reference operator*() noexcept {
				return *_inner;
			}
			pointer operator->() noexcept {
				return &*_inner;
			}
		private:
			friend class MemoryPool;
			iterator _inner;
		};
	};
};