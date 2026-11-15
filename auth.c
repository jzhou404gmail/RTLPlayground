/*
 * Salted password record without on-device hashing: the digest arrives from
 * the browser or the host tooling, the 8051 only stores and compares it.
 *
 * This code is in the Public Domain.
 */

#include <stdint.h>
#include "rtl837x_common.h"
#include "rtl837x_flash.h"
#include "page_impl.h"
#include "auth.h"
#include "sessions.h"

#pragma codeseg BANK3
#pragma constseg BANK3

extern __xdata uint8_t sfr_data[4];
extern __xdata uint8_t config_buf[];
extern __xdata uint8_t flash_buf[FLASH_BUF_SIZE];
extern __xdata struct flash_region_t flash_region;
extern __xdata uint8_t outbuf[];
extern __xdata uint16_t slen;
extern __xdata uint8_t authenticated;
extern __xdata uint8_t http_tls;

__xdata uint8_t auth_salt[16];
__xdata uint8_t auth_hash[32];
__xdata uint8_t auth_set;
__xdata uint8_t auth_save_pending;

static __xdata uint8_t a_i, a_hi, a_lo, a_diff;
static __xdata uint16_t a_j;
static __xdata uint8_t auth_line[112];
/* Decode into scratch first: a malformed passwd_hash must not overwrite the
 * live salt/hash before it is known to be valid, or a typo would lock the
 * operator out until the next boot. */
static __xdata uint8_t auth_salt_tmp[16];
static __xdata uint8_t auth_hash_tmp[32];

__code const uint8_t auth_hexchars[17] = "0123456789abcdef";
__code const char auth_json_hdr[] =
	"HTTP/1.1 200 OK\r\nConnection: close\r\nCache-Control: no-store\r\n"
	"X-Content-Type-Options: nosniff\r\nContent-Type: application/json\r\n\r\n"
	"{\"set\":";

static uint8_t hex_val(uint8_t c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0xff;
}

static uint8_t hex_decode(const __xdata uint8_t *src, __xdata uint8_t *dst, uint8_t n)
{
	for (a_i = 0; a_i < n; a_i++) {
		a_hi = hex_val(src[a_i * 2]);
		a_lo = hex_val(src[a_i * 2 + 1]);
		if (a_hi == 0xff || a_lo == 0xff)
			return 0;
		dst[a_i] = (a_hi << 4) | a_lo;
	}
	return 1;
}

uint8_t auth_parse_hex(const __xdata uint8_t *salt_hex, const __xdata uint8_t *hash_hex) __banked
{
	if (!hex_decode(salt_hex, auth_salt_tmp, 16)
	    || !hex_decode(hash_hex, auth_hash_tmp, 32))
		return 0;
	for (a_i = 0; a_i < 16; a_i++)
		auth_salt[a_i] = auth_salt_tmp[a_i];
	for (a_i = 0; a_i < 32; a_i++)
		auth_hash[a_i] = auth_hash_tmp[a_i];
	auth_set = 1;
	return 1;
}

uint8_t auth_verify_hex(const __xdata uint8_t *hex64) __banked
{
	if (!auth_set)
		return 0;

	a_diff = 0;
	for (a_i = 0; a_i < 32; a_i++) {
		a_hi = hex_val(hex64[a_i * 2]);
		a_lo = hex_val(hex64[a_i * 2 + 1]);
		if (a_hi == 0xff || a_lo == 0xff)
			return 0;
		a_diff |= ((a_hi << 4) | a_lo) ^ auth_hash[a_i];
	}
	return a_diff == 0;
}

void auth_clear(void) __banked
{
	for (a_i = 0; a_i < 16; a_i++)
		auth_salt[a_i] = 0;
	for (a_i = 0; a_i < 32; a_i++)
		auth_hash[a_i] = 0;
	auth_set = 0;
	passwd[0] = 0;
}

static uint8_t line_is_auth(const __xdata uint8_t *p, uint16_t len)
{
	static __code const uint8_t pw[] = "passwd";
	static __code const uint8_t ph[] = "passwd_hash";

	/* A bare "passwd" disables web management, so it has to be dropped
	 * along with "passwd <text>". "passwd_hash ..." is the digest record
	 * and is rewritten from RAM at the end of the configuration. */
	if (len >= 6) {
		for (a_i = 0; a_i < 6; a_i++)
			if (p[a_i] != pw[a_i])
				break;
		if (a_i == 6 && (len == 6 || p[6] == ' '))
			return 1;
	}
	if (len >= 11) {
		for (a_i = 0; a_i < 11; a_i++)
			if (p[a_i] != ph[a_i])
				break;
		if (a_i == 11 && (len == 11 || p[11] == ' '))
			return 1;
	}
	return 0;
}

