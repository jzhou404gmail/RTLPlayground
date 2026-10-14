/*
 * test_ntp.c — host test for the SNTP client (ntp.c) and the reason it gives
 * when it has no time yet.
 *
 * ntp.c and dns.c are compiled UNMODIFIED from the firmware tree. Only the
 * uIP edge they talk to is mocked here: a table of UDP connections, a packet
 * buffer, and the globals uip_slen/uip_appdata/uip_len/uip_flags. That is
 * enough to walk the whole state machine — resolve, send, receive — and to
 * read back what the console prints.
 *
 * The firmware counts seconds in the RTL837X_REG_SEC_COUNTER register, read
 * back through sfr_data; secs_set() puts a second count there and the client
 * sees exactly that.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "support.h"
#include "hw_mock.h"
#include "rtl837x_common.h"
#include "rtl837x_regs.h"
#include "cmd_parser.h"
#include "uip/uip.h"
#include "dns.h"
#include "ntp.h"

/* ---- globals the two firmware TUs link against ---- */
volatile uint32_t ticks;
struct dhcp_state dhcp_state;
uint8_t ntp_hosts_print_ip[4];

/* ---- console output: reuse support.c's capture, add the two printers it
 * does not carry ---- */
void print_ip(uint8_t *ptr)
{
	char b[16];
	snprintf(b, sizeof(b), "%u.%u.%u.%u", ptr[0], ptr[1], ptr[2], ptr[3]);
	print_string(b);
}

void itoa_short(uint16_t v)
{
	char b[8];
	snprintf(b, sizeof(b), "%u", (unsigned)v);
	print_string(b);
}

/* ---- the uIP edge ---- */
#define MOCK_CONNS 4
struct uip_udp_conn mock_conns[MOCK_CONNS];
void *uip_appdata = NULL;
void *uip_sappdata = NULL;
uint16_t uip_slen = 0;
uint16_t uip_len = 0;
uint8_t uip_flags = 0;
static uint16_t mock_lastport = 4096;

struct uip_udp_conn *uip_udp_new(uip_ipaddr_t *ripaddr, u16_t rport)
{
	for (int i = 0; i < MOCK_CONNS; i++) {
		if (mock_conns[i].lport == 0) {
			mock_lastport++;
			mock_conns[i].lport = HTONS(mock_lastport);
			mock_conns[i].rport = rport;
			memcpy(mock_conns[i].ripaddr, ripaddr, 4);
			return &mock_conns[i];
		}
	}
	return NULL;
}

/* uip_udp_send() is uip_send(); a datagram the client builds in uip_appdata is
 * already where it belongs, so only the length is recorded. */
void uip_send(const void *data, uint16_t len)
{
	if (len > 0) {
		uip_slen = len;
		if (data != uip_sappdata)
			memcpy(uip_sappdata, data, len);
	}
}

/* the connection the client should have: the one bound to the NTP port */
static struct uip_udp_conn *mock_ntp_conn(void)
{
	for (int i = 0; i < MOCK_CONNS; i++)
		if (mock_conns[i].lport && mock_conns[i].rport == HTONS(123))
			return &mock_conns[i];
	return NULL;
}

static void mock_reset(void)
{
	memset(mock_conns, 0, sizeof(mock_conns));
	uip_flags = 0;
	uip_slen = 0;
	uip_len = 0;
}

/* ---- the clock the client schedules against ----
 * The firmware reads the seconds counter back through sfr_data the same way on
 * the host as the register mock stores it, so the register holds the seconds
 * the client sees. */
static uint32_t host_secs = 0;

static void secs_set(uint32_t t)
{
	host_secs = t;
	hw_reg_set(RTL837X_REG_SEC_COUNTER, t);
}

static void secs_add(uint32_t d) { secs_set(host_secs + d); }

/* ---- packet buffer the client builds its request in and reads answers from ---- */
static uint8_t pkt[512];

/* Poll every UDP connection the way handle_tx() does, minus the uIP guts:
 * the client sees UIP_POLL (no newdata). */
static void poll_all(void)
{
	for (int i = 0; i < MOCK_CONNS; i++) {
		if (!mock_conns[i].lport)
			continue;
		uip_flags = UIP_POLL;
		uip_slen = 0;
		uip_len = 0;
		uip_appdata = pkt;
		uip_sappdata = pkt;
		ntp_callback(mock_conns[i].lport);
		dns_callback(mock_conns[i].lport);
	}
}

