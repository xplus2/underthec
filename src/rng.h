#ifndef UNDERTHEC_RNG_H
#define UNDERTHEC_RNG_H

#include <stdint.h>

void rng_seed(uint64_t seed);
int rng_int(int n);
double rng_double(double n);

#endif
