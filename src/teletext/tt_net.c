#define _DEFAULT_SOURCE

#include "teletext.h"
#include "../xalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

struct tt_net {
  int unused;
};

struct tt_net *tt_net_open(const char *spec, int ttl, const char *iface, char *errbuf, size_t errbuf_len) {
  (void)spec;
  (void)ttl;
  (void)iface;
  snprintf(errbuf, errbuf_len, "multicast output is not supported on this platform");
  return NULL;
}

int tt_net_send(struct tt_net *n, const uint8_t *buf, size_t len) {
  (void)n;
  (void)buf;
  (void)len;
  return -1;
}

void tt_net_close(struct tt_net *n) {
  free(n);
}

#else

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

struct tt_net {
  int fd;
  struct sockaddr_storage dst;
  socklen_t dst_len;
};

static bool parse_port(const char *s, int *out) {
  char *end = NULL;
  long n = strtol(s, &end, 10);
  if (s[0] == '\0' || *end != '\0' || n < 1 || n > 65535) return false;
  *out = (int)n;
  return true;
}

static bool parse_spec(const char *spec, struct sockaddr_storage *ss, socklen_t *len, char *errbuf, size_t errbuf_len) {
  char host[64];
  const char *port_s;
  bool v6 = spec[0] == '[';
  if (v6) {
    const char *close = strchr(spec, ']');
    if (close == NULL || close[1] != ':' || (size_t)(close - spec) >= sizeof host) {
      snprintf(errbuf, errbuf_len, "invalid multicast address '%s', expected [GROUP]:PORT", spec);
      return false;
    }
    memcpy(host, spec + 1, (size_t)(close - spec - 1));
    host[close - spec - 1] = '\0';
    port_s = close + 2;
  } else {
    const char *colon = strrchr(spec, ':');
    if (colon == NULL || (size_t)(colon - spec) >= sizeof host) {
      snprintf(errbuf, errbuf_len, "invalid multicast address '%s', expected GROUP:PORT", spec);
      return false;
    }
    memcpy(host, spec, (size_t)(colon - spec));
    host[colon - spec] = '\0';
    port_s = colon + 1;
  }
  int port;
  if (!parse_port(port_s, &port)) {
    snprintf(errbuf, errbuf_len, "invalid port '%s'", port_s);
    return false;
  }
  memset(ss, 0, sizeof(*ss));
  if (v6) {
    struct sockaddr_in6 *a = (struct sockaddr_in6 *)ss;
    a->sin6_family = AF_INET6;
    a->sin6_port = htons((uint16_t)port);
    if (inet_pton(AF_INET6, host, &a->sin6_addr) != 1 || a->sin6_addr.s6_addr[0] != 0xFF) {
      snprintf(errbuf, errbuf_len, "'%s' is not an IPv6 multicast group", host);
      return false;
    }
    *len = sizeof(*a);
  } else {
    struct sockaddr_in *a = (struct sockaddr_in *)ss;
    a->sin_family = AF_INET;
    a->sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &a->sin_addr) != 1 || (ntohl(a->sin_addr.s_addr) >> 28) != 0xE) {
      snprintf(errbuf, errbuf_len, "'%s' is not an IPv4 multicast group", host);
      return false;
    }
    *len = sizeof(*a);
  }
  return true;
}

static bool set_iface(int fd, int family, const char *iface, char *errbuf, size_t errbuf_len) {
  if (iface == NULL) return true;
  if (family == AF_INET6) {
    unsigned idx = if_nametoindex(iface);
    if (idx == 0 || setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &idx, sizeof idx) != 0) {
      snprintf(errbuf, errbuf_len, "cannot use interface '%s'", iface);
      return false;
    }
    return true;
  }
  struct in_addr addr;
  if (inet_pton(AF_INET, iface, &addr) != 1 || setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof addr) != 0) {
    snprintf(errbuf, errbuf_len, "cannot use interface address '%s'", iface);
    return false;
  }
  return true;
}

static bool set_ttl(int fd, int family, int ttl, char *errbuf, size_t errbuf_len) {
  int rc;
  if (family == AF_INET6) {
    rc = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &ttl, sizeof ttl);
  } else {
    unsigned char t = (unsigned char)ttl;
    rc = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &t, sizeof t);
  }
  if (rc != 0) snprintf(errbuf, errbuf_len, "cannot set multicast ttl");
  return rc == 0;
}

struct tt_net *tt_net_open(const char *spec, int ttl, const char *iface, char *errbuf, size_t errbuf_len) {
  struct tt_net *n = xcalloc(1, sizeof(*n));
  if (!parse_spec(spec, &n->dst, &n->dst_len, errbuf, errbuf_len)) {
    free(n);
    return NULL;
  }
  int family = n->dst.ss_family;
  n->fd = socket(family, SOCK_DGRAM, 0);
  if (n->fd < 0) {
    snprintf(errbuf, errbuf_len, "cannot create socket");
    free(n);
    return NULL;
  }
  if (!set_ttl(n->fd, family, ttl, errbuf, errbuf_len) || !set_iface(n->fd, family, iface, errbuf, errbuf_len)) {
    close(n->fd);
    free(n);
    return NULL;
  }
  return n;
}

int tt_net_send(struct tt_net *n, const uint8_t *buf, size_t len) {
  ssize_t rc = sendto(n->fd, buf, len, 0, (const struct sockaddr *)&n->dst, n->dst_len);
  return rc == (ssize_t)len ? 0 : -1;
}

void tt_net_close(struct tt_net *n) {
  close(n->fd);
  free(n);
}

#endif
