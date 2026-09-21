#include "app/application.hpp"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    const char* fb_path = (argc > 1) ? argv[1] : "/dev/fb0";
    const char* font_path = (argc > 2) ? argv[2] : "ter-u12n.hex";

    std::printf("Starting kindle-myts (Modern C++ Stack)...\n");

    myts::app::Application app(fb_path, font_path, /*rows=*/24, /*cols=*/80);
    if (!app.init()) {
        std::fprintf(stderr, "Failed to initialize kindle-myts application.\n");
        return 1;
    }

    // Launch default interactive shell
    const char* shell = std::getenv("SHELL");
    if (shell == nullptr || shell[0] == '\0') {
        shell = "/bin/sh";
    }

    app.run(shell);

    std::printf("kindle-myts terminated cleanly.\n");
    return 0;
}
