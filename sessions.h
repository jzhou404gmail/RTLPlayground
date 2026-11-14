#ifndef _SESSIONS_H_
#define _SESSIONS_H_

#include <stdint.h>

#define SESSIONS 4
#define SESSION_ID_LENGTH 12

extern __xdata char session_id[SESSIONS][SESSION_ID_LENGTH + 1];
extern __xdata uint32_t last_session_use[SESSIONS];

/* No session at boot */
/* Absolute lifetime of a session, regardless of activity: an idle timeout
 * alone lets a cookie that is used often stay valid forever. */
#define SESSION_LIFETIME_MAX (8UL * 3600UL)

void session_init(void) __banked;
/* End only the session the current request came with. */
void session_drop(void) __banked;
/* Sets authenticated if the cookie in session belongs to a live session */
void session_check(void) __banked;
/* Starts a session at now and writes its id to outbuf at slen */
void session_new(void) __banked;

#endif
