VERSION=0.1.0
IMAGESIZE = 524288
DEFAULT_CONFIG_LOCATION = 454656
CONFIG_LOCATION = 458752
HTML_LOCATION = 262144

ifeq ($(origin CC),default)
CC = sdcc
endif
CC_FLAGS = -mmcs51 -I. -Ihttpd -Iuip
ASM ?= sdas8051
AFLAGS= -plosgff

SUBDIRS := tools
SUBDIRSCLEAN=$(addsuffix clean,$(SUBDIRS))

# Machine selection: either pass MACHINE=<target> on the command line, or
# uncomment exactly one "#define MACHINE_<target>" line in machine.h. The
# latter needs no -D flag because machine.h itself then defines the macro.
MACHINE_TARGETS := $(shell grep -E '^[[:space:]]*(//[[:space:]]*)?#define MACHINE_' machine.h | sed -E 's%^[[:space:]]*(//[[:space:]]*)?#define MACHINE_%%' | awk '{print $$1}' | sort -u)

ifeq ($(MACHINE),)
	MACHINE:= $(shell grep "^\s*#define MACHINE_" machine.h | sed "s/^\s*#define MACHINE_//")
else
	CC_FLAGS += -DMACHINE_$(MACHINE)
endif

# Goals that make sense without a machine selection.
MACHINE_FREE_GOALS = clean distclean tools tools/clean machine_check
# A bare "make" has no goals but builds $(all), which does need a machine.
MACHINE_IS_NEEDED = $(if $(MAKECMDGOALS),$(filter-out $(MACHINE_FREE_GOALS),$(MAKECMDGOALS)),all)

# Fail immediately and legibly instead of generating "output//..." paths and
# tripping the #error in machine.c.
ifeq ($(MACHINE),)
ifneq ($(MACHINE_IS_NEEDED),)
# $(shell) folds newlines into spaces, so expand the list one name per line.
empty :=
space := $(empty) $(empty)
define newline


endef
MACHINE_TARGETS_LIST := $(subst $(space),$(newline),$(MACHINE_TARGETS))
define NO_MACHINE_HELP

No machine type selected.

Pick your target on the command line, for example:
    make MACHINE=DEFAULT_8C_1SFP

or uncomment exactly one "#define MACHINE_<target>" line in machine.h
(see doc/devices/ for the target matching your hardware).

Available machine targets:
$(MACHINE_TARGETS_LIST)
endef
$(error $(NO_MACHINE_HELP))
endif
endif
# Health instrumentation and the "health" console command: HEALTH=1 gmake ...
ifneq ($(HEALTH),)
	CC_FLAGS += -DHEALTH
endif

# Some SPI NOR chips (e.g. Boya/BoHong BY25Q16BS) do not work with the flash
# controller's Dual-I/O MMIO read mode: fetching code from a bank then returns
# garbage and the firmware resets in a loop. FLASH_NO_DIO=1 selects single-bit
# fast read instead. See doc/devices/PCB-K0402W-V1.0.md.
ifneq ($(FLASH_NO_DIO),)
	CC_FLAGS += -DFLASH_NO_DIO
endif

ifeq ($(CI),1)
	CC_FLAGS += --Werror
endif

# Factory default startup configuration: the embedded config.txt holds the IP,
# gateway and netmask a freshly flashed board comes up with, plus - if it has
# such a line - a "passwd" line that replaces the built-in default password.
# Targets whose vendor default differs get their own file.
XIKESTOR_TARGETS := SKS3200M_4GPY2XF SKS3200M_5GPY1XF SKS3200M_8GPY1XF
DEFAULT_CONFIG := config.txt
ifneq ($(filter $(XIKESTOR_TARGETS),$(MACHINE)),)
DEFAULT_CONFIG := config-xikestor.txt
endif

BUILDDIR = output/$(MACHINE)
VERSION_HEADER := version.h

GIT_VERSION := $(shell git rev-parse --short HEAD)
ifeq ($(shell git status --porcelain --untracked-files=no),)
else
	GIT_VERSION := $(GIT_VERSION)-dirty
endif

