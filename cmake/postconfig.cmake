
find_path(PHMAP_HEADERS "parallel_hashmap/phmap.h")
if(${PHMAP_HEADERS} STREQUAL "PHMAP_HEADERS-NOTFOUND")
	message(FATAL_ERROR "Could not locate the headers for the parallel_hashmap library!")
endif()

set_property(TARGET ez::pool APPEND
	PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PHMAP_HEADERS}"
)