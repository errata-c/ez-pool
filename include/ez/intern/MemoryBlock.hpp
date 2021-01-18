#pragma once
#include <cinttypes>
#include <array>
#include <bitset>
#include <cassert>

namespace ez::intern {
	template<typename T, std::size_t BlockSize = 256>
	class MemoryBlock {
	public:
		union Memory {
			Memory() : index(0) {};
			~Memory() {};

			std::uint8_t index;
			T object;
		};

		class iterator;
		class const_iterator;

		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		MemoryBlock()
			: top(0)
			, numFree(BlockSize)
		{
			int index = 1;
			for (Memory& mem : data) {
				mem.index = static_cast<std::uint8_t>(index);
				++index;
			}
		}

		T* alloc() {
			assert(numFree != 0);
			--numFree;
			Memory& mem = data[top];
			top = static_cast<std::uint16_t>(mem.index);
			return &mem.object;
		}
		void free(const T* obj) {
			assert(!empty());
			++numFree;
			int offset = static_cast<int>(obj - basePtr());
			data[offset].index = static_cast<std::uint8_t>(top);
			top = static_cast<std::int16_t>(offset);
		}

		T* basePtr() {
			return &data.front().object;
		}
		const T* basePtr() const {
			return &data.front().object;
		}

		bool boundsCheck(T* obj) const {
			return obj >= (&data.data()->object) && obj < ((&data.data()->object) + BlockSize);
		}

		iterator begin() {
			iterator tmp(this);
			tmp.findFirst();
			return tmp;
		}
		iterator end() {
			return iterator(this, BlockSize);
		}
		
		const_iterator begin() const {
			const_iterator tmp(this);
			tmp.findFirst();
			return tmp;
		}
		const_iterator end() const {
			return const_iterator(this, BlockSize);
		}

		const_iterator cbegin() const {
			return begin();
		}
		const_iterator cend() const {
			return end();
		}

		reverse_iterator rbegin() {
			return std::make_reverse_iterator(end());
		}
		reverse_iterator rend() {
			return std::make_reverse_iterator(begin());
		}

		const_reverse_iterator rbegin() const {
			return std::make_reverse_iterator(end());
		}
		const_reverse_iterator rend() const {
			return std::make_reverse_iterator(begin());
		}

		const_reverse_iterator crbegin() const {
			return std::make_reverse_iterator(end());
		}
		const_reverse_iterator crend() const {
			return std::make_reverse_iterator(begin());
		}

		iterator erase(const_iterator pos) {
			assert(pos != cend());
			pos.marks[pos.index] = true;
			free(&*pos);
			++pos;

			return iterator(this, pos.index, pos.marks);
		}
		iterator erase(const_iterator first, const_iterator last) {
			assert(first != cend());
			while (first != last) {
				first.marks[first.index] = true;
				free(&*first);
				++first;
			}
			return iterator(this, first.index, first.marks);
		}

		void clear() {
			numFree = BlockSize;
			top = 0;
			int index = 1;
			for (Memory & mem : data) {
				mem.index = static_cast<std::uint8_t>(index);
				++index;
			}
		}

		bool empty() const {
			return numFree == BlockSize;
		}

		std::size_t size() const {
			return BlockSize - numFree;
		}
		static constexpr std::size_t max_size() {
			return BlockSize;
		}

		std::int16_t top, numFree;
		std::array<Memory, BlockSize> data;
	
		// Impl iterators
		template<typename Block, typename U>
		class iterator_impl {
		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = U;
			using reference = value_type&;
			using pointer = value_type*;
			using difference_type = std::ptrdiff_t;

			iterator_impl()
				: block(nullptr)
				, index(0)
			{}
			iterator_impl(Block* it, int _index)
				: block(it)
				, index(_index)
			{
				markBlock();
			}
			iterator_impl(Block* it, int _index, std::bitset<BlockSize> _marks)
				: block(it)
				, index(_index)
				, marks(_marks)
			{}
			~iterator_impl()
			{}

			iterator_impl(const iterator_impl& other) = default;

			iterator_impl& operator=(const iterator_impl& other) = default;

			iterator_impl& operator++() {				
				nextIndex();
				return *this;
			}
			iterator_impl operator++(int) {
				iterator_impl copy = *this;
				++(*this);
				return copy;
			}

			iterator_impl& operator--() {
				prevIndex();
				return *this;
			}
			iterator_impl operator--(int) {
				iterator_impl copy = *this;
				--(*this);
				return copy;
			}

			bool operator==(const iterator_impl& other) const {
				return (index == other.index);
			}
			bool operator!=(const iterator_impl& other) const {
				return (index != other.index);
			}

			reference operator*() {
				return block->data[index].object;
			}
			pointer operator->() {
				return &block->data[index].object;
			}

			void findFirst() {
				index = 0;
				while ((index < BlockSize) && marks.test(index)) {
					++index;
				}
			}
			void findLast() {
				index = BlockSize - 1;
				while ((index >= 0) && marks.test(index)) {
					++index;
				}
			}
		protected:
			friend class MemoryBlock;
			Block* block;
			int index;
			std::bitset<BlockSize> marks;

			void markBlock() {
				/// Mark all free elements
				int offset = block->top;
				for (int count = block->numFree; count > 0; --count) {
					marks.set(offset);
					offset = block->data[offset].index;
				}
			}

			void nextIndex() {
				++index;
				while ((index < BlockSize) && marks.test(index)) {
					++index;
				}
			}
			void prevIndex() {
				--index;
				while ((index >= 0) && marks.test(index)) {
					--index;
				}
			}
		}; // End iterator
	public:
		class iterator : public iterator_impl<MemoryBlock, T> {
		public:
			using base_t = iterator_impl<MemoryBlock, T>;

			iterator() = default;
			iterator(const iterator&) = default;
			iterator& operator=(const iterator&) = default;

			iterator(MemoryBlock * block, int _index = 0)
				: base_t(block, _index)
			{}
			iterator(MemoryBlock* block, int _index, std::bitset<BlockSize> _marks)
				: base_t(block, _index, _marks)
			{}

			MemoryBlock* getBlock() {
				return base_t::block;
			}
			bool inRange() const {
				return (base_t::index >= 0) && (base_t::index < BlockSize);
			}
			bool atStart() const {
				return base_t::index < 0;
			}
			bool atEnd() const {
				return base_t::index >= BlockSize;
			}

			friend class const_iterator;
			friend class MemoryBlock;
		};

		class const_iterator : public iterator_impl<const MemoryBlock, const T> {
		public:
			using base_t = iterator_impl<const MemoryBlock, const T>;

			const_iterator() = default;
			const_iterator(const const_iterator&) = default;
			const_iterator& operator=(const const_iterator&) = default;

			const_iterator(const MemoryBlock* block, int _index = 0)
				: base_t(block, _index)
			{}
			const_iterator(const MemoryBlock* block, int _index, std::bitset<BlockSize> _marks)
				: base_t(block, _index, _marks)
			{}

			const_iterator(const iterator& other) {
				base_t::block = other.block;
				base_t::index = other.index;
				base_t::marks = other.marks;
			}
			const_iterator& operator=(const iterator& other) {
				base_t::block = other.block;
				base_t::index = other.index;
				base_t::marks = other.marks;
				return *this;
			}

			const MemoryBlock* getBlock() {
				return base_t::block;
			}
			bool inRange() const {
				return (base_t::index >= 0) && (base_t::index < BlockSize);
			}
			bool atStart() const {
				return base_t::index < 0;
			}
			bool atEnd() const {
				return base_t::index >= BlockSize;
			}
		};
	};
};