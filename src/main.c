/*
 * main.c - ipcalc: IPv4 subnet information and VLSM planning.
 */
#include "ipv4.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s <address>/<prefix>                       subnet details\n"
            "  %s -v <network>/<prefix> <hosts> [hosts...]  VLSM plan\n"
            "\nExamples:\n"
            "  %s 192.168.1.10/24\n"
            "  %s -v 192.168.0.0/24 50 20 10 2\n",
            prog, prog, prog, prog);
}

static void print_info(ipv4_net_t net)
{
    char a[IPV4_STR_LEN], m[IPV4_STR_LEN], w[IPV4_STR_LEN];
    char n[IPV4_STR_LEN], b[IPV4_STR_LEN], f[IPV4_STR_LEN], l[IPV4_STR_LEN];
    uint32_t network = ipv4_network(net);
    uint32_t broadcast = ipv4_broadcast(net);
    uint32_t first = net.prefix >= 31 ? network : network + 1;
    uint32_t last = net.prefix >= 31 ? broadcast : broadcast - 1;

    ipv4_format(net.addr, a);
    ipv4_format(ipv4_mask(net.prefix), m);
    ipv4_format(~ipv4_mask(net.prefix), w);
    ipv4_format(network, n);
    ipv4_format(broadcast, b);
    ipv4_format(first, f);
    ipv4_format(last, l);

    printf("Address    : %s\n", a);
    printf("Netmask    : %s = /%d\n", m, net.prefix);
    printf("Wildcard   : %s\n", w);
    printf("Network    : %s/%d\n", n, net.prefix);
    printf("Broadcast  : %s\n", net.prefix >= 31 ? "n/a" : b);
    printf("First host : %s\n", f);
    printf("Last host  : %s\n", l);
    printf("Hosts      : %" PRIu64 "\n", ipv4_usable_hosts(net.prefix));
    printf("Class      : %c (%s)\n", ipv4_class(net.addr),
           ipv4_is_private(net.addr) ? "private" : "public");
}

static int print_vlsm(ipv4_net_t base, char **hosts, size_t n)
{
    vlsm_entry_t *entries = calloc(n, sizeof *entries);
    char net_s[IPV4_STR_LEN], first_s[IPV4_STR_LEN], last_s[IPV4_STR_LEN], bc_s[IPV4_STR_LEN];
    uint64_t used = 0;

    if (entries == NULL) {
        perror("calloc");
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < n; i++) {
        char *end;
        unsigned long h = strtoul(hosts[i], &end, 10);

        if (*hosts[i] == '\0' || *end != '\0' || h == 0 || h > UINT32_MAX) {
            fprintf(stderr, "Invalid host count: %s\n", hosts[i]);
            free(entries);
            return EXIT_FAILURE;
        }
        entries[i].index = i;
        entries[i].hosts_needed = (uint32_t)h;
    }

    if (ipv4_vlsm(base, entries, n) < 0) {
        ipv4_format(ipv4_network(base), net_s);
        fprintf(stderr, "Not enough addresses in %s/%d for these subnets\n", net_s, base.prefix);
        free(entries);
        return EXIT_FAILURE;
    }

    printf("%-6s %7s %7s  %-19s %-33s %s\n", "LAN", "Needed", "Usable", "Subnet", "Host range", "Broadcast");
    for (size_t i = 0; i < n; i++) {
        ipv4_net_t net = entries[i].net;
        char subnet[32], range[48];

        ipv4_format(net.addr, net_s);
        ipv4_format(ipv4_network(net) + 1, first_s);
        ipv4_format(ipv4_broadcast(net) - 1, last_s);
        ipv4_format(ipv4_broadcast(net), bc_s);
        snprintf(subnet, sizeof subnet, "%s/%d", net_s, net.prefix);
        snprintf(range, sizeof range, "%s - %s", first_s, last_s);
        printf("LAN%-3zu %7" PRIu32 " %7" PRIu64 "  %-19s %-33s %s\n",
               entries[i].index + 1, entries[i].hosts_needed, ipv4_usable_hosts(net.prefix),
               subnet, range, bc_s);
        used += UINT64_C(1) << (32 - net.prefix);
    }
    printf("\nAddresses used: %" PRIu64 " / %" PRIu64 "\n", used, UINT64_C(1) << (32 - base.prefix));
    free(entries);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    ipv4_net_t net;

    if (argc == 2 && strcmp(argv[1], "-h") != 0) {
        if (ipv4_parse_cidr(argv[1], &net) < 0) {
            fprintf(stderr, "Invalid address: %s (expected a.b.c.d/prefix)\n", argv[1]);
            return EXIT_FAILURE;
        }
        print_info(net);
        return EXIT_SUCCESS;
    }
    if (argc >= 4 && strcmp(argv[1], "-v") == 0) {
        if (ipv4_parse_cidr(argv[2], &net) < 0) {
            fprintf(stderr, "Invalid network: %s (expected a.b.c.d/prefix)\n", argv[2]);
            return EXIT_FAILURE;
        }
        return print_vlsm(net, argv + 3, (size_t)(argc - 3));
    }
    usage(argv[0]);
    return argc == 2 ? EXIT_SUCCESS : EXIT_FAILURE;
}
