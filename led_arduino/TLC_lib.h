#include <stdint.h>

void TLC_init(uint8_t dimming);
void TLC_write();
void TLC_setAll_16(uint16_t R, uint16_t G, uint16_t B);
void TLC_setData(uint8_t* data);
void TLC_setData(uint16_t* data);
void TLC_setData(uint8_t* data, uint16_t* vals);

