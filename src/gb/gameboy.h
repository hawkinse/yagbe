#pragma once
#include "gbz80cpu.h"
#include "gbmem.h"
#include "gbcart.h"
#include "gblcd.h"
#include "gbaudio.h"
#include "gbserial.h"

class Gameboy{
private:
    //Hardware objects
    GBCart* m_cartridge;
    GBMem* m_memory;
    GBLCD* m_lcd;
    GBAudio* m_apu;
    GBZ80* m_cpu;
    GBPad* m_pad;
    GBSerial* m_serial;
    
    //System information
    Platform systemType;
    
public:
    Gameboy(char* gamePath, char* bootromPath = NULL, Platform type = PLATFORM_AUTO);
    
    ~Gameboy();
    
    //Getters for hardware objects
    GBCart* getCartridge();
    GBMem* getMemory();
    GBLCD* getLCD();
    GBAudio* getAPU();
    GBZ80* getCPU();
    GBPad* getPad();
    GBSerial* getSerial();
};
