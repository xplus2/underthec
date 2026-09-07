#ifndef UNDERTHEC_ART_SHIP_H
#define UNDERTHEC_ART_SHIP_H

#include "../sprite.h"

static const char *const ship_image_0[] = {
	"     |    |    |",
	"    )_)  )_)  )_)",
	"   )___))___))___)\\",
	"  )____)____)_____)\\\\",
	"_____|____|____|____\\\\\\__",
	"\\                   /",
	NULL
};

static const char *const ship_image_1[] = {
	"         |    |    |",
	"        (_(  (_(  (_(",
	"      /(___((___((___(",
	"    //(_____(____(____(",
	"__///____|____|____|_____",
	"    \\                   /",
	NULL
};

static const char *const ship_mask_0[] = {
	"     y    y    y",
	"",
	"                  w",
	"                   ww",
	"yyyyyyyyyyyyyyyyyyyywwwyy",
	"y                   y",
	NULL
};

static const char *const ship_mask_1[] = {
	"         y    y    y",
	"",
	"      w",
	"    ww",
	"yywwwyyyyyyyyyyyyyyyyyyyy",
	"    y                   y",
	NULL
};

static const struct sprite_pair ship[2] = {
	{ ship_image_0, ship_mask_0 },
	{ ship_image_1, ship_mask_1 },
};

#endif
