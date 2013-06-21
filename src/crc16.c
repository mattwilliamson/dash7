#include "crc16.h"

uint16_t crc16_calc(uint16_t old_crc, uint8_t data)
{
    uint16_t crc;
    uint16_t x;

    x = ((old_crc >> 8) ^ data) & 0xFF;
    x ^= x >> 4;
    crc = (old_crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
    crc &= 0xFFFF;

    return crc;
}

/* 
Original Code: Ashley Roll 
Optimisations: Scott Dattalo 
*/
