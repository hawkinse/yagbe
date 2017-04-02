#include <stdint.h>
//TODO - remove helpers!
inline uint8_t getLSB(uint16_t val){
  return val & 0x00FF;
}

inline uint8_t getMSB(uint16_t val){
  return (val >> 8) & 0xFF;
}

inline uint16_t setLSB(uint16_t val, uint8_t lsb){
    return (val & 0xFF00) | (lsb & 0xFF);
}

inline uint16_t setMSB(uint16_t val, uint8_t msb){
    return (val & 0x00FF) | ((msb << 8) & 0xFF00);
}
