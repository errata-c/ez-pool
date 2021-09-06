# ez-pool
Memory pools and object pools for c++.

Makes use of a unique allocation strategy to minimize overhead, allow for checking ownership of objects in the pool, and iterating over all the allocated objects.

### CMake
```cmake
find_package(ez-pool CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE ez::pool)
```

### C++
```cpp
#include <ez/MemoryPool.hpp>
#include <cassert>

int main() {
	// Create some memory pools
	ez::MemoryPool<int> pool0, pool1;
	
	// Allocate some objects
	int * obj0 = pool0.alloc();
	int * obj1 = pool1.alloc();
	
	// Returns nullptr if allocation fails
	if(obj0 == nullptr) {
		printf("Failed to allocate!");
		return -1;
	}
	if(obj1 == nullptr) {
		printf("Failed to allocate!");
		return -1;
	}
	
    // Check for ownership
    assert(pool0.contains(obj0));
    assert(pool1.contains(obj1));
    assert(!pool0.contains(obj1));
    assert(!pool1.contains(obj0));
    
	// Free the pointers when done
	pool0.free(obj0);
	pool1.free(obj1);
    
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