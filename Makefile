TARGET := underthec
BUILDDIR := build

-include config.mk

ifndef SRCS
$(error config.mk not found - run ./configure first)
endif

OBJS := $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS))
SCR_OBJS := $(patsubst %.c,$(BUILDDIR)/%.o,$(SCR_SRCS)) $(patsubst %.rc,$(BUILDDIR)/%.res.o,$(SCR_RC))
DEPS := $(sort $(OBJS:.o=.d) $(patsubst %.c,$(BUILDDIR)/%.d,$(SCR_SRCS)))

.PHONY: all clean install

all: $(TARGET) $(WEB_FILES) $(SCR_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(SCR_TARGET): $(SCR_OBJS)
	$(CC) $(SCR_OBJS) $(LDFLAGS) $(SCR_LDFLAGS) -o $@

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/%.res.o: %.rc src/version.h src/scr/scr_res.h src/scr/scr.manifest
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
