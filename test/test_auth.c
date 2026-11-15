/*
 * test_auth.c - auth.c against a simulated configuration sector.
 *
 * The firmware stores a salted SHA-256 digest as "passwd_hash <salt> <digest>"
 * and keeps the plaintext "passwd <text>" line only as a migration fallback.
 * This bench checks the parts that run on the 8051: hex parsing, the
 * constant-time compare, the JSON the login page reads, and the configuration
 * write-back (strip old auth lines, append the digest, read back and verify).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sdcc_shim.h"

#include "auth.h"
#include "rtl837x_common.h"
#include "rtl837x_flash.h"

#define FLASH_SIZE 0x80000

static int checks, failed;

static void check(int cond, const char *what)
{
	checks++;
	if (!cond) {
		failed++;
		printf("  FAIL  %s\n", what);
	}
}

/* ---- firmware environment ------------------------------------------------ */

__xdata uint8_t config_buf[CONFIG_LEN];
__xdata uint8_t flash_buf[FLASH_BUF_SIZE];
__xdata struct flash_region_t flash_region;
__xdata uint8_t outbuf[1024];
__xdata uint16_t slen;
__xdata char passwd[21];
__xdata uint8_t authenticated;
__xdata uint8_t http_tls;

static uint8_t flash_mem[FLASH_SIZE];
static int drop_calls;

uint16_t strtox(uint8_t *dst, const char *s)
{
	uint16_t n = 0;

	while (s[n]) {
		dst[n] = (uint8_t)s[n];
		n++;
	}
	return n;
}

void flash_read_bulk(uint8_t *dst)
{
	memcpy(dst, flash_mem + flash_region.addr, flash_region.len);
}

void flash_sector_erase(void)
{
	memset(flash_mem + flash_region.addr, 0xff, FLASH_SECTOR_SIZE);
}

void flash_write_bytes(uint8_t *ptr)
{
	memcpy(flash_mem + flash_region.addr, ptr, flash_region.len);
}

/* auth.c's logout reply calls this; the real one also clears the cookie slot,
 * which is covered by test_sessions.c. */
void session_drop(void)
{
	drop_calls++;
}

/* ---- helpers ------------------------------------------------------------- */

static void program_config(const char *text)
{
	memset(flash_mem, 0xff, sizeof(flash_mem));
	memcpy(flash_mem + CONFIG_START, text, strlen(text) + 1);
}

static const char *stored_config(void)
{
	return (const char *)(flash_mem + CONFIG_START);
}

static void send_info(char *dst, size_t n)
{
	auth_send_info();
	if (slen >= n)
		slen = (uint16_t)(n - 1);
	memcpy(dst, outbuf, slen);
	dst[slen] = 0;
}

static void set_digest(void)
{
	static uint8_t salt_hex[] = "000102030405060708090a0b0c0d0e0f";
	static uint8_t hash_hex[] =
		"99c90773ea9ce66a355bd6a2a6e55122c3caf5a164785c90b1d58c2d63bb8680";

	(void)auth_parse_hex(salt_hex, hash_hex);
}

