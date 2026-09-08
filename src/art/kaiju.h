#ifndef UNDERTHEC_ART_KAIJU_H
#define UNDERTHEC_ART_KAIJU_H

#include "../sprite.h"

static const char *const kaiju_image_0[] = {
	"                _,-}}-._",
	"               \\/   {  \\/",
	"             _|(0//_ _\\0)",
	"            _|\\  (__''__)",
	"          _|//    WVVVVW",
	"          \\_/     /MMMM\\_",
	"         _|/     _ '---; /_",
	"  /\\    _\\/\\      /_   \\   /",
	" \\ (    \\/\\     /   /  |'VVV",
	"(  '-,._/_.)      'VVV \\",
	" /         \\   _) \\   _)",
	"  '....--''/__vvv)/__vvv)",
	"",
	NULL
};

static const char *const kaiju_mask_0[] = {
	"                22233222",
	"               22   2  22",
	"             332R222 22R2",
	"            333  22222222",
	"          3333    yyyyyy",
	"         3333     2yyyy22",
	"       33333     2 22222 22",
	"  22   33333      22   2   2",
	" 2 2    333     2   2  22yyy",
	"2  222222222      2yyy 2",
	" 2         2   22 2   22",
	"  222222222222yyy2222yyy2",
	"",
	NULL
};

static const char *const kaiju_image_1[] = {
	"    _.-{{-,_",
	"   \\/  }   \\/",
	"   (0/_ _\\\\0)|_",
	"   (__''__)  /|_",
	"    WVVVVW    \\\\|_",
	"   _/MMMM\\     \\_/",
	" _\\ ;---' _     \\|_",
	"\\   /   _\\      /\\/_    /\\",
	"VVV'|  \\   \\     /\\/    ) /",
	"    / VVV'      (._\\_.,-'  )",
	"    (_   / (_   /         \\",
	"   (vvv__\\(vvv__\\''--....'",
	"",
	NULL
};

static const char *const kaiju_mask_1[] = {
	"    22233222",
	"   22  2   22",
	"   2R22 222R233",
	"   22222222  333",
	"    yyyyyy    3333",
	"   22yyyy2     3333",
	" 22 22222 2     33333",
	"2   2   22      33333   22",
	"yyy22  2   2     333    2 2",
	"    2 yyy2      222222222  2",
	"    22   2 22   2         2",
	"   2yyy2222yyy222222222222",
	"",
	NULL
};

static const struct sprite_pair kaiju[2] = {
	{ kaiju_image_0, kaiju_mask_0 },
	{ kaiju_image_1, kaiju_mask_1 },
};

#endif
