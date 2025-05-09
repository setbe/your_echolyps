#include <stdint.h>
#include <sys/mman.h>

namespace hi {

void *alloc(size_t size) noexcept {
    return mmap(nullptr, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void free(void *ptr, size_t size) noexcept { munmap(ptr, size); }

} // namespace hi
