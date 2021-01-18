#include <fmt/printf.h>
#include <cstdlib>
#include <ctime>
#include <ez/intern/MemoryBlock.hpp>

int main() {
	fmt::print("Begin MemoryBlock test\n");

	ez::intern::MemoryBlock<int> pool;
	int* p0 = pool.alloc();
	int* p1 = pool.alloc();
	int* p2 = pool.alloc();

	*p0 = 32;
	*p1 = 64;
	*p2 = 128;

	for (int val : pool) {
		fmt::print("{} ", val);
	}
	fmt::print("\n");
	for (int val : static_cast<const ez::intern::MemoryBlock<int>&>(pool)) {
		fmt::print("{} ", val);
	}
	fmt::print("\n");
	
	{
		auto iter = pool.rbegin();
		auto end = pool.rend();
		for (; iter != end; ++iter) {
			int val = *iter;
			fmt::print("{} ", val);
		}
		fmt::print("\n");
	}
	{
		auto iter = pool.crbegin();
		auto end = pool.crend();
		for (; iter != end; ++iter) {
			int val = *iter;
			fmt::print("{} ", val);
		}
		fmt::print("\n");
	}
	
	pool.clear();
	srand(time(0));

	for (int i = 0; i < 256; ++i) {
		int* ptr = pool.alloc();
		*ptr = (rand() % 2);
	}

	fmt::print("Printing random values:\n");
	for (int val : pool) {
		fmt::print("{} ", val);
	}
	fmt::print("\nPruning all non-zero values:\n");
	{
		auto iter = pool.begin();
		auto end = pool.end();
		while (iter != end) {
			if (*iter == 1) {
				iter = pool.erase(iter);
			}
			else {
				++iter;
			}
		}

		for (int val : pool) {
			fmt::print("{} ", val);
		}
		fmt::print("\n");
	}

	fmt::print("End MemoryBlock test\n");


	return 0;
}