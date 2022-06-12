#include <catch2/catch_all.hpp>
#include <cstdlib>
#include <fmt/printf.h>
#include <ez/ObjectPool.hpp>
#include <string>

TEST_CASE("object pools") {
	using pool_t = ez::ObjectPool<std::string>;

	pool_t pool;

	std::string * str = pool.create("Hello, world!");
	REQUIRE(str != nullptr);

	REQUIRE(pool.size() == 1);
	REQUIRE(pool.contains(str));

	auto iter = pool.begin();
	REQUIRE(&(*iter) == str);

	fmt::print("Quick! Whats a good way to learn some programming?\n");
	fmt::print("Doing a quick '{}' program of course.\n", *str);

	
}