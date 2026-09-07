#ifndef UNDERTHEC_ART_SWAN_H
#define UNDERTHEC_ART_SWAN_H

#include "../sprite.h"

static const char *const swan_image_0[] = {
	"",
	"       ___",
	",_    / _,\\",
	"| \\   \\( \\|",
	"|  \\_  \\\\",
	"(_   \\_) \\",
	"(\\_   `   \\",
	" \\   -=~  /",
	NULL
};

static const char *const swan_image_1[] = {
	"",
	" ___",
	"/,_ \\    _,",
	"|/ )/   / |",
	"  //  _/  |",
	" / ( /   _)",
	"/   `   _/)",
	"\\  ~=-   /",
	NULL
};

static const char *const swan_mask_0[] = {
	"",
	"",
	"         g",
	"         yy",
	NULL
};

static const char *const swan_mask_1[] = {
	"",
	"",
	" g",
	"yy",
	NULL
};

static const struct sprite_pair swan[2] = {
	{ swan_image_0, swan_mask_0 },
	{ swan_image_1, swan_mask_1 },
};

#endif
