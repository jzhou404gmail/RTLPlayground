/*
 * Environment the httpd test does not exercise: the password record, the
 * configured host name and the tick counter. Compiled against the real
 * sessions.c and timer.c, so the login/session paths under test are the
 * firmware ones.
 */
#include <stdint.h>
#include <string.h>

#include "sdcc_shim.h"

/* The test requests carry "Host: sw", which has to match this. */
__xdata char hostname[32] = "sw";

__xdata uint8_t auth_set;
__xdata uint8_t auth_save_pending;
__xdata uint8_t auth_salt[16];
__xdata uint8_t auth_hash[32];

uint8_t auth_verify_hex(__xdata uint8_t *hex64)
{
	(void)hex64;
	return 0;
}

uint8_t auth_parse_hex(__xdata uint8_t *salt_hex, __xdata uint8_t *hash_hex)
{
	(void)salt_hex;
	(void)hash_hex;
	return 0;
}

uint8_t auth_persist(void)
{
	return 1;
}

void auth_clear(void)
{
	auth_set = 0;
}

void auth_send_info(void)
{
}

void auth_logout_reply(void)
{
}
