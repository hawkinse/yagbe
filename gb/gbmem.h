#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gbcart.h"
#include "gblcd.h"
#include "gbaudio.h"
#include "gbpad.h"
#include "gbserial.h"
#include "../constants.h"

//RAM regions
#define ROM_BANK_0_START  0x0000
#define ROM_BANK_0_END    0x3FFF
#define ROM_BANK_N_START  0x4000
#define ROM_BANK_N_END    0x7FFF
#define VRAM_START        0x8000
#define VRAM_END          0x9FFF
#define EXTRAM_START      0xA000
#define EXTRAM_END        0xBFFF
#define WRAM_BANK_0_START 0xC000
#define WRAM_BANK_0_END   0xCFFF
#define WRAM_BANK_1_START 0xD000
#define WRAM_BANK_1_END   0xDFFF
#define ECHO_RAM_START    0xE000
#define ECHO_RAM_END      0xFDFF
#define OAM_START         0xFE00
#define OAM_END           0xFE9F
#define UNUSABLE_START    0xFEA0
#define UNUSABLE_END      0xFEFF
#define IO_START          0xFF00
#define IO_END            0xFF7F
#define HRAM_START        0xFF80
#define HRAM_END          0xFFFE
#define INTERRUPT_ENABLE  0xFFFF

//Notable addresses
#define ADDRESS_LCDC   0xFF40
#define ADDRESS_STAT   0xFF41
#define ADDRESS_SCY    0xFF42
#define ADDRESS_SCX    0xFF43
#define ADDRESS_LY     0xFF44
#define ADDRESS_LYC    0xFF45
#define ADDRESS_WY     0xFF4A
#define ADDRESS_WX     0xFF4B
#define ADDRESS_BGP    0xFF47 //Non-GBC only
#define ADDRESS_OBP0   0xFF48 //Non-GBC only
#define ADDRESS_OBP1   0xFF49 //Non-GBC only
#define ADDRESS_BOOTROM 0xFF50
#define ADDRESS_BCPS   0xFF68 //GBC only
#define ADDRESS_BCPD   0xFF69 //GBC only
#define ADDRESS_OCPS   0xFF6A //GBC only
#define ADDRESS_VBK    0xFF4F
#define ADDRESS_DMA    0xFF46
#define ADDRESS_HDMA1  0xFF51 //GBC only
#define ADDRESS_HDMA2  0xFF52 //GBC only
#define ADDRESS_HDMA3  0xFF53 //GBC only
#define ADDRESS_HDMA4  0xFF54 //GBC only
#define ADDRESS_HDMA5  0xFF55 //GBC only

//Square Wave 1 Channel Addresses
#define ADDRESS_NR10 0xFF10
#define ADDRESS_NR11 0xFF11
#define ADDRESS_NR12 0xFF12
#define ADDRESS_NR13 0xFF13
#define ADDRESS_NR14 0xFF14

//Square Wave 2 Channel Addresses
#define ADDRESS_NR21 0xFF16
#define ADDRESS_NR22 0xFF17
#define ADDRESS_NR23 0xFF18
#define ADDRESS_NR24 0xFF19

//Wave Table Channel Addresses
#define ADDRESS_NR30 0xFF1A
#define ADDRESS_NR31 0xFF1B
#define ADDRESS_NR32 0xFF1C
#define ADDRESS_NR33 0xFF1D
#define ADDRESS_NR34 0xFF1E

//Noise Channel Addresses
#define ADDRESS_NR41 0xFF20
#define ADDRESS_NR42 0xFF21
#define ADDRESS_NR43 0xFF22
#define ADDRESS_NR44 0xFF23

//Audio Control and Status Addresses
#define ADDRESS_NR50 0xFF24
#define ADDRESS_NR51 0xFF25
#define ADDRESS_NR52 0xFF26

//Wave Table Data
#define ADDRESS_WAVE_TABLE_DATA_START 0xFF30
#define ADDRESS_WAVE_TABLE_DATA_END   0xFF3F

#define ADDRESS_JOYP   0xFF00
#define ADDRESS_SERIAL_DATA 0xFF01
#define ADDRESS_SERIAL_CONTROL 0xFF02
#define ADDRESS_DIV    0xFF04
#define ADDRESS_TIMA   0xFF05
#define ADDRESS_TMA    0xFF06
#define ADDRESS_TAC    0xFF07
#define ADDRESS_IF     0xFF0F

#define ADDRESS_KEY1   0xFF4D //GBC only, switch speed
//IR register skipped
#define ADDRESS_SVBK   0xFF70 //GBC WRAM bank

#define DIV_INCREMENT_CLOCK 16384 //Increment speed of DIV register
//TIMA Increment speeds in Hz. 
#define TAC_CLOCK_4096   0x00
#define TAC_CLOCK_262144 0x01
#define TAC_CLOCK_65536  0x02
#define TAC_CLOCK_16384  0x03

#define INTERRUPT_FLAG_VBLANK 0x1 //Bit 0
#define INTERRUPT_FLAG_STAT   0x2 //Bit 1
#define INTERRUPT_FLAG_TIMER  0x4 //Bit 2
#define INTERRUPT_FLAG_SERIAL 0x8 //Bit 3
#define INTERRUPT_FLAG_JOYPAD 0x10 //Bit 4


class GBMem{
  private:
    GBCart* m_gbcart;
    GBLCD* m_gblcd;
    GBAudio* m_gbaudio;
    GBPad* m_gbpad;
    GBSerial* m_gbserial;
    
    bool m_bVRamBank;
    uint8_t m_ExtRamBank; //TODO - move to cart?
    uint8_t m_WRamBank;
    
    //Timer and Divider "registers"
    uint8_t m_RegisterDIV;
    uint8_t m_RegisterTIMA;
    uint8_t m_RegisterTMA;
    uint8_t m_RegisterTAC;
    
    //TODO - split array into separate arrays for each section of memory!
    uint8_t m_mem[0xFFFF]; //Entire memory map.
    uint8_t* m_extRamExtraBanks; //Stores all banks of extram
    uint8_t* m_wRamBanks; //Only stores banks 1-7, since bank 0 is constant 
    
    void switch_bank_vram();
    void switch_bank_extram(uint8_t bank);
    void switch_bank_wram(uint8_t bank);
    
    //Special read/write funcs go here
    void write_unusable(uint16_t address);
    void read_unusable(uint16_t address);
    
    void increment_RegisterDIV(long long hz);
    void increment_RegisterTIMA(long long hz);
    
  public:
    GBMem();
    ~GBMem();
    
    void write(uint16_t address, uint8_t value);
    uint8_t read(uint16_t address);
    
    //Direct read and write, to bypass logic for hardware reads and writes
    //Ex. to better implement display
    void direct_write(uint16_t address, uint8_t value);
    uint8_t direct_read(uint16_t address);
    
    void loadCart(GBCart* cart);
    void setLCD(GBLCD* lcd);
    void setAudio(GBAudio* audio);
    void setPad(GBPad* pad);
    void setSerial(GBSerial* serial);
    
    //Get whether or not we are reading from boot rom instead of cartridge
    bool getBootRomEnabled();
    
    //Used to update timer registers
    void tick(long long hz);
};
