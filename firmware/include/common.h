/* common.h — shared types and the NOT_IMPLEMENTED stub marker.
 * Units used throughout (D-010):
 *   time  : microseconds (us); Timer1 tick = 1 us (8 MHz / 8)
 *   angle : centidegrees of CRANKSHAFT rotation (cdeg), 100 cdeg = 1 deg
 *   speed : crankshaft rpm
 *   volts : millivolts (mV)
 * Angles for timing are "degrees Before Top Dead Centre" (BTDC) of the
 * compression stroke; positive = before TDC. */
#ifndef ARIEL_COMMON_H
#define ARIEL_COMMON_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef HOST_BUILD
#include <stdio.h>
#define NOT_IMPLEMENTED() fprintf(stderr, "NOT IMPLEMENTED: %s\n", __func__)
#else
#define NOT_IMPLEMENTED() do { } while (0)
#endif

#endif