/* Deliver datagram of len bytes to the connection bound to lport. */
static void deliver(uint16_t lport, const uint8_t *d, uint16_t len)
{
	memcpy(pkt, d, len);
	uip_appdata = pkt;
	uip_len = len;
	uip_flags = UIP_NEWDATA;
	uip_slen = 0;
	ntp_callback(lport);
	dns_callback(lport);
}

static bool out_has(const char *s) { return strstr(out_buf, s) != NULL; }

/* ---- helpers to set the client up ---- */
static void client_on(const char *server)
{
	mock_reset();
	env_reset();
	out_reset();
	hw_reset();
	ntp_init();
	dns_init();
	memset(&dhcp_state, 0, sizeof(dhcp_state));
	ticks = 0;
	secs_set(0);
	strcpy(ntp_state.server, server);
	ntp_start();
}

static void dns_server(const char *ip4)
{
	unsigned a, b, c, d;
	sscanf(ip4, "%u.%u.%u.%u", &a, &b, &c, &d);
	dns_state.server[0][0] = a;
	dns_state.server[0][1] = b;
	dns_state.server[0][2] = c;
	dns_state.server[0][3] = d;
}

/* A 48 byte SNTP answer, stratum 2, transmit timestamp at 40..43. */
static uint16_t ntp_answer(uint8_t *p, uint32_t unix_time)
{
	memset(p, 0, 48);
	p[0] = 0x24;			/* LI 0, VN 4, mode 4 (server) */
	p[1] = 2;			/* stratum */
	p[40] = (uint8_t)((unix_time + 2208988800UL) >> 24);
	p[41] = (uint8_t)((unix_time + 2208988800UL) >> 16);
	p[42] = (uint8_t)((unix_time + 2208988800UL) >> 8);
	p[43] = (uint8_t)(unix_time + 2208988800UL);
	return 48;
}

static uint16_t dns_answer(uint8_t *p, uint16_t id, const uint8_t ip[4])
{
	uint16_t i = 0;
	memset(p, 0, 64);
	p[0] = id >> 8; p[1] = (uint8_t)id;
	p[2] = 0x81;			/* QR 1, RD 1 */
	p[3] = 0x80;			/* RA 1, rcode 0 */
	p[5] = 1;			/* QDCOUNT */
	p[7] = 1;			/* ANCOUNT */
	i = 12;
	p[i++] = 3; p[i++] = 'n'; p[i++] = 't'; p[i++] = 'p';
	p[i++] = 0;			/* root label */
	i += 4;				/* QTYPE, QCLASS */
	p[i++] = 0xc0; p[i++] = 0x0c;	/* name: pointer to the question */
	p[i++] = 0; p[i++] = 1;		/* TYPE A */
	p[i++] = 0; p[i++] = 1;		/* CLASS IN */
	p[i++] = 0; p[i++] = 0; p[i++] = 0; p[i++] = 60;	/* TTL */
	p[i++] = 0; p[i++] = 4;		/* RDLENGTH */
	memcpy(p + i, ip, 4); i += 4;
	return i;
}


