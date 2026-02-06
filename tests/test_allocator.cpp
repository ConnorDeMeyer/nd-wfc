#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include <span>
#include <cstring>
#include "nd-wfc/wfc_allocator.hpp"

namespace {

// Test fixture for WFCStackAllocator tests
class WFCStackAllocatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup if needed
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

// Test basic allocation and deallocation
TEST_F(WFCStackAllocatorTest, BasicAllocation) {
    WFC::WFCStackAllocator allocator(1024);

    void* ptr1 = allocator.allocate(64);
    ASSERT_NE(ptr1, nullptr);

    void* ptr2 = allocator.allocate(128);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr1, ptr2); // Should be different addresses

    // Check that allocations are properly aligned
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % 8, 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 8, 0);

    // deallocate doesn't do anything in this allocator
    allocator.deallocate(ptr1);
    allocator.deallocate(ptr2);
}

// Test alignment
TEST_F(WFCStackAllocatorTest, Alignment) {
    WFC::WFCStackAllocator allocator(1024);

    // Test various sizes and ensure 8-byte alignment
    for (size_t size : {1, 3, 7, 9, 15, 17}) {
        void* ptr = allocator.allocate(size);
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 8, 0);
    }
}

// Test stack frame functionality
TEST_F(WFCStackAllocatorTest, StackFrame) {
    WFC::WFCStackAllocator allocator(1024);

    // Allocate some memory in the root frame
    void* rootPtr = allocator.allocate(64);
    ASSERT_NE(rootPtr, nullptr);

    size_t initialCapacity = allocator.getCapacity();

    {
        // Create a new stack frame
        auto frame = allocator.createFrame();

        // Allocate in the new frame
        void* framePtr1 = allocator.allocate(32);
        void* framePtr2 = allocator.allocate(48);
        ASSERT_NE(framePtr1, nullptr);
        ASSERT_NE(framePtr2, nullptr);

        // Capacity should be reduced
        EXPECT_LT(allocator.getCapacity(), initialCapacity);

        // Frame goes out of scope, memory should be freed
    }

    // After frame destruction, capacity should be restored
    EXPECT_EQ(allocator.getCapacity(), initialCapacity);

    // We can still allocate (should reuse the freed space)
    void* newPtr = allocator.allocate(32);
    ASSERT_NE(newPtr, nullptr);
}

// Test nested stack frames
TEST_F(WFCStackAllocatorTest, NestedStackFrames) {
    WFC::WFCStackAllocator allocator(1024);

    void* rootPtr = allocator.allocate(32);
    size_t rootCapacity = allocator.getCapacity();

    {
        auto frame1 = allocator.createFrame();
        void* frame1Ptr = allocator.allocate(32);
        size_t frame1Capacity = allocator.getCapacity();

        {
            auto frame2 = allocator.createFrame();
            void* frame2Ptr = allocator.allocate(32);
            size_t frame2Capacity = allocator.getCapacity();

            // Each nested frame should have less capacity
            EXPECT_LT(frame2Capacity, frame1Capacity);
            EXPECT_LT(frame1Capacity, rootCapacity);

            // frame2 goes out of scope
        }

        // Back to frame1 capacity
        EXPECT_EQ(allocator.getCapacity(), frame1Capacity);

        // frame1 goes out of scope
    }

    // Back to root capacity
    EXPECT_EQ(allocator.getCapacity(), rootCapacity);
}

// Test automatic pool expansion
TEST_F(WFCStackAllocatorTest, PoolExpansion) {
    WFC::WFCStackAllocator allocator(128); // Small initial pool

    // Allocate until we exceed the initial pool
    std::vector<void*> allocations;
    size_t totalAllocated = 0;

    while (totalAllocated < 1000) { // More than initial capacity
        void* ptr = allocator.allocate(64);
        ASSERT_NE(ptr, nullptr);
        allocations.push_back(ptr);
        totalAllocated += 64;
    }

    // All allocations should be valid and aligned
    for (void* ptr : allocations) {
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 8, 0);
    }
}

// Test constructor with user-provided memory
TEST_F(WFCStackAllocatorTest, UserProvidedMemory) {
    const size_t bufferSize = 512;
    std::vector<uint8_t> buffer(bufferSize);

    std::span<uint8_t> span(buffer.data(), buffer.size());
    WFC::WFCStackAllocator allocator(span);

    // Should be able to allocate from the provided buffer
    void* ptr1 = allocator.allocate(64);
    ASSERT_NE(ptr1, nullptr);

    // Pointer should be within our buffer
    EXPECT_GE(ptr1, buffer.data());
    EXPECT_LT(static_cast<uint8_t*>(ptr1) + 64, buffer.data() + bufferSize);

    void* ptr2 = allocator.allocate(128);
    ASSERT_NE(ptr2, nullptr);

    // Should still be able to expand to new pools when user buffer is exhausted
    void* ptr3 = allocator.allocate(bufferSize); // Larger than user buffer
    ASSERT_NE(ptr3, nullptr);
}

