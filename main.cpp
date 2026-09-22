/**
 * @file main.cpp
 * @brief Modern C++ Entry Point for kindle-myts terminal emulator.
 *
 * Supports CLI argument parsing for runtime debug options (--debug, --overlay,
 * --log-level, --log-file, --metrics, --dry-run).
 */

#include "app/application.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

static void print_usage(const char* prog_name) {
    std::printf("Usage: %s [OPTIONS] [fb_device] [font_file]\n", prog_name);
    std::printf("Options:\n");
    std::printf("  --debug            Enable debug logging and diagnostics\n");
    std::printf("  --overlay          Render live on-screen telemetry banner\n");
    std::printf("  --metrics          Dump telemetry metrics summary on exit\n");
    std::printf("  --log-level LEVEL  Set log level (trace, debug, info, warn, error)\n");
    std::printf("  --log-file FILE    Redirect diagnostic logs to specified file\n");
    std::printf("  --dry-run          Initialize and render single frame without running loop\n");
    std::printf("  --help, -h         Display this help message\n");
}

int main(int argc, char** argv) {
    const char* fb_path = "/dev/fb0";
    const char* font_path = "ter-u12n.hex";
    myts::app::DebugConfig debug_cfg{};
    bool dry_run = false;

    int positional_idx = 0;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--debug") {
            debug_cfg.enable_debug = true;
            debug_cfg.level = myts::core::LogLevel::Debug;
        } else if (arg == "--overlay") {
            debug_cfg.enable_debug = true;
            debug_cfg.enable_overlay = true;
        } else if (arg == "--metrics") {
            debug_cfg.enable_metrics = true;
        } else if (arg == "--dry-run") {
            dry_run = true;
        } else if (arg == "--log-level" && i + 1 < argc) {
            std::string_view lvl(argv[++i]);
            if (lvl == "trace") debug_cfg.level = myts::core::LogLevel::Trace;
            else if (lvl == "debug") debug_cfg.level = myts::core::LogLevel::Debug;
            else if (lvl == "info") debug_cfg.level = myts::core::LogLevel::Info;
            else if (lvl == "warn") debug_cfg.level = myts::core::LogLevel::Warn;
            else if (lvl == "error") debug_cfg.level = myts::core::LogLevel::Error;
        } else if (arg == "--log-file" && i + 1 < argc) {
            debug_cfg.log_file = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg.rfind("--", 0) != 0) {
            if (positional_idx == 0) {
                fb_path = argv[i];
                positional_idx++;
            } else if (positional_idx == 1) {
                font_path = argv[i];
                positional_idx++;
            }
        }
    }

    std::printf("Starting kindle-myts (Modern C++ Stack)...\n");

    myts::app::Application app(fb_path, font_path, /*rows=*/24, /*cols=*/80);
    if (debug_cfg.enable_debug || debug_cfg.enable_metrics || debug_cfg.log_file != nullptr) {
        app.configure_debug(debug_cfg);
    }

    if (!app.init()) {
        std::fprintf(stderr, "Failed to initialize kindle-myts application.\n");
        return 1;
    }

    if (dry_run) {
        app.feed_terminal_output("kindle-myts dry-run test frame\r\n");
        app.render_frame();
        std::printf("Dry-run completed successfully.\n");
        return 0;
    }

    // Launch default interactive shell
    const char* shell = std::getenv("SHELL");
    if (shell == nullptr || shell[0] == '\0') {
        shell = "/bin/sh";
    }

    app.run(shell);

    if (debug_cfg.enable_metrics) {
        myts::core::StringBuilder sb;
        app.metrics().format_summary(sb);
        std::printf("Telemetry Summary: %s\n", sb.c_str());
    }

    std::printf("kindle-myts terminated cleanly.\n");
    return 0;
}
