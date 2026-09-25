#ifndef UNDERTHEC_ART_CASTLE_H
#define UNDERTHEC_ART_CASTLE_H

#include "../sprite.h"

static const char *const castle_image[] = {
	"               T~~",
	"               |",
	"              /^\\",
	"             /   \\",
	" _   _   _  /     \\  _   _   _",
	"[ ]_[ ]_[ ]/ _   _ \\[ ]_[ ]_[ ]",
	"|_=__-_ =_|_[ ]_[ ]_|_=-___-__|",
	" | _- =  | =_ = _    |= _=   |",
	" |= -[]  |- = _ =    |_-=[]  |",
	" | =_    |= - ___    | =_ =  |",
	" |=  []- |-  /| |\\   |=_ []  |",
	" |- =_   | =|=|=|=|  |- = -  |",
	" |_______|__|_|_|_|__|_______|",
	NULL
};

static const char *const castle_mask[] = {
	"               wRR",
	"               w",
	"              rrr",
	"             r   r",
	"            r     r",
	"           r       r",
	"",
	"",
	"",
	"              yyy",
	"             yy yy",
	"            yyyyyyy",
	"            yyyyyyy",
	NULL
};

static const struct sprite_pair castle = { castle_image, castle_mask };

#define CASTLE_DOOR_COL 13
#define CASTLE_DOOR_ROW 10
#define CASTLE_NAME_COL 10
#define CASTLE_NAME_ROW 8

static const char *const door_bars_2[] = { "/| |\\", " |=|=", "_|_|_", NULL };
static const char *const door_bars_1[] = { "/| |\\", " | |=", "_|_|_", NULL };
static const char *const door_bars_0[] = { "/| |\\", " | | ", "_|_|_", NULL };
static const char *const door_lift_1[] = { "/| |\\", "_|_|_", " oo o", NULL };
static const char *const door_lift_2[] = { "/|_|\\", "oo o ", "     ", NULL };
static const char *const door_lift_3[] = { "/ oo\\", "     ", "     ", NULL };
static const char *const door_open[] = { "/   \\", "     ", "     ", NULL };
static const char *const door_drop_1[] = { "/|_|\\", "     ", "     ", NULL };
static const char *const door_drop_2[] = { "/| |\\", "_|_|_", "     ", NULL };

static const char *const door_mask[] = { "yyyyy", "yyyyy", "yyyyy", NULL };
static const char *const door_lift_1_mask[] = { "yyyyy", "yyyyy", "yCCyC", NULL };
static const char *const door_lift_2_mask[] = { "yyyyy", "CCyCy", "yyyyy", NULL };
static const char *const door_lift_3_mask[] = { "yyCCy", "yyyyy", "yyyyy", NULL };

#define CASTLE_DOOR_STEPS 12
#define CASTLE_DOOR_OPEN_STEP 6

static const struct sprite_pair castle_door_frames[CASTLE_DOOR_STEPS] = {
	{ door_bars_2, door_mask },
	{ door_bars_1, door_mask },
	{ door_bars_0, door_mask },
	{ door_lift_1, door_lift_1_mask },
	{ door_lift_2, door_lift_2_mask },
	{ door_lift_3, door_lift_3_mask },
	{ door_open, door_mask },
	{ door_drop_1, door_mask },
	{ door_drop_2, door_mask },
	{ door_bars_0, door_mask },
	{ door_bars_1, door_mask },
	{ door_bars_2, door_mask },
};

static const char *const rubble_image[] = {
	"    ,   .     o   .      ,",
	" __/|_  / \\   _|_  /\\   __|\\__",
	"[_/=\\_]\\_/\\ [_=_]\\/[_/=\\_/]",
	" |_-_|   '   |_-_|   '   |_-_|",
	"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
	NULL
};

static const struct sprite_pair rubble = { rubble_image, NULL };

#endif
