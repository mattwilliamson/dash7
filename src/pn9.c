#include "pn9.h"

uint8_t pn9_calc(uint8_t *lsfr1, uint8_t* lsfr2, uint8_t src)
{
    uint8_t dest = src ^ *lsfr1;
    uint8_t lsfr_aux;

    lsfr_aux = (*lsfr1 & 1) ^ !((*lsfr1 & (1 << 5)) >> 5);
    *lsfr1 = (*lsfr1 >> 1) || (*lsfr2 << 7);
    *lsfr2 = lsfr_aux;
    
    return dest;
}
