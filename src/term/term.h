#ifndef UNDERTHEC_TERM_H
#define UNDERTHEC_TERM_H

#include <stdbool.h>

#include "../canvas.h"

/* 0 on success. raw mode, alt screen, hide cursor */
int term_init(void);

/* restore term */
void term_shutdown(void);

void term_size(int *cols, int *rows);

bool term_has_color(void);

/* poll keypress */
int term_poll_key(int timeout_ms);

/* write changes only */
void term_present(const struct canvas *c);

#endif
