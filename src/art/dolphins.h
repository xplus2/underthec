#ifndef UNDERTHEC_ART_DOLPHINS_H
#define UNDERTHEC_ART_DOLPHINS_H

#include "../sprite.h"

static const char *const dolphin_image_0_0[] = {
	"",
	"        ,",
	"      __)\\_",
	"(\\_.-'    a`-.",
	"(/~~````(/~^^`",
	NULL
};

static const char *const dolphin_image_0_1[] = {
	"",
	"        ,",
	"(\\__  __)\\_",
	"(/~.''    a`-.",
	"    ````\\)~^^`",
	NULL
};

static const char *const dolphin_image_1_0[] = {
	"",
	"     ,",
	"   _/(__",
	".-'a    `-._/)",
	"'^^~\\)''''~~\\)",
	NULL
};

static const char *const dolphin_image_1_1[] = {
	"",
	"     ,",
	"   _/(__  __/)",
	".-'a    ``.~\\)",
	"'^^~(/''''",
	NULL
};

static const char *const dolphin_mask_0[] = {
	"",
	"",
	"",
	"          W",
	NULL
};

static const char *const dolphin_mask_1[] = {
	"",
	"",
	"",
	"   W",
	NULL
};

static const struct sprite_pair dolphins[2][2] = {
	{ { dolphin_image_0_0, dolphin_mask_0 }, { dolphin_image_0_1, dolphin_mask_0 } },
	{ { dolphin_image_1_0, dolphin_mask_1 }, { dolphin_image_1_1, dolphin_mask_1 } },
};

#endif
