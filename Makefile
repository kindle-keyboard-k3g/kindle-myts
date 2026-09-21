# Compile this on an old armv6 raspberry pi or cross-compiler.

CC ?= gcc
STRIP ?= strip
CFLAGS ?= -Os -Wall -Wextra
HOST_CFLAGS = -Wall -Wextra -g3 -O0 -I. -DNODEBUG
TEST_CFLAGS = $(HOST_CFLAGS)
ASAN_CFLAGS = $(HOST_CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer

# files to publish
PUB= $(HEADERS) $(ALLSRCS) Makefile README myts myts.ini keydefs.ini $(TABLES)

CODEPAGES = CP437 CP1255
TABLES = $(patsubst %,%.table,$(CODEPAGES))

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

$(OBJS): myts.h
terminal.o: terminal.h

tgz: $(PUB)
	tar cvzf /tmp/kiterm.tgz --exclude .svn $(PUB)

myts.zip: $(PUB)
	rm -f myts.zip
	mkdir -p myts
	mkdir -p launchpad
	cp myts.l.ini launchpad/
	cp profile myts.sh myts.ini *.hex *.table README keymap keydefs.ini bdf2hex about.txt myts/
	cp myts myts/myts
	zip -r myts.zip launchpad myts
	rm -r myts/ launchpad/

clean:
	rm -rf *lll myts *.o *.core *.table myts.zip tests/test_dynstring tests/test_config tests/test_pixop tests/*.o

TEST_BINS = tests/test_dynstring tests/test_config tests/test_pixop

tests/test_dynstring: tests/test_dynstring.c dynstring.c
	$(CC) $(TEST_CFLAGS) -o $@ $^

tests/test_config: tests/test_config.c config.c
	$(CC) $(TEST_CFLAGS) -o $@ $^

tests/test_pixop: tests/test_pixop.c pixop.c
	$(CC) $(TEST_CFLAGS) -o $@ $^

test: $(TEST_BINS)
	@echo "Running test suite..."
	@tests/test_dynstring
	@tests/test_config
	@tests/test_pixop
	@echo "All tests passed successfully!"

test-asan:
	@echo "Building with AddressSanitizer..."
	$(CC) $(ASAN_CFLAGS) -o tests/test_dynstring tests/test_dynstring.c dynstring.c
	$(CC) $(ASAN_CFLAGS) -o tests/test_config tests/test_config.c config.c
	$(CC) $(ASAN_CFLAGS) -o tests/test_pixop tests/test_pixop.c pixop.c
	@echo "Running test suite under AddressSanitizer..."
	@tests/test_dynstring
	@tests/test_config
	@tests/test_pixop
	@echo "All sanitizer tests passed!"

# conversion
# hexdump -e '"\n\t" 8/1 "%3d, "'
# DO NOT DELETE

%.table: codepage.sh 
	./codepage.sh $*
