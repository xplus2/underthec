#ifndef UNDERTHEC_ART_SEAHORSE_H
#define UNDERTHEC_ART_SEAHORSE_H

#include "../sprite.h"

static const char *const seahorse_image_0_0[] = {
	"          ,",
	"   _\?\?\?.`^ '(o\\",
	"  (_\\  ;_:,,,_.`._",
	" (_ (\\ \\--\\--\\ `.'",
	"(_ = (\\|--:'-|",
	"(_ _ (/;--\"-'|",
	" (_ (/ ;';--/",
	"  (_/  /-;_.'",
	"      |_./\?\?_",
	"       \\'\\_',;",
	"        '.__.'",
	NULL
};

static const char *const seahorse_image_0_1[] = {
	"          ,",
	"       .`^ '(o\\",
	"   (\\  ;_:,,,_.`._",
	"  (_(\\ \\--\\--\\ `.'",
	" (_= (\\|--:'-|",
	" (_ _(/;--\"-'|",
	"  (_(/ ;';--/",
	"   (/  /-;_.'",
	"      |_./\?\?_",
	"       \\'\\_',;",
	"        '.__.'",
	NULL
};

static const char *const seahorse_image_1_0[] = {
	"       ,",
	"   /o)` ^'.\?\?\?_",
	"_.'._,,,:_;  /_)",
	"`.' /--/--/ /) _)",
	"    |-`:--|/) = _)",
	"    |`-\"--;\\) _ _)",
	"     \\--;`; \\) _)",
	"     `._;-\\  \\_)",
	"     _\?\?\\._|",
	"    ;,`_/`/",
	"    `.__.`",
	NULL
};

static const char *const seahorse_image_1_1[] = {
	"       ,",
	"   /o)` ^'.",
	"_.'._,,,:_;  /)",
	"`.' /--/--/ /)_)",
	"    |-`:--|/) =_)",
	"    |`-\"--;\\)_ _)",
	"     \\--;`; \\)_)",
	"     `._;-\\  \\)",
	"     _\?\?\\._|",
	"    ;,`_/`/",
	"    `.__.`",
	NULL
};

static const char *const seahorse_mask_0[] = {
	"          1",
	"   2   111 11W1",
	"  222  11111111111",
	" 22222 1111111 111",
	"2222 221111111",
	"22222221111111",
	" 22222 111111",
	"  222  111111",
	"      1111  1",
	"       1111111",
	"        111111",
	NULL
};

static const char *const seahorse_mask_1[] = {
	"       1",
	"   1W11 111   2",
	"11111111111  222",
	"111 1111111 22222",
	"    111111122 2222",
	"    11111112222222",
	"     111111 22222",
	"     111111  222",
	"     1  1111",
	"    1111111",
	"    111111",
	NULL
};

/* one mask per direction, covers both frames */
static const struct sprite_pair seahorse[2][2] = {
	{ { seahorse_image_0_0, seahorse_mask_0 }, { seahorse_image_0_1, seahorse_mask_0 } },
	{ { seahorse_image_1_0, seahorse_mask_1 }, { seahorse_image_1_1, seahorse_mask_1 } },
};

#endif
