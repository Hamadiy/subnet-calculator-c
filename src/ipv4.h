/*
 * ipv4.h - IPv4 addressing helpers: parsing, masks, subnetting and VLSM.
 */
#ifndef IPV4_H
#define IPV4_H

#include <stddef.h>
#include <stdint.h>

#define IPV4_STR_LEN 16 /* "255.255.255.255" + NUL */

typedef struct {
    uint32_t addr;   /* host byte order */
    int      prefix; /* 0..32 */
} ipv4_net_t;

typedef struct {
    size_t     index;        /* position of the request on the command line */
    uint32_t   hosts_needed;
    ipv4_net_t net;          /* filled by ipv4_vlsm() */
} vlsm_entry_t;

int      ipv4_parse_addr(const char *s, uint32_t *out);
int      ipv4_parse_cidr(const char *s, ipv4_net_t *out);
void     ipv4_format(uint32_t addr, char *buf);

uint32_t ipv4_mask(int prefix);
uint32_t ipv4_network(ipv4_net_t net);
uint32_t ipv4_broadcast(ipv4_net_t net);
uint64_t ipv4_usable_hosts(int prefix);

char     ipv4_class(uint32_t addr);
int      ipv4_is_private(uint32_t addr);

/* Smallest prefix (<= /30) offering at least `hosts` usable addresses, or -1. */
int      ipv4_prefix_for_hosts(uint32_t hosts);

/*
 * Allocates one subnet per entry inside `base`, largest first (VLSM).
 * Entries are sorted by decreasing size. Returns 0, or -1 if they do not fit.
 */
int      ipv4_vlsm(ipv4_net_t base, vlsm_entry_t *entries, size_t n);

#endif /* IPV4_H */
