#ifndef UNDERTHEC_ART_KAIJU_H
#define UNDERTHEC_ART_KAIJU_H

#include "../sprite.h"

static const char *const kaiju_image_0[] = {
    "            _,-^^-,_",
    "           /\\  ### /\\",
    "         _|{o\\  = /o}",
    "        _|/  {~~''~~}",
    "       |\\/     wwww",
    "  _   _\\|      \\NNN",
    " / }  \\/        \\ \\ \\",
    "{  ~_/  }       ww|ww",
    " \\     /    }\\     }",
    "  '~~~^\\vvvv} \\vvvv}",
    NULL
};

static const char *const kaiju_mask_0[] = {
    "            22233222",
    "           22  222 22",
    "         322R2  2 2R2",
    "        332  22222222",
    "       332     wwww",
    "  2   332      2www",
    " 2 2  32        2 2 2",
    "2  222  2       yy2yy",
    " 2     2    22     2",
    "  222222yyyy2 2yyyy2",
    NULL
};

static const char *const kaiju_image_1[] = {
	" _,-^^-,_",
	"/\\ ###  /\\",
	"{o\\ =  /o}|_",
	"{~~''~~}  \\|_",
	"  wwww     \\/|",
	"  NNN/      |/_   _",
	"/ / /        \\/  { \\",
	"ww|ww       {  \\_~  }",
	" {     /{    \\     /",
	" {vvvv/ {vvvv/^~~~'",
	NULL
};

static const char *const kaiju_mask_1[] = {
	" 22233222",
	"22 222  22",
	"2R2 2  2R223",
	"22222222  233",
	"  wwww     233",
	"  www2      233   2",
	"2 2 2        23  2 2",
	"yy2yy       2  222  2",
	" 2     22    2     2",
	" 2yyyy2 2yyyy222222",
	NULL
};

static const struct sprite_pair kaiju[2] = {
	{ kaiju_image_0, kaiju_mask_0 },
	{ kaiju_image_1, kaiju_mask_1 },
};

#endif
