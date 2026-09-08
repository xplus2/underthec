#ifndef UNDERTHEC_XALLOC_H
#define UNDERTHEC_XALLOC_H

#include <stdio.h>
#include <stdlib.h>

static inline void *xmalloc(size_t size) {
  void *p = malloc(size);
  if (p == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  return p;
}

static inline void *xcalloc(size_t nmemb, size_t size) {
  void *p = calloc(nmemb, size);
  if (p == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  return p;
}

static inline void *xrealloc(void *ptr, size_t size) {
  void *p = realloc(ptr, size);
  if (p == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  return p;
}

#endif
