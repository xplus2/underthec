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
