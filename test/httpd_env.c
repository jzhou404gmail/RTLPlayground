/*
 * Environment the httpd test does not exercise: the configured host name.
 *
 * The password record is no longer stubbed: auth.c is compiled into the test
 * unmodified, and test_httpd_tx.c carries a simulated configuration sector so
 * the first-run setup, digest login, legacy login and logout paths run against
 * the real auth code. sessions.c and timer.c are real as well.
 */
#include <stdint.h>
#include <string.h>

#include "sdcc_shim.h"

/* The test requests carry "Host: sw", which has to match this. */
__xdata char hostname[32] = "sw";
