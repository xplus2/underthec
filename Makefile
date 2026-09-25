TARGET := underthec
BUILDDIR := build

-include config.mk

ifndef SRCS
$(error config.mk not found - run ./configure first)
endif

OBJS := $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS))
RC_OBJS := $(patsubst %.rc,$(BUILDDIR)/%.res.o,$(RC))
SCR_OBJS := $(patsubst %.c,$(BUILDDIR)/%.o,$(SCR_SRCS)) $(patsubst %.rc,$(BUILDDIR)/%.res.o,$(SCR_RC))
XSCR_OBJS := $(patsubst %.c,$(BUILDDIR)/%.o,$(XSCR_SRCS))
DEPS := $(sort $(OBJS:.o=.d) $(patsubst %.c,$(BUILDDIR)/%.d,$(SCR_SRCS)) $(patsubst %.c,$(BUILDDIR)/%.d,$(XSCR_SRCS)))

.PHONY: all clean install

all: $(TARGET) $(WEB_FILES) $(SCR_TARGET) $(XSCR_TARGET)

$(TARGET): $(OBJS) $(RC_OBJS)
	$(CC) $(OBJS) $(RC_OBJS) $(LDFLAGS) -o $@

$(SCR_TARGET): $(SCR_OBJS)
	$(CC) $(SCR_OBJS) $(LDFLAGS) $(SCR_LDFLAGS) -o $@

# X11/Xft/fontconfig .a pull in unresolved transitive deps -> sorry, not static
$(XSCR_TARGET): $(XSCR_OBJS)
	$(CC) $(XSCR_OBJS) $(filter-out -static,$(LDFLAGS)) $(XSCR_LDFLAGS) -o $@

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.res.o: %.rc src/version.h src/scr/scr_res.h src/scr/scr.manifest src/win/underthec.ico src/win/scr.ico
	@mkdir -p $(dir $@)
	$(WINDRES) -Isrc -O coff $< -o $@

$(BUILDDIR)/index.html: web/index.html
	@mkdir -p $(dir $@)
	cp $< $@

-include $(DEPS)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

install: $(TARGET)
	@if [ -n "$(WEB_FILES)" ]; then echo "install: not supported for the web build" >&2; exit 1; fi
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	@if [ -n "$(SCR_TARGET)" ]; then install -m 755 $(SCR_TARGET) $(DESTDIR)$(PREFIX)/bin/underthec.scr; fi
	@if [ -n "$(XSCR_TARGET)" ]; then \
		install -d $(DESTDIR)$(XSCR_BINDIR); \
		install -m 755 $(XSCR_TARGET) $(DESTDIR)$(XSCR_BINDIR)/underthec; \
		install -d $(DESTDIR)$(XSCR_CONFIGDIR); \
		install -m 644 src/xscr/underthec.xml $(DESTDIR)$(XSCR_CONFIGDIR)/underthec.xml; \
	fi