VERSION_EXTENSION = v$(VERSION)-$(GIT_VERSION)
FILENAME_EXTENSION = $(VERSION_EXTENSION)-$(MACHINE)

# Deterministic build date: honor SOURCE_DATE_EPOCH, else the HEAD commit date,
# else wall-clock (no-git fallback). Keeps same-commit builds byte-identical
# (BUILD_DATE is baked into the image and covered by the trailing CRC).
#
# It is the commit time, not the wall clock of the build, so a rebuild of the
# same sources produces the same image. It is rendered in the build host's time
# zone, so the System tab shows a date the operator reads in their own clock -
# build in one zone (or set TZ) if two hosts have to produce identical bytes.
SOURCE_DATE_EPOCH ?= $(shell git show -s --format=%ct HEAD 2>/dev/null)
ifeq ($(SOURCE_DATE_EPOCH),)
BUILD_DATE := $(shell date +"%Y-%m-%d %H:%M:%S")
else
BUILD_DATE := $(shell date -d @$(SOURCE_DATE_EPOCH) +"%Y-%m-%d %H:%M:%S" 2>/dev/null \
	|| date -r $(SOURCE_DATE_EPOCH) +"%Y-%m-%d %H:%M:%S")
endif

all: create_build_dir $(VERSION_HEADER) $(SUBDIRS) $(BUILDDIR)/rtlplayground-$(FILENAME_EXTENSION).bin

create_build_dir:
	mkdir -p "$(BUILDDIR)"
	mkdir -p "$(BUILDDIR)/uip"
	mkdir -p "$(BUILDDIR)/httpd"

# Keep machine.c in first position to fail immediately on invalid $MACHINE value
SRCS = \
	machine.c \
	machine_init.c \
	cmd_editor.c \
	cmd_parser.c \
	dhcp.c \
	html_data.c \
	rtlplayground.c \
	boot.c \
	sfp.c \
	dns.c \
	dns_cli.c \
	ntp.c \
	ntp_cli.c \
	syslog.c \
	udp_apps.c

# RTL837x
SRCS += \
	rtl837x_bandwidth.c \
	rtl837x_flash.c \
	rtl837x_igmp.c \
	rtl837x_init.c \
	rtl837x_leds.c \
	rtl837x_leds_dump.c \
	rtl837x_phy.c \
	rtl837x_pins.c\
	rtl837x_port.c \
	rtl837x_stp.c \
	rtl837x_lldp.c \
	rtl837x_storm.c \
	rtl837x_trap.c
SRCS += \
	httpd/httpd.c \
	httpd/page_impl.c
SRCS += \
	uip/timer.c \
	uip/uip.c \
	uip/uiplib.c \
	uip/uip_arp.c \
	uip/uip-fw.c \
	uip/uip-neighbor.c \
	uip/uip-split.c

OBJS = ${SRCS:%.c=$(BUILDDIR)/%.rel}
DEPS := ${SRCS:%.c=$(BUILDDIR)/%.d}
HTML := $(shell find html -name '*.js' -or -name '*.html' -or -name '*.svg' -or -name '*.css' -or -name '*.ico')

# Minified copy of the web UI sources, used as the fileadder input.
# The raw html/ sources stay untouched for development; the minified
# copy is a build artifact under output/.
HTML_MIN := output/html_min
.PHONY: html_min
html_min: $(HTML)
	rm -rf $(HTML_MIN)
	mkdir -p $(HTML_MIN)
	@for f in $(HTML); do python3 tools/minify.py $$f $(HTML_MIN)/$$(basename $$f) || exit 1; done

html_data.c html_data.h &: $(HTML) | tools html_min
	tools/output/fileadder -a $(HTML_LOCATION) -s $(IMAGESIZE) -b BANK1 -z -d $(HTML_MIN) -p html_data

$(VERSION_HEADER): FORCE
	@printf '%s\n' "#ifndef VERSION_H" "#define VERSION_H" \
		"#define VERSION_SW \"$(VERSION_EXTENSION)\"" \
		"#define BUILD_DATE \"$(BUILD_DATE)\"" \
		"#endif" > $(VERSION_HEADER).tmp
	@cmp -s $(VERSION_HEADER).tmp $(VERSION_HEADER) \
		&& rm -f $(VERSION_HEADER).tmp \
		|| mv $(VERSION_HEADER).tmp $(VERSION_HEADER)

