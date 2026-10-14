#ifndef _NTP_H_
#define _NTP_H_

#include <stdint.h>
#include "dns.h"

#define NTP_DST_OFF	0
#define NTP_DST_EU	1
#define NTP_DST_US	2

/* Why there is no time yet, so "time" and the web UI can say more than
 * "not synchronised". Set on every attempt, so it always names the last
 * outcome. */
#define NTP_WHY_OFF	0	/* the client is turned off */
#define NTP_WHY_IDLE	1	/* turned on, no answer to the first attempt yet */
#define NTP_WHY_NODNS	2	/* the server is a name and no DNS server is set */
#define NTP_WHY_DNSFAIL	3	/* the DNS server did not answer for the name */
#define NTP_WHY_NOANSWER 4	/* the request went out, no usable answer came back */

struct uip_udp_conn;

struct ntp_state {
	uint8_t enabled;
	char server[DNS_NAME_LEN];	/* host name or dotted address */
	uint8_t addr[4];	/* address of the server last used */
	uint16_t interval;	/* minutes between two synchronisations */
	int16_t offset;		/* time zone, minutes east of UTC */
	uint8_t dst;		/* NTP_DST_* */
	uint8_t phase;
	uint8_t tries;
	uint8_t synced;
	uint8_t why;		/* NTP_WHY_* */
	uint8_t stratum;
	uint32_t utc;		/* seconds since 1970 UTC at the last synchronisation */
	uint32_t at;		/* uptime seconds at the last synchronisation */
	uint32_t next;		/* uptime seconds of the next synchronisation */
	uint32_t sent;		/* ticks when the request went out */
	struct uip_udp_conn *conn;
};

extern __xdata struct ntp_state ntp_state;

void ntp_init(void) __banked;
void ntp_start(void) __banked;
void ntp_stop(void) __banked;
void ntp_show(void) __banked __reentrant;
void ntp_show_time(void) __banked __reentrant;
void ntp_callback(uint16_t lport) __banked __reentrant;
void ntp_parse(void) __banked __reentrant;

#endif
