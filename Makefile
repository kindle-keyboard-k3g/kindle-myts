# Compile this on an old armv6 raspberry pi or cross-compiler.

CC ?= gcc
CXX ?= g++
STRIP ?= strip

# C & C++ Standard and Embedded Optimizations
CFLAGS ?= -Os -Wall -Wextra
CXXFLAGS ?= -std=c++17 -Os -Wall -Wextra -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables

# Debug compiler flags
CFLAGS_DEBUG = -Wall -Wextra -g3 -O0 -I. -DDEBUG -UNDEBUG
CXXFLAGS_DEBUG = -std=c++17 -Wall -Wextra -g3 -O0 -I. -DDEBUG -UNDEBUG -fno-exceptions -fno-rtti

HOST_CFLAGS = -Wall -Wextra -g3 -O0 -I. -DNODEBUG
HOST_CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -g3 -O0 -I. -DNODEBUG -fno-exceptions -fno-rtti

TEST_CFLAGS = $(HOST_CFLAGS)
TEST_CXXFLAGS = $(HOST_CXXFLAGS)
ASAN_CFLAGS = $(HOST_CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer
ASAN_CXXFLAGS = $(HOST_CXXFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer

# files to publish
PUB= $(HEADERS) $(ALLSRCS) Makefile README.md myts myts-ng myts.ini keydefs.ini myts.l.ini

HEADERS = config.h dynstring.h font.h myts.h pixop.h screen.h terminal.h
HEADERS += linux/
ALLSRCS= myts.c terminal.c dynstring.c
ALLSRCS += config.c launchpad.c
ALLSRCS += screen.c pixop.c font.c
SRCS= $(ALLSRCS)
CFLAGS += -I.

CFLAGS += -DNODEBUG

OBJS := $(strip $(patsubst %.c,%.o,$(strip $(SRCS))))

myts: $(OBJS)
	$(CC) $(CFLAGS) -o myts $(OBJS) $(LDFLAGS)
	$(STRIP) $@

myts-dbg: $(SRCS)
	$(CC) $(CFLAGS_DEBUG) -o $@ $^ $(LDFLAGS)

$(OBJS): myts.h
terminal.o: terminal.h

tgz: $(PUB)
	tar cvzf /tmp/kiterm.tgz --exclude .svn $(PUB)

myts.zip: myts myts-ng myts.l.ini
	rm -rf myts.zip myts-bundle
	mkdir -p myts-bundle/myts myts-bundle/launchpad
	cp myts.l.ini myts-bundle/launchpad/myts.ini
	cp myts.ini *.hex README.md LICENSE keydefs.ini bdf2hex tools/launch_kindle.sh tools/matrix-anim.sh myts-bundle/myts/
	cp tools/myts myts-bundle/myts/myts
	@if [ -x "$(KINDLE_MUSL_CXX)" ]; then \
		echo "Building ARMv6 Kindle binaries for distribution..."; \
		$(MAKE) myts-ng-kindle tools/matrix-kindle; \
		cp myts-ng-kindle myts-bundle/myts/myts-ng; \
		cp tools/matrix-kindle myts-bundle/myts/matrix; \
	else \
		cp myts-ng myts-bundle/myts/myts-ng; \
		if [ -x tools/matrix ]; then cp tools/matrix myts-bundle/myts/matrix; fi; \
	fi
	cp myts myts-bundle/myts/myts-legacy
	(cd myts-bundle && zip -r ../myts.zip launchpad myts)
	rm -rf myts-bundle

clean:
	rm -rf *lll myts myts-ng myts-dbg myts-ng-dbg *.o *.core *.table myts.zip tools/matrix tools/matrix-kindle tests/test_dynstring tests/test_config tests/test_pixop tests/test_raii tests/test_buffers tests/test_pixmap tests/test_modern_config tests/test_ansi tests/test_event_loop tests/test_eink_display tests/test_font_renderer tests/test_terminal_session tests/test_input_manager tests/test_application tests/test_logger tests/test_metrics tests/test_debug_overlay tests/*.o

TEST_BINS = tests/test_dynstring tests/test_config tests/test_pixop tests/test_raii tests/test_buffers tests/test_pixmap tests/test_modern_config tests/test_ansi tests/test_event_loop tests/test_eink_display tests/test_font_renderer tests/test_terminal_session tests/test_input_manager tests/test_application tests/test_logger tests/test_metrics tests/test_debug_overlay

# Kindle ARM32 Toolchain Configuration
KINDLE_MUSL_CC ?= $(HOME)/.local/toolchains/armv6-linux-musleabi-cross/bin/armv6-linux-musleabi-gcc
KINDLE_MUSL_CXX ?= $(HOME)/.local/toolchains/armv6-linux-musleabi-cross/bin/armv6-linux-musleabi-g++
KINDLE_MUSL_STRIP ?= $(HOME)/.local/toolchains/armv6-linux-musleabi-cross/bin/armv6-linux-musleabi-strip
KINDLE_CFLAGS ?= -Os -Wall -Wextra -march=armv6j -mtune=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp -static
KINDLE_CXXFLAGS ?= -std=c++17 -Os -Wall -Wextra -march=armv6j -mtune=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp -fno-exceptions -fno-rtti -static
KINDLE_CXXFLAGS_DEBUG ?= -std=c++17 -Wall -Wextra -g3 -O0 -march=armv6j -mtune=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp -DDEBUG -UNDEBUG -fno-exceptions -fno-rtti -static

myts-ng: main.cpp
	$(CXX) $(CXXFLAGS) -I. -o $@ $^

myts-ng-dbg: main.cpp
	$(CXX) $(CXXFLAGS_DEBUG) -I. -o $@ $^

myts-ng-kindle: main.cpp
	$(KINDLE_MUSL_CXX) $(KINDLE_CXXFLAGS) -I. -o $@ $^
	@if [ -x "$(KINDLE_MUSL_STRIP)" ]; then $(KINDLE_MUSL_STRIP) $@; fi

myts-ng-kindle-dbg: main.cpp
	$(KINDLE_MUSL_CXX) $(KINDLE_CXXFLAGS_DEBUG) -I. -o $@ $^

tools/matrix: tools/matrix.c
	$(CC) $(CFLAGS) -o $@ $^

tools/matrix-kindle: tools/matrix.c
	$(KINDLE_MUSL_CC) $(KINDLE_CFLAGS) -o $@ $^
	@if [ -x "$(KINDLE_MUSL_STRIP)" ]; then $(KINDLE_MUSL_STRIP) $@; fi


tests/test_dynstring: tests/test_dynstring.c dynstring.c
	$(CC) $(TEST_CFLAGS) -o $@ $^

tests/test_config: tests/test_config.c config.c
	$(CC) $(TEST_CFLAGS) -o $@ $^

tests/test_pixop: tests/test_pixop.c pixop.c
	$(CC) $(TEST_CFLAGS) -o $@ $^

tests/test_raii: tests/test_raii.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_buffers: tests/test_buffers.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_pixmap: tests/test_pixmap.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_modern_config: tests/test_modern_config.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_ansi: tests/test_ansi.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_event_loop: tests/test_event_loop.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_eink_display: tests/test_eink_display.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_font_renderer: tests/test_font_renderer.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_terminal_session: tests/test_terminal_session.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_input_manager: tests/test_input_manager.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_application: tests/test_application.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_logger: tests/test_logger.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_metrics: tests/test_metrics.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

tests/test_debug_overlay: tests/test_debug_overlay.cpp
	$(CXX) $(TEST_CXXFLAGS) -o $@ $^

test: $(TEST_BINS)
	@echo "Running complete test suite..."
	@tests/test_dynstring
	@tests/test_config
	@tests/test_pixop
	@tests/test_raii
	@tests/test_buffers
	@tests/test_pixmap
	@tests/test_modern_config
	@tests/test_ansi
	@tests/test_event_loop
	@tests/test_eink_display
	@tests/test_font_renderer
	@tests/test_terminal_session
	@tests/test_input_manager
	@tests/test_application
	@tests/test_logger
	@tests/test_metrics
	@tests/test_debug_overlay
	@echo "All tests passed successfully!"

test-asan:
	@echo "Building with AddressSanitizer..."
	$(CC) $(ASAN_CFLAGS) -o tests/test_dynstring tests/test_dynstring.c dynstring.c
	$(CC) $(ASAN_CFLAGS) -o tests/test_config tests/test_config.c config.c
	$(CC) $(ASAN_CFLAGS) -o tests/test_pixop tests/test_pixop.c pixop.c
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_raii tests/test_raii.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_buffers tests/test_buffers.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_pixmap tests/test_pixmap.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_modern_config tests/test_modern_config.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_ansi tests/test_ansi.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_event_loop tests/test_event_loop.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_eink_display tests/test_eink_display.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_font_renderer tests/test_font_renderer.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_terminal_session tests/test_terminal_session.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_input_manager tests/test_input_manager.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_application tests/test_application.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_logger tests/test_logger.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_metrics tests/test_metrics.cpp
	$(CXX) $(ASAN_CXXFLAGS) -o tests/test_debug_overlay tests/test_debug_overlay.cpp
	@echo "Running test suite under AddressSanitizer..."
	@tests/test_dynstring
	@tests/test_config
	@tests/test_pixop
	@tests/test_raii
	@tests/test_buffers
	@tests/test_pixmap
	@tests/test_modern_config
	@tests/test_ansi
	@tests/test_event_loop
	@tests/test_eink_display
	@tests/test_font_renderer
	@tests/test_terminal_session
	@tests/test_input_manager
	@tests/test_application
	@tests/test_logger
	@tests/test_metrics
	@tests/test_debug_overlay
	@echo "All sanitizer tests passed!"

# conversion
# hexdump -e '"\n\t" 8/1 "%3d, "'
# DO NOT DELETE

%.table: codepage.sh
	./codepage.sh $*