httpd: html_data.h

$(SUBDIRS):
	$(MAKE) -C $@

clean: $(SUBDIRSCLEAN)
	-rm -f html_data.c html_data.h $(VERSION_HEADER)
	-if [ -d $(BUILDDIR) ]; then find $(BUILDDIR) -type f ! -name "*.bin" -delete; fi

distclean: $(SUBDIRSCLEAN)
	-rm -f html_data.c html_data.h $(VERSION_HEADER)
	-rm -rf $(BUILDDIR)

$(SUBDIRSCLEAN):
	$(MAKE) -C $(@:clean=) clean

# Objects depend on the flags they were built with, through a stamp file that
# is rewritten only when CC_FLAGS actually changes.
CCFLAGS_STAMP := $(BUILDDIR)/.ccflags

.PHONY: FORCE
FORCE:

$(CCFLAGS_STAMP): FORCE | create_build_dir
	@echo '$(CC_FLAGS)' | cmp -s - $@ 2>/dev/null || echo '$(CC_FLAGS)' > $@

$(BUILDDIR)/%.rel: %.c $(CCFLAGS_STAMP) | create_build_dir html_data.h
	$(CC) -MMD $(CC_FLAGS) -o $@ -c $<

$(BUILDDIR)/%.rel: %.asm $(CCFLAGS_STAMP) | create_build_dir
	${ASM} ${AFLAGS} -o $@ $<
#	mv -f $(addprefix $(basename $^), .lst .rel .sym) .

$(BUILDDIR)/rtlplayground.ihx: $(OBJS) $(BUILDDIR)/crtbank.rel $(BUILDDIR)/crc16.rel
	$(CC) $(CC_FLAGS) --xram-size 49151 -Wl-bHOME=0x00000 -Wl-bBANK1=0x14000 -Wl-bBANK2=0x24000 -Wl-bBANK3=0x34000 -Wl-r -o $@ $^

$(BUILDDIR)/rtlplayground.img: $(BUILDDIR)/rtlplayground.ihx
	objcopy --input-target=ihex -O binary $< $@

$(BUILDDIR)/rtlplayground-$(FILENAME_EXTENSION).bin: $(BUILDDIR)/rtlplayground.img $(DEFAULT_CONFIG) | tools
	if [ -e $@ ]; then rm $@; fi
	tools/output/imagebuilder -i $< $@
	tools/output/fileadder -a $(DEFAULT_CONFIG_LOCATION) -s $(IMAGESIZE) -d $(DEFAULT_CONFIG) $@
	tools/output/fileadder -a $(CONFIG_LOCATION) -s $(IMAGESIZE) -d $(DEFAULT_CONFIG) $@
	tools/output/fileadder -a $(HTML_LOCATION) -s $(IMAGESIZE) -z -d $(HTML_MIN) -b BANK1 $@
	tools/output/crc_calculator -u $@
	ln -sf $(MACHINE)/rtlplayground-$(FILENAME_EXTENSION).bin output/rtlplayground.bin

.PHONY: clean distclean all $(SUBDIRS) $(SUBDIRSCLEAN) create_build_dir

.PHONY:
machine_check:
	@mkdir -p $(BUILDDIR)/tmp
	@set -eo pipefail; \
	for MACHINE in `grep -E '^[[:space:]]*(//[[:space:]]*)?#define MACHINE_' machine.h | sed -E 's%^[[:space:]]*(//[[:space:]]*)?#define MACHINE_%%' | awk '{print $$1}' | sort -u`; \
	do \
	echo "Checking $${MACHINE}"; \
	$(CC) $(CC_FLAGS) -DMACHINE_$${MACHINE} -MMD -o $(BUILDDIR)/tmp/machine_check -c machine.c; \
	$(CC) $(CC_FLAGS) -DMACHINE_$${MACHINE} -MMD -o $(BUILDDIR)/tmp/machine_check -c machine_init.c; \
	done
	@rm -rf $(BUILDDIR)/tmp

-include $(DEPS)
