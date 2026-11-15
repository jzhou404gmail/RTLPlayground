# Host unit-test harness for RTLPlayground firmware logic.
#
# Compiles selected firmware translation units with host gcc + AddressSanitizer,
# force-including sdcc_shim.h to erase 8051 keywords, and links them against
# mocked hardware edges (support.c). No SDCC, no flashing, no hardware.
#
#   make        build + run all tests
#   make run    run tests
#   make clean

CC       = gcc
SRCROOT  = ..
# -include shim first so __xdata/__code/__banked vanish before any firmware header.
CFLAGS   = -std=gnu11 -g -O1 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unknown-pragmas \
           -Wno-pointer-sign \
           -DRTLP_HOST_TEST \
           -fsanitize=address,undefined -fno-omit-frame-pointer \
           -include sdcc_shim.h -Istub -I. -I$(SRCROOT) -I$(SRCROOT)/httpd -I$(SRCROOT)/uip
ifeq ($(findstring clang,$(shell $(CC) --version 2>/dev/null)),)
CFLAGS  += -Wno-builtin-declaration-mismatch
endif
LDFLAGS  = -fsanitize=address,undefined

BUILDDIR = build

# One binary per subsystem under test.
TESTS = $(BUILDDIR)/test_cmd_editor \
        $(BUILDDIR)/test_port_tables \
        $(BUILDDIR)/test_page_json \
        $(BUILDDIR)/test_httpd_tx \
        $(BUILDDIR)/test_ntp \
        $(BUILDDIR)/test_tick_gate \
        $(BUILDDIR)/test_sessions \
        $(BUILDDIR)/test_auth

# The table tests link the register mock and one environment file; stub/
# stands in for the two headers a firmware build generates.
TABLE_ENV = support.c hw_mock.c env_tables.c $(SRCROOT)/cmd_editor.c \
            $(SRCROOT)/rtl837x_port.c $(SRCROOT)/httpd/page_impl.c $(SRCROOT)/httpd/html_out.c

all: run

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# cmd_editor.c is compiled UNMODIFIED from the firmware tree.
$(BUILDDIR)/test_cmd_editor: test_cmd_editor.c support.c $(SRCROOT)/cmd_editor.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# rtl837x_port.c and httpd/page_impl.c are compiled UNMODIFIED from the firmware tree.
$(BUILDDIR)/test_port_tables: test_port_tables.c $(TABLE_ENV) hw_mock.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@ $(LDFLAGS)

$(BUILDDIR)/test_tick_gate: test_tick_gate.c support.c $(SRCROOT)/cmd_editor.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# sessions.c is compiled UNMODIFIED from the firmware tree.
$(BUILDDIR)/test_sessions: test_sessions.c $(SRCROOT)/sessions.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# auth.c is compiled UNMODIFIED from the firmware tree; this test carries the
# flash sector and the console-side environment itself.
$(BUILDDIR)/test_auth: test_auth.c $(SRCROOT)/auth.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILDDIR)/test_page_json: test_page_json.c $(TABLE_ENV) hw_mock.h | $(BUILDDIR)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@ $(LDFLAGS)

# httpd/httpd.c, uip/uip.c, sessions.c, timer.c and auth.c are compiled
# UNMODIFIED from the firmware tree; this test carries the environment below
# them itself, including a simulated configuration sector for auth.c.
$(BUILDDIR)/test_httpd_tx: test_httpd_tx.c httpd_env.c $(SRCROOT)/httpd/httpd.c $(SRCROOT)/uip/uip.c \
                         $(SRCROOT)/sessions.c $(SRCROOT)/timer.c $(SRCROOT)/auth.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(SRCROOT) $(filter %.c,$^) -o $@ $(LDFLAGS)

# ntp.c and dns.c are compiled UNMODIFIED from the firmware tree; the uIP edge
# they use is mocked in the test, so uip.c itself is not linked.
$(BUILDDIR)/test_ntp: test_ntp.c support.c hw_mock.c $(SRCROOT)/cmd_editor.c \
                      $(SRCROOT)/ntp.c $(SRCROOT)/dns.c $(SRCROOT)/timer.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@ $(LDFLAGS)

run: $(TESTS)
	@fail=0; for t in $(TESTS); do \
		echo "=== $$t ==="; \
		$$t || fail=1; \
		echo; \
	done; \
	if command -v node >/dev/null 2>&1; then \
		echo "=== web auth ==="; \
		node test_web_auth.js || fail=1; \
		echo; \
	else \
		echo "node not found, skipping the browser password test"; \
	fi; \
	if [ $$fail -ne 0 ]; then echo "TESTS FAILED"; exit 1; else echo "ALL TESTS PASSED"; fi

# The browser-side password protocol needs Node, not the C harness.
web:
	@node test_web_auth.js

clean:
	rm -rf $(BUILDDIR)

.PHONY: all run web clean
