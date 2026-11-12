/*
 * test_sessions.c - sessions.c against a clock the test moves by hand:
 * four slots, LRU eviction, the idle and the absolute deadline, and the
 * scope of a logout (one slot) versus a password change (all slots).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sdcc_shim.h"

#include "sessions.h"

/* Environment the firmware provides; the test owns it here. */
__xdata uint16_t session_timeout = 200;
__xdata uint32_t now;
__xdata uint8_t authenticated;
__xdata uint8_t * __xdata session;
__xdata uint8_t outbuf[256];
__xdata uint16_t slen;
volatile __xdata uint8_t sfr_data[4];

static int checks, failed;

static void check(int cond, const char *what)
{
	checks++;
	if (!cond) {
		failed++;
		printf("  FAIL  %s\n", what);
	}
}

static void ok(const char *what)
{
	printf("  ok:   %s\n", what);
	(void)what;
}

/* Deterministic "random" bytes so two logins differ but the test can repeat. */
/* A deterministic stream with a long period, so two logins never collide. */
static uint32_t rng = 0x12345678;
void get_random_32(void)
{
	for (int i = 0; i < 4; i++) {
		rng ^= rng << 13;
		rng ^= rng >> 17;
		rng ^= rng << 5;
		sfr_data[i] = (uint8_t)(rng >> 16);
	}
}

/* A login through the firmware path: session_new() writes the id into outbuf
 * right after the caller's prefix. */
static void login(char *id)
{
	/* a login POST arrives without a valid cookie: scan_header() leaves
	 * authenticated at 0 before the body runs */
	authenticated = 0;
	slen = 0;
	session_new();
	outbuf[slen] = 0;
	strcpy(id, (const char *)outbuf);
}

static int valid(const char *id)
{
	session = (__xdata uint8_t *)id;
	session_check();
	return authenticated;
}

int main(void)
{
	printf("== sessions ==\n");

	/* --- a fresh login authenticates, a wrong cookie does not --- */
	session_init();
	{
		char a[64];
		login(a);
		check(valid(a) == 1, "a fresh login authenticates");
		check(strlen(a) == SESSION_ID_LENGTH, "the id is 32 hex characters");
		char b[64];
		login(b);
		check(strcmp(a, b) != 0, "two logins get different ids");
		check(valid("00000000000000000000000000000000") == 0,
		      "an unknown cookie does not authenticate");
		check(valid("") == 0, "an empty cookie does not authenticate");

		/* the cookie value may be followed by ';' from a Cookie header */
		char with_end[80];
		snprintf(with_end, sizeof(with_end), "%s;", a);
		check(valid(with_end) == 1, "a cookie followed by ';' still matches");
		snprintf(with_end, sizeof(with_end), "%sx", a);
		check(valid(with_end) == 0, "a longer value does not match");
	}

	/* --- idle timeout, measured from the last request --- */
	session_init();
	{
		char a[64];
		now = 0;
		login(a);
		check(valid(a) == 1, "valid right after login");
		now = session_timeout;
		check(valid(a) == 1, "still valid at the idle deadline (inclusive)");
		now = session_timeout + 1;
		check(valid(a) == 1, "one second past it is kept by the request above");
		now = session_timeout * 3 + 1;
		check(valid(a) == 0, "idle past the deadline expires the session");
	}

	/* --- absolute deadline: activity does not extend it --- */
	session_init();
	{
		char a[64];
		session_timeout = 100;
		now = 0;
		login(a);
		uint32_t t;
		int alive = 1;
		for (t = 0; t <= SESSION_LIFETIME_MAX; t += 50) {
			now = t;
			if (!valid(a)) {
				alive = 0;
				break;
			}
		}
		check(alive == 1, "kept alive by requests until the absolute deadline");
		now = SESSION_LIFETIME_MAX + 1;
		check(valid(a) == 0, "absolutely expired after 8 h");
		session_timeout = 200;
	}

	/* --- four slots, the fifth login evicts the least recently used --- */
	session_init();
	{
		char id[5][64];
		now = 0;
		for (int i = 0; i < 4; i++) {
			now += 10;
			login(id[i]);
			check(valid(id[i]) == 1, "each of four logins is valid");
		}
		/* touch slot 0 so slot 1 becomes the least recently used */
		now += 10;
		check(valid(id[0]) == 1, "slot 0 used");
		now += 10;
		char fifth[64];
		login(fifth);
		check(valid(fifth) == 1, "the fifth login is valid");
		check(valid(id[1]) == 0, "the least recently used slot was evicted");
		check(valid(id[0]) == 1, "a recently used session survives");
		check(valid(id[2]) == 1, "the other sessions survive");
		check(valid(id[3]) == 1, "the other sessions survive");
	}

	/* --- an expired slot is reused before a live one is evicted --- */
	session_init();
	{
		char a[64], b[64], c[64], d[64], e[64];
		now = 0;
		login(a); login(b); login(c); login(d);
		now = session_timeout + 1;	/* a and b are now idle */
		login(e);
		check(valid(a) == 0, "an expired slot was reused");
		check(valid(e) == 1, "the new login owns it");
	}

	/* --- logout clears only the calling browser --- */
	session_init();
	{
		char a[64], b[64];
		now = 0;
		login(a);
		login(b);
		check(valid(a) == 1 && valid(b) == 1, "both sessions valid");
		check(valid(b) == 1, "logout is called on B's request");
		session_drop();
		check(valid(b) == 0, "B is logged out");
		check(valid(a) == 1, "A is still logged in");
	}

	/* --- a password change ends every session --- */
	session_init();
	{
		char a[64], b[64];
		now = 0;
		login(a);
		login(b);
		session_init();
		check(valid(a) == 0 && valid(b) == 0, "session_init ends every slot");
	}

	/* --- the 16-bit clock wraps --- */
	session_init();
	{
		char a[64];
		now = 0xffff - 5;
		login(a);
		now = 0xffff;
		check(valid(a) == 1, "valid across the clock wrap");
		now = (uint32_t)0xffff + session_timeout / 2;
		check(valid(a) == 1, "the idle deadline counts across the wrap");
	}

	printf("\n%d checks, %d failed\n", checks, failed);
	if (!failed)
		ok("sessions");
	return failed ? 1 : 0;
}
