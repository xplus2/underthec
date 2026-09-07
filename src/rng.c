#include "rng.h"

static uint64_t state;

void rng_seed(uint64_t seed) { state = seed ? seed : 0x9e3779b97f4a7c15ULL; }

static uint64_t next(void) {
  state ^= state >> 12;
  state ^= state << 25;
  state ^= state >> 27;
  return state * 0x2545F4914F6CDD1DULL;
}

int rng_int(int n) {
  if (n <= 0) return 0;
  return (int)(next() % (uint64_t)n);
}

double rng_double(double n) {
  double frac = (double)(next() >> 11) / (double)(1ULL << 53);
  return frac * n;
}