// Test allocator adapter
TEST_F(WFCStackAllocatorTest, AllocatorAdapter) {
    WFC::WFCStackAllocator allocator(1024);
    WFC::WFCStackAllocatorAdapter<int> adapter(allocator);

    // Test allocation
    int* ptr = adapter.allocate(10); // Space for 10 ints
    ASSERT_NE(ptr, nullptr);

    // Should be properly aligned for int
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignof(int), 0);

    // Test deallocation
    adapter.deallocate(ptr, 10);
}

// Test STL container with custom allocator
TEST_F(WFCStackAllocatorTest, STLContainerWithAdapter) {
    WFC::WFCStackAllocator allocator(1024);
    using IntVector = std::vector<int, WFC::WFCStackAllocatorAdapter<int>>;

    {
        auto frame = allocator.createFrame();
        IntVector vec((WFC::WFCStackAllocatorAdapter<int>(allocator)));

        // Add some elements
        for (int i = 0; i < 10; ++i) {
            vec.push_back(i);
        }

        EXPECT_EQ(vec.size(), 10);
        for (int i = 0; i < 10; ++i) {
            EXPECT_EQ(vec[i], i);
        }

        // Vector goes out of scope, memory freed with frame
    }

    // Should be able to allocate again (reusing freed memory)
    void* newAlloc = allocator.allocate(64);
    ASSERT_NE(newAlloc, nullptr);
}

// Test alignment helper function
TEST_F(WFCStackAllocatorTest, AlignUp) {
    EXPECT_EQ(WFC::WFCStackAllocator::alignUp(0), 0);
    EXPECT_EQ(WFC::WFCStackAllocator::alignUp(1), 8);
    EXPECT_EQ(WFC::WFCStackAllocator::alignUp(7), 8);
    EXPECT_EQ(WFC::WFCStackAllocator::alignUp(8), 8);
    EXPECT_EQ(WFC::WFCStackAllocator::alignUp(9), 16);
    EXPECT_EQ(WFC::WFCStackAllocator::alignUp(15), 16);
    EXPECT_EQ(WFC::WFCStackAllocator::alignUp(16), 16);
}

// Test edge case: allocate zero bytes
TEST_F(WFCStackAllocatorTest, ZeroAllocation) {
    WFC::WFCStackAllocator allocator(1024);

    void* ptr = allocator.allocate(0);
    // Should return a valid pointer (can be null or non-null depending on implementation)
    // But should not crash

    allocator.deallocate(ptr);
}

// Test edge case: very large allocation
TEST_F(WFCStackAllocatorTest, LargeAllocation) {
    WFC::WFCStackAllocator allocator(1024);

    // Allocate something larger than initial capacity
    void* ptr = allocator.allocate(2000);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 8, 0);
}

// Test memory reuse after frame destruction
TEST_F(WFCStackAllocatorTest, MemoryReuse) {
    WFC::WFCStackAllocator allocator(1024);

    size_t initialCapacity = allocator.getCapacity();

    {
        auto frame = allocator.createFrame();
        allocator.allocate(64);
        allocator.allocate(64);

        // Should have less capacity during frame
        EXPECT_LT(allocator.getCapacity(), initialCapacity);
    }

    // After frame destruction, should be back to initial capacity
    EXPECT_EQ(allocator.getCapacity(), initialCapacity);
}

// Test multiple frames and complex nesting
TEST_F(WFCStackAllocatorTest, ComplexFrameNesting) {
    WFC::WFCStackAllocator allocator(1024);

    // Root allocations
    void* root1 = allocator.allocate(32);
    void* root2 = allocator.allocate(32);

    {
        auto frame1 = allocator.createFrame();
        void* f1_1 = allocator.allocate(32);
        void* f1_2 = allocator.allocate(32);

        {
            auto frame2 = allocator.createFrame();
            void* f2_1 = allocator.allocate(32);

            {
                auto frame3 = allocator.createFrame();
                void* f3_1 = allocator.allocate(32);
                // frame3 ends
            }

            void* f2_2 = allocator.allocate(32);
            // frame2 ends
        }

        void* f1_3 = allocator.allocate(32);
        // frame1 ends
    }

    // All frame memory should be freed, root allocations still valid
    void* root3 = allocator.allocate(32); // Should reuse freed space
    ASSERT_NE(root3, nullptr);
}

// Test that deallocation is a no-op
TEST_F(WFCStackAllocatorTest, DeallocateIsNoOp) {
    WFC::WFCStackAllocator allocator(1024);

    void* ptr = allocator.allocate(64);
    size_t capacityBefore = allocator.getCapacity();

    // deallocate should do nothing
    allocator.deallocate(ptr);

    // Capacity should be unchanged
    EXPECT_EQ(allocator.getCapacity(), capacityBefore);

    // Should be able to allocate again
    void* ptr2 = allocator.allocate(64);
    ASSERT_NE(ptr2, nullptr);
}

} // namespace
