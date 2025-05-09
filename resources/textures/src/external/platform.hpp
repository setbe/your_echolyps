#pragma once

#include <stddef.h>

namespace hi {
// Allocate OS memory, use `hi::free(ptr, size)`
void *alloc(size_t size) noexcept;

// Release OS memory back
void free(void *ptr, size_t size_in_bytes) noexcept;
} // namespace hi