#include <cstdlib>
#include <memory>
#include <string>
#include <fmt/core.h>
#include <ez/MemoryPool.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("basic") {
	ez::MemoryPool<int> pool;

	REQUIRE(pool.size() == 0);
	REQUIRE(pool.empty());

	int* a = pool.alloc();
	REQUIRE(pool.size() == 1);
	int* b = pool.alloc();
	REQUIRE(pool.size() == 2);
	int* c = pool.alloc();
	REQUIRE(pool.size() == 3);

	REQUIRE(pool.capacity() == 256);

	*a = 7;
	*b = 25;
	*c = 1996;


	{
		auto it = pool.begin();
		auto end = pool.end();

		REQUIRE(*it == *a);
		REQUIRE(it != end);
		++it;
		REQUIRE(*it == *b);
		REQUIRE(it != end);
		++it;
		REQUIRE(*it == *c);
		REQUIRE(it != end);
		++it;

		REQUIRE(it == end);
	}

	pool.free(b);
	REQUIRE(pool.size() == 2);
	
	{
		auto it = pool.begin();
		auto end = pool.end();

		REQUIRE(*it == *a);
		REQUIRE(it != end);
		++it;

		REQUIRE(*it == *c);
		REQUIRE(it != end);
		++it;

		REQUIRE(it == end);
	}

	pool.clear();
	REQUIRE(pool.empty());

	for (int i = 0; i < 257; ++i) {
		pool.create(i);
	}

	REQUIRE(pool.capacity() == 512);
	REQUIRE(pool.size() == 257);

	pool.destroy_clear();
	REQUIRE(pool.empty());

	for (int i = 0; i < 100; ++i) {
		pool.create(i % 2);
	}
	
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

TEST_CASE("Move assign memory pool") {
	using strong_ref = std::shared_ptr<std::string>;
	using weak_ref = std::weak_ptr<std::string>;
	
	ez::MemoryPool<strong_ref> pool;
	weak_ref tracker;
	{
		strong_ref * ptr = pool.create(new std::string("This is a string, DUH"));
		tracker = *ptr;
	}

	REQUIRE(pool.size() == 1);
	REQUIRE(!tracker.expired());

	pool.create(tracker.lock());
	pool.create(tracker.lock());

	REQUIRE(tracker.use_count() == 3);

	ez::MemoryPool<strong_ref> second = std::move(pool);
	REQUIRE(second.size() == 3);
	REQUIRE(tracker.use_count() == 3);


	second.destroy_clear();
	REQUIRE(second.size() == 0);
	REQUIRE(second.empty());

	REQUIRE(tracker.expired());
}

