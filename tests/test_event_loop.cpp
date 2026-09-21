#include "tests/test_framework.h"
#include "core/event_loop.hpp"
#include <unistd.h>
#include <sys/socket.h>

using namespace myts::core;

int test_event_loop_fd_read_event(void)
{
    EventLoop loop;
    int fds[2];
    ASSERT_EQ(pipe(fds), 0);

    bool received = false;
    char buffer[16] = {0};

    // Register reader callback
    bool reg_ok = loop.register_read(fds[0], [&](int fd) {
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            received = true;
        }
    });
    ASSERT_TRUE(reg_ok);

    // Write byte into pipe
    ASSERT_EQ(write(fds[1], "X", 1), 1);

    // Run single step of event loop with 50ms timeout
    bool step_res = loop.step(50);
    ASSERT_TRUE(step_res);
    ASSERT_TRUE(received);
    ASSERT_EQ(buffer[0], 'X');

    // Clean up
    loop.unregister(fds[0]);
    close(fds[0]);
    close(fds[1]);

    return 0;
}

int test_event_loop_timer_event(void)
{
    EventLoop loop;
    bool timer_fired = false;

    // Register one-shot 10ms timer
    uint32_t timer_id = loop.set_timer(10, [&]() {
        timer_fired = true;
    });
    ASSERT_TRUE(timer_id > 0);

    // Run step before timer expires (0 timeout)
    loop.step(0);
    // Might not have expired yet depending on precision, sleep 15ms and step again
    usleep(15000);
    loop.step(10);

    ASSERT_TRUE(timer_fired);

    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_event_loop_fd_read_event);
    RUN_TEST(test_event_loop_timer_event);
TEST_MAIN_END()
