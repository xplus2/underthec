#ifndef UNDERTHEC_VERSION_H
#define UNDERTHEC_VERSION_H

#define TOOL_NAME "underthec"
#define TOOL_DISPLAY_NAME "Under The C"

#define TOOL_WIDEN_(x) L##x
#define TOOL_WIDEN(x) TOOL_WIDEN_(x)
#define TOOL_DISPLAY_NAME_W TOOL_WIDEN(TOOL_DISPLAY_NAME)

/* keep in sync */
#define TOOL_VERSION "0.6.0"
#define TOOL_VERSION_MAJOR 0
#define TOOL_VERSION_MINOR 6
#define TOOL_VERSION_PATCH 0

#endif
