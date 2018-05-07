#include "gameboy.h"

Gameboy::Gameboy(char* gamePath, char* bootromPath, Platform type){
    systemType = type;
    
    m_cartridge = new GBCart(gamePath, bootromPath);
    m_memory = new GBMem(systemType);
    m_memory->loadCart(m_cartridge);
    m_lcd = new GBLCD(m_memory);
    m_memory->setLCD(m_lcd);
    m_apu = new GBAudio(m_memory);
    m_memory->setAudio(m_apu);
    m_cpu = new GBZ80(m_memory, m_lcd, m_apu);
    m_pad = new GBPad(m_memory);
    m_memory->setPad(m_pad);
    m_serial = new GBSerial(m_memory);
    m_memory->setSerial(m_serial);
    
    m_cartridge->printCartInfo();
}

Gameboy::~Gameboy(){
    delete m_pad;
    delete m_cpu;
    delete m_apu;
    delete m_lcd;
    delete m_memory;
    delete m_cartridge;
    delete m_serial;
}

GBCart* Gameboy::getCartridge(){
    return m_cartridge;
}

GBMem* Gameboy::getMemory(){
    return m_memory;
}

GBLCD* Gameboy::getLCD(){
    return m_lcd;
}

GBAudio* Gameboy::getAPU(){
    return m_apu;
}

GBZ80* Gameboy::getCPU(){
    return m_cpu;
}

GBPad* Gameboy::getPad(){
    return m_pad;
}

GBSerial* Gameboy::getSerial(){
    return m_serial;
}
