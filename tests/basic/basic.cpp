#include <cstdlib>
#include <fmt/printf.h>
#include <ez/MemoryPool.hpp>

int main() {
	fmt::print("Hello, world!\n");

	ez::MemoryPool<int> pool;
	int* a = pool.alloc();
	int* b = pool.alloc();
	int* c = pool.alloc();

	*a = 7;
	*b = 25;
	*c = 1996;

	fmt::print("a == {}, b == {}, c == {}\n", *a, *b, *c);
	fmt::print("Pool capacity == {}\n", pool.capacity());
	fmt::print("Pool size == {}\n", pool.size());

	for (int value : pool) {
		fmt::print("Found a value: {}\n", value);
	}

	const ez::MemoryPool<int>& cpool = pool;
	for (int value : cpool) {
		fmt::print("Found a const value: {}\n", value);
	}

	pool.free(b);
	fmt::print("\nFreed an element\n\n");


	for (int value : pool) {
		fmt::print("Found a value: {}\n", value);
	}
	
	for (int value : cpool) {
		fmt::print("Found a const value: {}\n", value);
	}

	pool.clear();
	for (int value : pool) {
		fmt::print("There shouldn't be any values!: {}\n", value);
	}

	fmt::print("Forcing allocation of another block.\n");
	for (int i = 0; i < 512; ++i) {
		pool.create(i);
	}

	fmt::print("Pool capacity == {}\n", pool.capacity());
	fmt::print("Pool size == {}\n", pool.size());

	fmt::print("Printing out a long list of values from the pool; note that they may not be in order, and that is to be expected.\n");
	for (int val : pool) {
		fmt::print("{} ", val);
	}
	fmt::print("\n\n");

	pool.destroy_clear();


	srand(time(0));

	for (int i = 0; i < 100; ++i) {
		int* tmp = pool.create();
		*tmp = (rand() % 2);
	}
	
	for (auto iter = pool.begin(), end = pool.end(); iter != end;) {
		if (*iter == 1) {
			iter = pool.erase(iter);
		}
		else {
			++iter;
		}
	}

	fmt::print("Should print out all zeros:\n");
	for (int val : pool) {
		fmt::print("{} ", val);
	}
	fmt::print("\n\n");
	
	fmt::print("End basic test.\n");
	return 0;
}