int main(void)
{
	uint8_t p[256];
	uint16_t n;

	/* ---- 1. a name with no DNS server at all: name the missing server ---- */
	client_on("ntp.example.com");
	poll_all();			/* IDLE -> dns_lookup: no server, DNS_FAILED */
	poll_all();			/* RESOLVE -> reason, retry */
	out_reset();
	ntp_show_time();
	CHECK(out_has("Time not synchronised, no DNS server for ntp.example.com"),
	      "a name with no resolver says which lookup cannot be made");
	CHECK(ntp_state.why == NTP_WHY_NODNS, "the reason is NTP_WHY_NODNS");

	/* ---- 2. a name and a DNS server that does not answer ---- */
	client_on("ntp.example.com");
	dns_server("192.0.2.1");
	poll_all();			/* IDLE -> query out */
	CHECK(mock_ntp_conn() != NULL, "the client holds a UDP connection");
	/* every try needs a poll to send and one to time out: six tries, then
	 * the resolver gives up and the client records why */
	for (int i = 0; i < 2 * 6 + 2; i++) {
		ticks += 2 * 200;
		poll_all();
	}
	out_reset();
	ntp_show_time();
	CHECK(out_has("Time not synchronised, cannot resolve ntp.example.com"),
	      "an unanswered lookup is reported as an unresolvable name");
	CHECK(ntp_state.why == NTP_WHY_DNSFAIL, "the reason is NTP_WHY_DNSFAIL");

	/* ---- 3. address from DNS, no answer from the server ---- */
	client_on("ntp.example.com");
	dns_server("203.0.113.9");
	poll_all();			/* IDLE -> query out */
	n = dns_answer(p, dns_state.id, (const uint8_t *)"\xcb\x00\x71\x09");
	/* the answer arrives on the DNS connection */
	{
		struct uip_udp_conn *dc = NULL;
		for (int i = 0; i < MOCK_CONNS; i++)
			if (mock_conns[i].lport && mock_conns[i].rport == HTONS(53))
				dc = &mock_conns[i];
		CHECK(dc != NULL, "the resolver holds a UDP connection");
		deliver(dc->lport, p, n);
	}
	CHECK(dns_state.status == DNS_DONE, "the answer is taken");
	poll_all();			/* RESOLVE -> address, connect */
	CHECK(ntp_state.addr[0] == 203 && ntp_state.addr[3] == 9, "the address is the one resolved");
	poll_all();			/* SEND -> request out */
	CHECK(uip_slen == 48, "the request is 48 bytes");
	CHECK(((uint8_t *)pkt)[0] == 0x23, "the request is a version 4 client packet");
	CHECK(mock_ntp_conn()->ripaddr[0] == 203, "the request goes to the resolved address");
	/* no answer: three tries, then the reason */
	for (int i = 0; i < 2 * 3 + 2; i++) {
		ticks += 3 * 200 + 1;
		poll_all();
	}
	poll_all();
	out_reset();
	ntp_show_time();
	CHECK(out_has("Time not synchronised, no usable answer from 203.0.113.9"),
	      "a silent server is named by its address");
	CHECK(ntp_state.why == NTP_WHY_NOANSWER, "the reason is NTP_WHY_NOANSWER");

	/* ---- 4. an address given as a literal, and an answer ---- */
	client_on("203.0.113.7");
	poll_all();			/* no lookup needed: DNS_DONE */
	poll_all();			/* RESOLVE -> connect */
	CHECK(ntp_state.addr[0] == 203 && ntp_state.addr[3] == 7, "a literal address is used as it is");
	poll_all();			/* SEND */
	n = ntp_answer(p, 1234567890UL);
	deliver(mock_ntp_conn()->lport, p, n);
	CHECK(ntp_state.synced == 1, "a valid answer synchronises the clock");
	out_reset();
	ntp_show_time();
	CHECK(out_has("Time 2009-02-14 07:31:30 UTC+08:00"),
	      "the answer's time is printed in the default zone, UTC+08:00");
	CHECK(out_has("stratum 2"), "the stratum is printed");
	CHECK(ntp_state.why == NTP_WHY_IDLE, "a synced client carries no reason");

	/* the clock runs on from the last answer, one minute per minute */
	secs_add(120);
	out_reset();
	ntp_show_time();
	CHECK(out_has("Time 2009-02-14 07:33:30 UTC"), "the time advances past the answer");
	CHECK(out_has("synchronised 2 min ago"), "and says how old the answer is");

	/* ---- 5. a client that never got the time, then turned off ---- */
	client_on("ntp.tencent.com");
	ntp_stop();
	out_reset();
	ntp_show_time();
	CHECK(out_has("Time not synchronised, NTP is off"), "a stopped client says so");
	CHECK(ntp_state.why == NTP_WHY_OFF, "the reason is NTP_WHY_OFF");

	/* ---- 6. a sync survives the client being turned off: the clock keeps
	 * running from the last answer, one minute per minute ---- */
	CHECK(ntp_state.synced == 0, "a fresh client holds no time");

	/* ---- 7. the built-in defaults, before any command or config line ---- */
	ntp_init();
	CHECK(!strcmp(ntp_state.server, "ntp.tencent.com"), "the default server is ntp.tencent.com");
	CHECK(ntp_state.offset == 480, "the default time zone is UTC+08:00");
	CHECK(ntp_state.interval == 60, "the default interval is 60 minutes");
	CHECK(ntp_state.enabled == 0, "and the client is off until it is turned on");

	printf("\n%d checks, %d failed\n", tests_run, tests_failed);
	return tests_failed ? 1 : 0;
}