uint8_t auth_persist(void) __banked
{
	static __xdata uint16_t valid, r, w, ls, le, i, n;
	static __xdata uint8_t ok;

	valid = 0;
	n = 0;
	ok = 1;
	for (i = 0; i < CONFIG_LEN; i += FLASH_BUF_SIZE) {
		flash_region.addr = CONFIG_START + i;
		flash_region.len = FLASH_BUF_SIZE;
		flash_read_bulk(config_buf + i);
	}

	while (valid < CONFIG_LEN && config_buf[valid])
		valid++;
	/* No terminator in the whole sector: the read may be corrupt or the
	 * configuration may fill it. Rewriting it from this buffer would replace
	 * the user's settings with only the auth line, so do not touch flash. */
	if (valid >= CONFIG_LEN)
		return 0;

	if (auth_set) {
		static __code const uint8_t prefix[] = "passwd_hash ";
		for (i = 0; i < 12; i++)
			auth_line[i] = prefix[i];
		for (i = 0; i < 16; i++) {
			auth_line[12 + i * 2] = auth_hexchars[auth_salt[i] >> 4];
			auth_line[13 + i * 2] = auth_hexchars[auth_salt[i] & 0x0f];
		}
		auth_line[44] = ' ';
		for (i = 0; i < 32; i++) {
			auth_line[45 + i * 2] = auth_hexchars[auth_hash[i] >> 4];
			auth_line[46 + i * 2] = auth_hexchars[auth_hash[i] & 0x0f];
		}
		auth_line[109] = '\n';
		n = 110;
	}

	r = 0;
	w = 0;
	while (r < valid) {
		ls = r;
		while (r < valid && config_buf[r] != '\n')
			r++;
		le = r;
		if (r < valid)
			r++;

		if (line_is_auth(config_buf + ls, le - ls))
			continue;

		if (w && config_buf[w - 1] != '\n')
			config_buf[w++] = '\n';
		for (i = ls; i < le; i++)
			config_buf[w++] = config_buf[i];
		if (w && config_buf[w - 1] != '\n')
			config_buf[w++] = '\n';
	}

	if (auth_set) {
		if (w + n + 1 >= CONFIG_LEN)
			return 0;
		for (i = 0; i < n; i++)
			config_buf[w++] = auth_line[i];
	}
	config_buf[w] = 0;

	flash_region.addr = CONFIG_START;
	flash_sector_erase();
	flash_region.addr = CONFIG_START;
	flash_region.len = w + 1;
	flash_write_bytes(config_buf);

	for (i = 0; i < w + 1; i += FLASH_BUF_SIZE) {
		__xdata uint16_t len = w + 1 - i;

		if (len > FLASH_BUF_SIZE)
			len = FLASH_BUF_SIZE;
		flash_region.addr = CONFIG_START + i;
		flash_region.len = len;
		flash_read_bulk(flash_buf);
		for (a_j = 0; a_j < len; a_j++)
			if (flash_buf[a_j] != config_buf[i + a_j])
				ok = 0;
	}
	return ok;
}

void auth_send_info(void) __banked
{
	slen = strtox(outbuf, auth_json_hdr);
	slen += strtox(outbuf + slen, auth_set ? "1" : "0");
	/* An old plaintext "passwd" line is still accepted by /login while no
	 * digest exists. Tell login.html so it posts the plaintext once instead
	 * of hashing a password the firmware cannot verify. */
	slen += strtox(outbuf + slen, ",\"legacy\":");
	slen += strtox(outbuf + slen, (!auth_set && passwd[0]) ? "1" : "0");
	slen += strtox(outbuf + slen, ",\"salt\":\"");
	if (auth_set) {
		for (a_i = 0; a_i < 16; a_i++) {
			outbuf[slen++] = auth_hexchars[auth_salt[a_i] >> 4];
			outbuf[slen++] = auth_hexchars[auth_salt[a_i] & 0x0f];
		}
	}
	slen += strtox(outbuf + slen, "\"}");
}

void auth_logout_reply(void) __banked
{
	/* Only this browser's slot: the other sessions stay logged in. */
	session_drop();
	authenticated = 0;
	slen = strtox(outbuf, "HTTP/1.1 302 Found\r\nConnection: close\r\n"
			     "Cache-Control: no-store\r\nLocation: login.html\r\n"
			     "Set-Cookie: session=; Max-Age=0; Path=/; HttpOnly; SameSite=Strict");
	if (http_tls)
		slen += strtox(outbuf + slen, "; Secure");
	slen += strtox(outbuf + slen, "\r\n\r\n");
}
