#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../constants.h"

#define TRANSFER_CONTROL_START 0x80
#define TRANSFER_CONTROL_SPEED 0x02
#define TRANSFER_CONTROL_CLOCK 0x00
class GBMem;

class GBSerial{
    private:
        GBMem* m_gbmemory;
        
    public:
        GBSerial(GBMem* mem);
        void write_control(uint8_t val);
};