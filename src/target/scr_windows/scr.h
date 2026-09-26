#ifndef UNDERTHEC_SCR_H
#define UNDERTHEC_SCR_H

#include <stdbool.h>
#include <stddef.h>
#include <windows.h>

#include "../../config.h"

/* registry. key missing=defaults. false: err */
bool scr_config_load(struct config *cfg, char *err, size_t err_len);
void scr_config_dialog(HINSTANCE inst, HWND parent);

#endif
