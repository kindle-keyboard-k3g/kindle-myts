#include "tests/test_framework.h"
#include "core/raii.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

using namespace myts::core;

int test_unique_fd_default_and_reset(void)
{
    UniqueFd fd;
    ASSERT_FALSE(fd.is_valid());
    ASSERT_EQ(fd.get(), -1);

    int pipe_fds[2];
    ASSERT_EQ(pipe(pipe_fds), 0);

    fd.reset(pipe_fds[0]);
    ASSERT_TRUE(fd.is_valid());
    ASSERT_EQ(fd.get(), pipe_fds[0]);

    // Resetting closes old fd
    fd.reset(pipe_fds[1]);
    ASSERT_TRUE(fd.is_valid());
    ASSERT_EQ(fd.get(), pipe_fds[1]);

    // Check old fd is closed
    char buf[1];
    ASSERT_EQ(read(pipe_fds[0], buf, 1), -1);

    fd.reset();
    ASSERT_FALSE(fd.is_valid());
    ASSERT_EQ(read(pipe_fds[1], buf, 1), -1);

    return 0;
}

int test_unique_fd_move_semantics(void)
{
    int pipe_fds[2];
    ASSERT_EQ(pipe(pipe_fds), 0);

    UniqueFd fd1(pipe_fds[0]);
    ASSERT_TRUE(fd1.is_valid());

    UniqueFd fd2 = std::move(fd1);
    ASSERT_FALSE(fd1.is_valid());
    ASSERT_TRUE(fd2.is_valid());
    ASSERT_EQ(fd2.get(), pipe_fds[0]);

    int raw_fd = fd2.release();
    ASSERT_FALSE(fd2.is_valid());
    ASSERT_EQ(raw_fd, pipe_fds[0]);

    close(raw_fd);
    close(pipe_fds[1]);
    return 0;
}

int test_memory_mapping_anonymous(void)
{
    MemoryMapping mapping;
    ASSERT_FALSE(mapping.is_valid());
    ASSERT_NULL(mapping.data());
    ASSERT_EQ(mapping.size(), 0);

    const size_t map_size = 4096;
    bool ok = mapping.map(-1, map_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(mapping.is_valid());
    ASSERT_NOT_NULL(mapping.data());
    ASSERT_EQ(mapping.size(), map_size);

    // Test memory can be written and read
    uint8_t *ptr = mapping.data();
    ptr[0] = 0xAA;
    ptr[map_size - 1] = 0x55;
    ASSERT_EQ(ptr[0], 0xAA);
    ASSERT_EQ(ptr[map_size - 1], 0x55);

    mapping.unmap();
    ASSERT_FALSE(mapping.is_valid());
    ASSERT_NULL(mapping.data());
    ASSERT_EQ(mapping.size(), 0);

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_unique_fd_default_and_reset);
    RUN_TEST(test_unique_fd_move_semantics);
    RUN_TEST(test_memory_mapping_anonymous);
TEST_MAIN_END()
