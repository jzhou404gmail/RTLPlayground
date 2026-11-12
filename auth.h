#ifndef _AUTH_H_
#define _AUTH_H_

#include <stdint.h>

/* The switch stores a salted SHA-256 digest of the admin password, never the
 * password itself. The digest is computed by the browser (Web Crypto is not
 * available on plain HTTP, so login.html/app.js carry a small JS SHA-256) or
 * by the host tooling and sent as 64 hex characters. */
extern __xdata uint8_t auth_salt[16];
extern __xdata uint8_t auth_hash[32];
extern __xdata uint8_t auth_set;

/* Set when a password change still has to be written to the startup config.
 * Deferred so it cannot clobber config_buf while a command body is parsed. */
extern __xdata uint8_t auth_save_pending;

/* Legacy plaintext record: only replayed from an old startup configuration so
 * a board can still be reached while it is migrated to a digest. */
extern __xdata char passwd[21];

uint8_t auth_parse_hex(const __xdata uint8_t *salt_hex, const __xdata uint8_t *hash_hex) __banked;
uint8_t auth_verify_hex(const __xdata uint8_t *hex64) __banked;
void auth_clear(void) __banked;

/* Rewrite the startup configuration with (or, after auth_clear(), without)
 * the passwd_hash line. Returns 1 when the read-back matches. */
uint8_t auth_persist(void) __banked;

/* GET /auth.json: the salt is public, the digest is not. */
void auth_send_info(void) __banked;

/* Clear the session and answer 302 to the login page with an expired cookie. */
void auth_logout_reply(void) __banked;

#endif
