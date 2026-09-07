#ifndef UNDERTHEC_ART_CRAB_H
#define UNDERTHEC_ART_CRAB_H

#include "../sprite.h"

static const char *const crab_frame_0[] = {
	" (\\/) (\\/)",
	"  \\(..)/",
	"  /\"  \"\\",
	NULL
};

static const char *const crab_frame_1[] = {
	"(\\/) (\\/)",
	"  \\(..)/",
	"  /\"  \"\\",
	NULL
};

static const char *const crab_mask_0[] = {
	"          ",
	"    ww  ",
	"       ",
	NULL
};

static const char *const crab_mask_1[] = {
	"          ",
	"    ww  ",
	"        ",
	NULL
};

static const struct sprite_pair crab_frames[2] = {
	{ crab_frame_0, crab_mask_0 },
	{ crab_frame_1, crab_mask_1 },
};

#endif
