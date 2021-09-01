# ez-pool
Memory pools and object pools for modern c++

### CMake
```cmake
find_package(ez-pool CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE ez::pool)
```

### C++
```cpp
#include <ez/MemoryPool.hpp>

int main() {
	// Create the pool
	ez::MemoryPool<int> pool;
	
	// Allocate some objects
	int * obj0 = pool.alloc();
	int * obj1 = pool.alloc();
	
	// Returns nullptr if allocation fails
	if(obj0 == nullptr) {
		printf("Failed to allocate!");
		return -1;
	}
	if(obj1 == nullptr) {
		printf("Failed to allocate!");
		return -1;
	}
	
	// Free the pointers when done
	pool.free(obj0);
	pool.free(obj1);
    
    // No destructors are called for the objects inside the basic memory pool.
    // To allocate and construct, call the create method
    obj0 = pool.create(0);
    obj1 = pool.create(1);
    
    // Then call the destroy method to destruct and free.
    pool.destroy(obj0);
    pool.destroy(obj1);
    
    // Use the LMemoryPool class when you want that to be the default behavior.
	
	return 0;
}
```