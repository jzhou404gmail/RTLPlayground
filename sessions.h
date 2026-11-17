#ifndef _SESSIONS_H_
#define _SESSIONS_H_

#include <stdint.h>

/* Up to SESSIONS logins at once, each with its own cookie and its own
 * expiry. With a single one, logging in from a second browser (or a phone)
 * ended the first browser's session at its next request. */
#define SESSIONS 4

/* 32 hex characters = 128 bit. */
#define SESSION_ID_LENGTH 32

/* Absolute lifetime of a session, regardless of activity. An idle timeout
 * alone lets a cookie that is used often stay valid forever. */
#define SESSION_LIFETIME_MAX (8UL * 3600UL)

void session_init(void) __banked;
void session_drop(void) __banked;
void session_check(void) __banked;
void session_new(void) __banked;

#endif
