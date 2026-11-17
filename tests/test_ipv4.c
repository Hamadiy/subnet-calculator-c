/*
 * test_ipv4.c - Unit tests for the IPv4 helpers.
 */
#include "../src/ipv4.h"

#include <stdio.h>
#include <string.h>

static int tests = 0;
static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        tests++;                                                             \
        if (!(cond)) {                                                       \
            failures++;                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
        }                                                                    \
    } while (0)

static uint32_t ip(const char *s)
{
    uint32_t a = 0;

    ipv4_parse_addr(s, &a);
    return a;
}

static void test_parse(void)
{
    uint32_t a;
    ipv4_net_t n;

    CHECK(ipv4_parse_addr("192.168.1.10", &a) == 0 && a == 0xC0A8010Au);
    CHECK(ipv4_parse_addr("0.0.0.0", &a) == 0 && a == 0);
    CHECK(ipv4_parse_addr("255.255.255.255", &a) == 0 && a == 0xFFFFFFFFu);
    CHECK(ipv4_parse_addr("256.1.1.1", &a) < 0);
    CHECK(ipv4_parse_addr("1.2.3", &a) < 0);
    CHECK(ipv4_parse_addr("1.2.3.4.5", &a) < 0);
    CHECK(ipv4_parse_addr("1.2.3.", &a) < 0);
    CHECK(ipv4_parse_addr("a.b.c.d", &a) < 0);
    CHECK(ipv4_parse_addr("1.2.3.4 ", &a) < 0);
    CHECK(ipv4_parse_addr("", &a) < 0);

    CHECK(ipv4_parse_cidr("10.0.0.1/8", &n) == 0 && n.addr == 0x0A000001u && n.prefix == 8);
    CHECK(ipv4_parse_cidr("10.0.0.1/32", &n) == 0 && n.prefix == 32);
    CHECK(ipv4_parse_cidr("10.0.0.1/33", &n) < 0);
    CHECK(ipv4_parse_cidr("10.0.0.1/", &n) < 0);
    CHECK(ipv4_parse_cidr("10.0.0.1", &n) < 0);
    CHECK(ipv4_parse_cidr("10.0.0.1/2a", &n) < 0);
}

static void test_masks(void)
{
    ipv4_net_t n = { ip("192.168.1.130"), 26 };
    char buf[IPV4_STR_LEN];

    CHECK(ipv4_mask(0) == 0);
    CHECK(ipv4_mask(24) == 0xFFFFFF00u);
    CHECK(ipv4_mask(32) == 0xFFFFFFFFu);
    CHECK(ipv4_network(n) == ip("192.168.1.128"));
    CHECK(ipv4_broadcast(n) == ip("192.168.1.191"));
    CHECK(ipv4_usable_hosts(24) == 254);
    CHECK(ipv4_usable_hosts(30) == 2);
    CHECK(ipv4_usable_hosts(31) == 2);
    CHECK(ipv4_usable_hosts(32) == 1);
    CHECK(ipv4_usable_hosts(0) == 4294967294u);

    ipv4_format(ip("172.16.5.4"), buf);
    CHECK(strcmp(buf, "172.16.5.4") == 0);
}

static void test_classes(void)
{
    CHECK(ipv4_class(ip("10.1.1.1")) == 'A');
    CHECK(ipv4_class(ip("172.16.0.1")) == 'B');
    CHECK(ipv4_class(ip("192.168.0.1")) == 'C');
    CHECK(ipv4_class(ip("224.0.0.5")) == 'D');
    CHECK(ipv4_class(ip("250.0.0.1")) == 'E');

    CHECK(ipv4_is_private(ip("10.200.3.4")));
    CHECK(ipv4_is_private(ip("172.31.255.255")));
    CHECK(!ipv4_is_private(ip("172.32.0.1")));
    CHECK(ipv4_is_private(ip("192.168.50.1")));
    CHECK(!ipv4_is_private(ip("8.8.8.8")));
}

static void test_prefix_for_hosts(void)
{
    CHECK(ipv4_prefix_for_hosts(0) == -1);
    CHECK(ipv4_prefix_for_hosts(1) == 30);
    CHECK(ipv4_prefix_for_hosts(2) == 30);
    CHECK(ipv4_prefix_for_hosts(3) == 29);
    CHECK(ipv4_prefix_for_hosts(50) == 26);
    CHECK(ipv4_prefix_for_hosts(62) == 26);
    CHECK(ipv4_prefix_for_hosts(63) == 25);
    CHECK(ipv4_prefix_for_hosts(254) == 24);
    CHECK(ipv4_prefix_for_hosts(255) == 23);
}

static void test_vlsm(void)
{
    ipv4_net_t base = { ip("192.168.0.0"), 24 };
    vlsm_entry_t e[4] = {
        { 0, 10, { 0, 0 } }, { 1, 50, { 0, 0 } }, { 2, 2, { 0, 0 } }, { 3, 20, { 0, 0 } }
    };
    vlsm_entry_t too_big[2] = { { 0, 200, { 0, 0 } }, { 1, 100, { 0, 0 } } };
    ipv4_net_t small = { ip("10.0.0.0"), 28 };
    vlsm_entry_t wider[1] = { { 0, 20, { 0, 0 } } };

    CHECK(ipv4_vlsm(base, e, 4) == 0);
    /* Sorted largest first: 50, 20, 10, 2 hosts. */
    CHECK(e[0].index == 1 && e[0].net.addr == ip("192.168.0.0") && e[0].net.prefix == 26);
    CHECK(e[1].index == 3 && e[1].net.addr == ip("192.168.0.64") && e[1].net.prefix == 27);
    CHECK(e[2].index == 0 && e[2].net.addr == ip("192.168.0.96") && e[2].net.prefix == 28);
    CHECK(e[3].index == 2 && e[3].net.addr == ip("192.168.0.112") && e[3].net.prefix == 30);

    CHECK(ipv4_vlsm(base, too_big, 2) == -1);
    CHECK(ipv4_vlsm(small, wider, 1) == -1); /* a /27 cannot fit in a /28 */
}

int main(void)
{
    test_parse();
    test_masks();
    test_classes();
    test_prefix_for_hosts();
    test_vlsm();

    printf("%d tests, %d failure(s)\n", tests, failures);
    return failures == 0 ? 0 : 1;
}
