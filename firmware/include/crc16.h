/* crc16.h — CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection,
 * xorout 0x0000). Check value for ASCII "123456789" is 0x29B1. (D-011) */
#ifndef ARIEL_CRC16_H
#define ARIEL_CRC16_H
#include "common.h"
uint16_t crc16_ccitt(const uint8_t *data, size_t len);
#endif
