#ifndef UNDERTHEC_ART_MISC_H
#define UNDERTHEC_ART_MISC_H

#include "../sprite.h"

static const char *const water_line_segment_0[] = {
	"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
	NULL
};

static const char *const water_line_segment_1[] = {
	"^^^^ ^^^  ^^^   ^^^    ^^^^      ",
	NULL
};

static const char *const water_line_segment_2[] = {
	"^^^^      ^^^^     ^^^    ^^     ",
	NULL
};

static const char *const water_line_segment_3[] = {
	"^^      ^^^^      ^^^    ^^^^^^  ",
	NULL
};

static const char *const *const water_line_segments[] = {
	water_line_segment_0,
	water_line_segment_1,
	water_line_segment_2,
	water_line_segment_3,
};

static const char *const splat_frame_0[] = {
	"",
	"   .",
	"  ***",
	"   '",
	NULL
};

static const char *const splat_frame_1[] = {
	"",
	" \",*;`",
	" \"*,**",
	" *\"'~'",
	NULL
};

static const char *const splat_frame_2[] = {
	"  , ,",
	" \" \",\"'",
	" *\" *'\"",
	"  \" ; .",
	NULL
};

static const char *const splat_frame_3[] = {
	"* ' , ' `",
	"' ` * . '",
	" ' `' \",'",
	"* ' \" * .",
	"\" * ', '",
	NULL
};

static const struct sprite_pair splat_frames[4] = {
	{ splat_frame_0, NULL },
	{ splat_frame_1, NULL },
	{ splat_frame_2, NULL },
	{ splat_frame_3, NULL },
};

static const char *const bubble_frame_0[] = { ".", NULL };
static const char *const bubble_frame_1[] = { "o", NULL };
static const char *const bubble_frame_2[] = { "O", NULL };

static const struct sprite_pair bubble_frames[5] = {
	{ bubble_frame_0, NULL },
	{ bubble_frame_1, NULL },
	{ bubble_frame_2, NULL },
	{ bubble_frame_2, NULL },
	{ bubble_frame_2, NULL },
};

static const char *const teeth_shape[] = { "*", NULL };
static const struct sprite_pair teeth_frame[1] = {
	{ teeth_shape, NULL },
};

#endif
