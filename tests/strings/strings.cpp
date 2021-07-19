#include <fmt/printf.h>
#include <ez/MemoryPool.hpp>
#include <ez/LMemoryPool.hpp>

struct Printer {
	std::string text;

	Printer(std::string value): text(value) {
		fmt::print("Constructed: {}\n", text);
	}
	~Printer() {
		fmt::print("Destructed: {}\n", text);
	}
};

void strings() {
	{
		ez::LMemoryPool<std::string> pool;

		std::string* str = pool.create("Hello, World!\n");

		fmt::print("{}", *str);

		pool.clear();
	}
	{
		ez::LMemoryPool<Printer> pool;
		Printer * t0 = pool.create("Test0");
		pool.create("Test1");

		pool.destroy(t0);

		pool.create("Test2");
	}

}