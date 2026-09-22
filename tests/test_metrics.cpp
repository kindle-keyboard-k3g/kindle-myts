/**
 * @file test_metrics.cpp
 * @brief Unit tests for performance and telemetry metrics collector (core/metrics.hpp)
 */

#include "test_framework.h"
#include "core/metrics.hpp"
#include "core/string_builder.hpp"
#include "graphics/geometry.hpp"
#include <string_view>

using namespace myts::core;
using namespace myts::graphics;

static int test_metrics_initial_state() {
    MetricsCollector metrics;
    MetricsSnapshot snap = metrics.snapshot();

    ASSERT_EQ(snap.partial_refreshes, 0);
    ASSERT_EQ(snap.full_refreshes, 0);
    ASSERT_EQ(snap.pty_bytes_read, 0);
    ASSERT_EQ(snap.pty_bytes_written, 0);
    ASSERT_EQ(snap.input_events, 0);
    ASSERT_EQ(snap.dirty_bounding_box.width, 0);
    ASSERT_EQ(snap.dirty_bounding_box.height, 0);
    return 0;
}

static int test_metrics_record_refreshes_and_dirty_box() {
    MetricsCollector metrics;

    // Record partial refresh
    metrics.record_refresh(Rect{10, 20, 100, 50}, false);
    MetricsSnapshot snap1 = metrics.snapshot();
    ASSERT_EQ(snap1.partial_refreshes, 1);
    ASSERT_EQ(snap1.full_refreshes, 0);
    ASSERT_EQ(snap1.dirty_bounding_box.x, 10);
    ASSERT_EQ(snap1.dirty_bounding_box.y, 20);
    ASSERT_EQ(snap1.dirty_bounding_box.width, 100);
    ASSERT_EQ(snap1.dirty_bounding_box.height, 50);

    // Record second partial refresh that expands bounding box
    metrics.record_refresh(Rect{5, 10, 50, 80}, false);
    MetricsSnapshot snap2 = metrics.snapshot();
    ASSERT_EQ(snap2.partial_refreshes, 2);
    ASSERT_EQ(snap2.dirty_bounding_box.x, 5);
    ASSERT_EQ(snap2.dirty_bounding_box.y, 10);
    // Bounding box union: x: 5 to 110 (w=105), y: 10 to 90 (h=80)
    ASSERT_EQ(snap2.dirty_bounding_box.width, 105);
    ASSERT_EQ(snap2.dirty_bounding_box.height, 80);

    // Record full refresh
    metrics.record_refresh(Rect{0, 0, 600, 800}, true);
    MetricsSnapshot snap3 = metrics.snapshot();
    ASSERT_EQ(snap3.partial_refreshes, 2);
    ASSERT_EQ(snap3.full_refreshes, 1);
    ASSERT_EQ(snap3.dirty_bounding_box.x, 0);
    ASSERT_EQ(snap3.dirty_bounding_box.y, 0);
    ASSERT_EQ(snap3.dirty_bounding_box.width, 600);
    ASSERT_EQ(snap3.dirty_bounding_box.height, 800);
    return 0;
}

static int test_metrics_record_io_and_input() {
    MetricsCollector metrics;

    metrics.record_pty_read(1024);
    metrics.record_pty_read(512);
    metrics.record_pty_write(256);
    metrics.record_input_event();
    metrics.record_input_event();
    metrics.record_input_event();

    MetricsSnapshot snap = metrics.snapshot();
    ASSERT_EQ(snap.pty_bytes_read, 1536);
    ASSERT_EQ(snap.pty_bytes_written, 256);
    ASSERT_EQ(snap.input_events, 3);
    return 0;
}

static int test_metrics_format_summary() {
    MetricsCollector metrics;
    metrics.record_refresh(Rect{0, 0, 100, 200}, false);
    metrics.record_refresh(Rect{0, 0, 600, 800}, true);
    metrics.record_pty_read(4096);
    metrics.record_pty_write(128);
    metrics.record_input_event();

    StringBuilder sb;
    metrics.format_summary(sb);

    std::string_view out = sb.view();
    ASSERT_TRUE(out.find("Refreshes: 1 full, 1 partial") != std::string_view::npos);
    ASSERT_TRUE(out.find("PTY: 4096 in, 128 out") != std::string_view::npos);
    ASSERT_TRUE(out.find("Inputs: 1") != std::string_view::npos);
    ASSERT_TRUE(out.find("DirtyBox: 600x800@(0,0)") != std::string_view::npos);
    return 0;
}

TEST_MAIN_BEGIN()
    RUN_TEST(test_metrics_initial_state);
    RUN_TEST(test_metrics_record_refreshes_and_dirty_box);
    RUN_TEST(test_metrics_record_io_and_input);
    RUN_TEST(test_metrics_format_summary);
TEST_MAIN_END()
