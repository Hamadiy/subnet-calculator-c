/*
 * ipv4.c - IPv4 addressing helpers: parsing, masks, subnetting and VLSM.
 */
#include "ipv4.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ipv4_parse_addr(const char *s, uint32_t *out)
{
    uint32_t addr = 0;

    for (int i = 0; i < 4; i++) {
        unsigned value = 0;
        int digits = 0;

        if (!isdigit((unsigned char)*s))
            return -1;
        while (isdigit((unsigned char)*s)) {
            value = value * 10 + (unsigned)(*s - '0');
            s++;
            if (++digits > 3 || value > 255)
                return -1;
        }
        addr = (addr << 8) | value;
        if (i < 3) {
            if (*s != '.')
                return -1;
            s++;
        }
    }
    if (*s != '\0')
        return -1;
    *out = addr;
    return 0;
}

int ipv4_parse_cidr(const char *s, ipv4_net_t *out)
{
    char addr_part[IPV4_STR_LEN];
    const char *slash = strchr(s, '/');
    size_t len;
    int prefix = 0;

    if (slash == NULL)
        return -1;
    len = (size_t)(slash - s);
    if (len == 0 || len >= sizeof addr_part)
        return -1;
    memcpy(addr_part, s, len);
    addr_part[len] = '\0';
    if (ipv4_parse_addr(addr_part, &out->addr) < 0)
        return -1;

    s = slash + 1;
    if (*s == '\0' || strlen(s) > 2)
        return -1;
    for (; *s != '\0'; s++) {
        if (!isdigit((unsigned char)*s))
            return -1;
        prefix = prefix * 10 + (*s - '0');
    }
    if (prefix > 32)
        return -1;
    out->prefix = prefix;
    return 0;
}

void ipv4_format(uint32_t addr, char *buf)
{
    snprintf(buf, IPV4_STR_LEN, "%u.%u.%u.%u",
             (unsigned)(addr >> 24), (unsigned)((addr >> 16) & 0xff),
             (unsigned)((addr >> 8) & 0xff), (unsigned)(addr & 0xff));
}

uint32_t ipv4_mask(int prefix)
{
    return prefix <= 0 ? 0 : 0xFFFFFFFFu << (32 - prefix);
}

uint32_t ipv4_network(ipv4_net_t net)
{
    return net.addr & ipv4_mask(net.prefix);
}

uint32_t ipv4_broadcast(ipv4_net_t net)
{
    return ipv4_network(net) | ~ipv4_mask(net.prefix);
}

uint64_t ipv4_usable_hosts(int prefix)
{
    if (prefix == 32)
        return 1;
    if (prefix == 31)
        return 2; /* point-to-point link, RFC 3021 */
    return (UINT64_C(1) << (32 - prefix)) - 2;
}

char ipv4_class(uint32_t addr)
{
    unsigned first = addr >> 24;

    if (first < 128) return 'A';
    if (first < 192) return 'B';
    if (first < 224) return 'C';
    if (first < 240) return 'D';
    return 'E';
}

int ipv4_is_private(uint32_t addr)
{
    return (addr & 0xFF000000u) == 0x0A000000u   /* 10.0.0.0/8     */
        || (addr & 0xFFF00000u) == 0xAC100000u   /* 172.16.0.0/12  */
        || (addr & 0xFFFF0000u) == 0xC0A80000u;  /* 192.168.0.0/16 */
}

int ipv4_prefix_for_hosts(uint32_t hosts)
{
    if (hosts == 0)
        return -1;
    for (int prefix = 30; prefix >= 0; prefix--)
        if ((UINT64_C(1) << (32 - prefix)) - 2 >= hosts)
            return prefix;
    return -1;
}

static int by_size_desc(const void *a, const void *b)
{
    const vlsm_entry_t *x = a, *y = b;

    if (x->hosts_needed != y->hosts_needed)
        return x->hosts_needed > y->hosts_needed ? -1 : 1;
    return x->index < y->index ? -1 : (x->index > y->index);
}

int ipv4_vlsm(ipv4_net_t base, vlsm_entry_t *entries, size_t n)
{
    uint64_t cursor = ipv4_network(base);
    uint64_t end = cursor + (UINT64_C(1) << (32 - base.prefix));

    qsort(entries, n, sizeof *entries, by_size_desc);

    /* Allocating from the largest block down keeps every subnet aligned. */
    for (size_t i = 0; i < n; i++) {
        int prefix = ipv4_prefix_for_hosts(entries[i].hosts_needed);
        uint64_t size;

        if (prefix < 0 || prefix < base.prefix)
            return -1;
        size = UINT64_C(1) << (32 - prefix);
        if (cursor + size > end)
            return -1;
        entries[i].net.addr = (uint32_t)cursor;
        entries[i].net.prefix = prefix;
        cursor += size;
    }
    return 0;
}
