// Memory management implementation stub
module;

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#if defined(_MSC_VER)
#include <malloc.h>
#define ALIGNED_ALLOC(alignment, size) _aligned_malloc(size, alignment)
#define ALIGNED_FREE(ptr) _aligned_free(ptr)
#elif defined(__APPLE__) || defined(__linux__)
static inline void* ALIGNED_ALLOC(size_t alignment, size_t size) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) return nullptr;
    return ptr;
}
#define ALIGNED_FREE(ptr) free(ptr)
#else
#define ALIGNED_ALLOC(alignment, size) std::aligned_alloc(alignment, size)
#define ALIGNED_FREE(ptr) std::free(ptr)
#endif

module engine.platform;

namespace engine::platform::memory {

    // LinearAllocator implementation
    LinearAllocator::LinearAllocator(size_t capacity)
        : buffer_(static_cast<uint8_t*>(ALIGNED_ALLOC(default_alignment, capacity)))
        , capacity_(capacity)
        , offset_(0)
        , peak_offset_(0) {
    }

    LinearAllocator::~LinearAllocator() {
        if (buffer_) {
            std::free(buffer_);
        }
    }

    inline size_t align_size(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    void* LinearAllocator::Allocate(size_t size, size_t alignment) {
        size_t aligned_offset = align_size(offset_, alignment);

        if (aligned_offset + size > capacity_) {
            return nullptr; // Out of memory
        }

        void* ptr = buffer_ + aligned_offset;
        offset_ = aligned_offset + size;
        peak_offset_ = std::max(peak_offset_, offset_);

        return ptr;
    }

    void LinearAllocator::Deallocate(void* ptr) {
        // Linear allocator doesn't support individual deallocation
    }

    void LinearAllocator::Reset() {
        offset_ = 0;
    }

    size_t LinearAllocator::AllocatedSize() const {
        return offset_;
    }

    size_t LinearAllocator::PeakSize() const {
        return peak_offset_;
    }

    size_t LinearAllocator::Remaining() const {
        return capacity_ - offset_;
    }

    // PoolAllocator implementation
    PoolAllocator::PoolAllocator(size_t block_size, size_t block_count)
        : block_size_(AlignSize(block_size))
        , block_count_(block_count)
        , allocated_blocks_(0)
        , peak_allocated_(0) {

        size_t total_size = block_size_ * block_count_;
        buffer_ = static_cast<uint8_t*>(ALIGNED_ALLOC(default_alignment, total_size));

        // Initialize free list
        free_list_ = reinterpret_cast<FreeBlock*>(buffer_);
        FreeBlock* current = free_list_;

        for (size_t i = 0; i < block_count_ - 1; ++i) {
            current->next = reinterpret_cast<FreeBlock*>(buffer_ + (i + 1) * block_size_);
            current = current->next;
        }
        current->next = nullptr;
    }

    PoolAllocator::~PoolAllocator() {
        if (buffer_) {
            std::free(buffer_);
        }
    }

    void* PoolAllocator::Allocate(size_t size, size_t alignment) {
        if (size > block_size_ || !free_list_) {
            return nullptr;
        }

        FreeBlock* block = free_list_;
        free_list_ = free_list_->next;

        allocated_blocks_++;
        peak_allocated_ = std::max(peak_allocated_, allocated_blocks_);

        return block;
    }

    void PoolAllocator::Deallocate(void* ptr) {
        if (!ptr) return;

        FreeBlock* block = static_cast<FreeBlock*>(ptr);
        block->next = free_list_;
        free_list_ = block;

        allocated_blocks_--;
    }

    size_t PoolAllocator::AllocatedSize() const {
        return allocated_blocks_ * block_size_;
    }

    size_t PoolAllocator::PeakSize() const {
        return peak_allocated_ * block_size_;
    }

    size_t PoolAllocator::AvailableBlocks() const {
        return block_count_ - allocated_blocks_;
    }

    // Global allocator stubs
    static LinearAllocator* g_frame_allocator = nullptr;

    Allocator& GetDefaultAllocator() {
        static PoolAllocator default_alloc(1024, 1000); // 1KB blocks, 1000 of them
        return default_alloc;
    }

    LinearAllocator& GetFrameAllocator() {
        if (!g_frame_allocator) {
            g_frame_allocator = new LinearAllocator(1024 * 1024); // 1MB frame allocator
        }
        return *g_frame_allocator;
    }

} // namespace engine::platform::memory
