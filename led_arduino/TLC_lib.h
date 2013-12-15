#include <stdint.h>

void TLC_init(uint8_t dimming);
void TLC_write();
void TLC_setAll(uint16_t R, uint16_t G, uint16_t B);
void TLC_setData(uint16_t* data);