int main(void)
{
	char json[512];

	printf("== auth ==\n");

	/* --- a fresh record is unprovisioned and not legacy --- */
	auth_clear();
	strcpy(passwd, "");
	send_info(json, sizeof(json));
	{
		const char *body = strstr(json, "\r\n\r\n");

		check(body && !strcmp(body + 4, "{\"set\":0,\"legacy\":0,\"salt\":\"\"}"),
		      "fresh auth.json has set=0, legacy=0 and an empty salt");
	}

	/* --- an old plaintext line is advertised as the fallback --- */
	strcpy(passwd, "1234");
	send_info(json, sizeof(json));
	check(strstr(json, "\"set\":0") != NULL, "legacy record: set=0");
	check(strstr(json, "\"legacy\":1") != NULL, "legacy record: legacy=1");
	strcpy(passwd, "");

	/* --- digest parse, compare, and the salt in the JSON --- */
	set_digest();
	check(auth_set == 1, "parse sets auth_set");
	send_info(json, sizeof(json));
	check(strstr(json, "\"set\":1") != NULL, "digest record: set=1");
	check(strstr(json, "\"legacy\":0") != NULL, "digest record: legacy=0");
	check(strstr(json, "\"salt\":\"000102030405060708090a0b0c0d0e0f\"") != NULL,
	      "digest record: the salt is exposed for the browser");

	{
		static uint8_t good[] =
			"99c90773ea9ce66a355bd6a2a6e55122c3caf5a164785c90b1d58c2d63bb8680";
		static uint8_t bad[] =
			"99c90773ea9ce66a355bd6a2a6e55122c3caf5a164785c90b1d58c2d63bb8681";
		static uint8_t not_hex[] =
			"zzc90773ea9ce66a355bd6a2a6e55122c3caf5a164785c90b1d58c2d63bb8680";

		check(auth_verify_hex(good) == 1, "the stored digest verifies");
		check(auth_verify_hex(bad) == 0, "a one-bit change is rejected");
		check(auth_verify_hex(not_hex) == 0, "non-hex input is rejected");

		/* A failed parse must not half-overwrite the live credential. */
		{
			static uint8_t other_salt[] =
				"ffffffffffffffffffffffffffffffff";

			check(auth_parse_hex(other_salt, not_hex) == 0,
			      "a malformed digest is rejected");
			check(auth_set == 1, "the existing record stays provisioned");
			check(auth_verify_hex(good) == 1,
			      "a malformed digest leaves the old hash intact");
		}
	}

	/* --- persisting a digest strips passwd/passwd_hash, keeps the rest --- */
	program_config("ip dhcp\npasswd 1234\nhostname sw\n");
	check(auth_persist() == 1, "auth_persist reports success");
	{
		const char *cfg = stored_config();

		check(strstr(cfg, "ip dhcp\n") == cfg, "the other settings come first");
		check(strstr(cfg, "hostname sw\n") != NULL, "the unrelated line survives");
		check(strstr(cfg, "passwd 1234") == NULL, "the old plaintext line is gone");
		check(strstr(cfg, "passwd_hash 000102030405060708090a0b0c0d0e0f "
				  "99c90773ea9ce66a355bd6a2a6e55122c3caf5a164785c90b1d58c2d63bb8680\n")
		      != NULL, "the digest line is appended in the firmware format");
	}

	/* --- a bare "passwd" line is stripped as well --- */
	program_config("ip dhcp\npasswd\nhostname sw\n");
	set_digest();
	check(auth_persist() == 1, "persist with a bare passwd line");
	check(strstr(stored_config(), "\npasswd\n") == NULL,
	      "a bare passwd line is removed");
	check(strstr(stored_config(), "passwd_hash ") != NULL,
	      "the digest still lands in the file");

	/* --- clearing leaves no auth line behind --- */
	auth_clear();
	check(auth_persist() == 1, "persist after clearing");
	check(strstr(stored_config(), "passwd") == NULL,
	      "no passwd/passwd_hash line remains");

	/* --- a full sector without a NUL is not rewritten --- */
	program_config("ip dhcp\n");
	memset(flash_mem + CONFIG_START, 'A', CONFIG_LEN);
	set_digest();
	check(auth_persist() == 0, "a sector with no terminator is refused");
	check(flash_mem[CONFIG_START] == 'A',
	      "the untouched configuration is still there");

	/* --- logout: expired cookie, session dropped --- */
	auth_clear();
	program_config("ip dhcp\n");
	strcpy(passwd, "");
	authenticated = 1;
	http_tls = 0;
	drop_calls = 0;
	slen = 0;
	auth_logout_reply();
	outbuf[slen] = 0;
	check(strstr((const char *)outbuf, "HTTP/1.1 302 Found") != NULL,
	      "logout answers 302");
	check(strstr((const char *)outbuf, "Set-Cookie: session=; Max-Age=0") != NULL,
	      "logout expires the cookie");
	check(strstr((const char *)outbuf, "HttpOnly") != NULL &&
	      strstr((const char *)outbuf, "SameSite=Strict") != NULL,
	      "the expired cookie keeps its attributes");
	check(drop_calls == 1, "logout drops the calling session");
	check(authenticated == 0, "logout clears the request's auth flag");

	printf("\n%d checks, %d failed\n", checks, failed);
	if (!failed)
		printf("  ok:   auth\n");
	return failed ? 1 : 0;
}
