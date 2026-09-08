#ifndef UNDERTHEC_ART_JELLYFISH_H
#define UNDERTHEC_ART_JELLYFISH_H

#include "../sprite.h"

static const char *const jellyfish_frame_0[] = {
	"  ___  ",
	" /   \\ ",
	"(_____)",
	" : ; : ",
	" ; : ; ",
	" : ; : ",
	NULL
};

static const char *const jellyfish_frame_1[] = {
	"  ___  ",
	" /   \\ ",
	"(_____)",
	" ; : ; ",
	" : ; : ",
	" ; : ; ",
	NULL
};

static const char *const jellyfish_mask_0[] = {
	"  ccc  ",
	" c   c ",
	"ccccccc",
	" m M m ",
	" M m M ",
	" m M m ",
	NULL
};

static const char *const jellyfish_mask_1[] = {
	"  ccc  ",
	" c   c ",
	"ccccccc",
	" M m M ",
	" m M m ",
	" M m M ",
	NULL
};

static const struct sprite_pair jellyfish_frames[2] = {
	{ jellyfish_frame_0, jellyfish_mask_0 },
	{ jellyfish_frame_1, jellyfish_mask_1 },
};

#endif
