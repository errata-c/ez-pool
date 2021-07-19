#include <catch.hpp>
#include <fmt/printf.h>
#include <cstdlib>
#include <ctime>
#include <ez/intern/MemoryBlock.hpp>

using Block = ez::intern::MemoryBlock<int, 256>;

TEST_CASE("blocks") {
	Block pool;

	REQUIRE(pool.size() == 0);
	REQUIRE(pool.empty());

	int* p0 = pool.alloc();
	int* p1 = pool.alloc();
	int* p2 = pool.alloc();

	REQUIRE(pool.contains(p0));
	REQUIRE(pool.contains(p1));
	REQUIRE(pool.contains(p2));

	REQUIRE(pool.isAllocated(p0));
	REQUIRE(pool.isAllocated(p1));
	REQUIRE(pool.isAllocated(p2));

	*p0 = 32;
	*p1 = 64;
	*p2 = 128;

	{
		Block::iterator it = pool.begin();
		Block::iterator end = pool.end();

		REQUIRE(&(*it) == p0);
		++it;
		REQUIRE(&(*it) == p1);
		++it;
		REQUIRE(&(*it) == p2);
		++it;

		REQUIRE(it == end);
	}

	{
		Block::const_iterator it = pool.cbegin();
		Block::const_iterator end = pool.cend();

		REQUIRE(&(*it) == p0);
		++it;
		REQUIRE(&(*it) == p1);
		++it;
		REQUIRE(&(*it) == p2);
		++it;

		REQUIRE(it == end);
	}
	
	pool.clear();
	REQUIRE(pool.empty());

	srand(time(0));

	for (int i = 0; i < pool.max_size(); ++i) {
		int* ptr = pool.alloc();
		*ptr = (rand() % 2);
	}

	REQUIRE(pool.size() == pool.max_size());
	
	{
		auto iter = pool.begin();
		auto end = pool.end();
		while (iter != end) {
			if (*iter != 0) {
				iter = pool.erase(iter);
			}
			else {
				++iter;
			}
		}
	}

	int count = 0;
	for (int val : pool) {
		if (val != 0) {
			++count;
		}
	}
	REQUIRE(count == 0);
}