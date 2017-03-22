#pragma once
#include <stdlib.h>
#include <string>
#include <stdint.h>
#include "../constants.h"

#define ADDRESS_CART_NINTENDO_START 0x0104 //Scrolling nintendo logo
#define ADDRESS_CART_NINTENDO_END   0x0133
#define ADDRESS_CART_TITLE_START    0x0134
#define ADDRESS_CART_TITLE_END      0x0142
#define ADDRESS_CART_PLATFORM       0x0143 //0x80 for GBC, 0x00 for normal GB
#define ADDRESS_CART_LICENSEE_HI    0x0144
#define ADDRESS_CART_LICENSEE_LO    0x0145
#define ADDRESS_CART_SGB            0x0146 //0x03 if super gameboy enhanced
#define ADDRESS_CART_TYPE           0x0147
#define ADDRESS_CART_ROM_SIZE       0x0148
#define ADDRESS_CART_RAM_SIZE       0x0149
#define ADDRESS_CART_REGION         0x014A //0 for Japanese, 1 for other
#define ADDRESS_CART_LICENSEE_OLD   0x014B
#define ADDRESS_CART_MASK_ROM_VER   0x014C
#define ADDRESS_CART_COMPLEMENT_CHK 0x014D
#define ADDRESS_CART_CHECKSUM_START 0x014E
#define ADDRESS_CART_CHECKSUM_END   0x014F

#define ADDRESS_CART_RAM_ENABLE_START   0x0000
#define ADDRESS_CART_RAM_ENABLE_END     0x1FFF
#define ADDRESS_MBC1_ROM_BANK_NUM_START 0x2000
#define ADDRESS_MBC1_ROM_BANK_NUM_END   0x3FFF
#define ADDRESS_MBC1_RAM_BANK_NUMBER_START 0x4000
#define ADDRESS_MBC1_RAM_BANK_NUMBER_END   0x5FFF
#define ADDRESS_MBC1_MODE_SELECT_START     0x6000
#define ADDRESS_MBC1_MODE_SELECT_END       0x7FFF

#define ADDRESS_MBC2_ROM_BANK_NUM_START ADDRESS_MBC1_ROM_BANK_NUM_START
#define ADDRESS_MBC2_ROM_BANK_NUM_END   ADDRESS_MBC1_ROM_BANK_NUM_END
#define ADDRESS_MBC2_RAM_BANK_NUMBER_START ADDRESS_MBC1_RAM_BANK_NUMBER_START
#define ADDRESS_MBC2_RAM_BANK_NUMBER_END ADDRESS_MBC1_RAM_BANK_NUMBER_END

#define ADDRESS_MBC3_ROM_BANK_NUM_START ADDRESS_MBC1_ROM_BANK_NUM_START
#define ADDRESS_MBC3_ROM_BANK_NUM_END   ADDRESS_MBC1_ROM_BANK_NUM_END
#define ADDRESS_MBC3_RAM_BANK_NUMBER_START ADDRESS_MBC1_RAM_BANK_NUMBER_START
#define ADDRESS_MBC3_RAM_BANK_NUMBER_END ADDRESS_MBC1_RAM_BANK_NUMBER_END

#define ADDRESS_MBC5_ROM_BANK_NUM_START ADDRESS_MBC1_ROM_BANK_NUM_START
#define ADDRESS_MBC5_ROM_BANK_NUM_END   0x2FFFF


#define CART_RAM_VALUE_ENABLED 0x0A
#define CART_MBC1_ROM_RAM_MODE_SELECT 0x01

class GBCart{
  private:
    uint8_t* m_bootRom;
    uint8_t* m_cartRom;
    uint8_t* m_cartRam;
    
    uint32_t m_cartDataLength;
    uint32_t m_cartRamLength;
    
    //Listed in the order that they appear in the documentation
    char* m_CartTitle;
    bool m_bIsGBC;
    uint8_t m_LicenseeNewHigh;
    uint8_t m_LicenseeNewLow;
    bool m_bIsSGB;    
    uint8_t m_CartType;
    uint8_t m_MBCType;
    uint8_t m_CartRomSize;
    uint8_t m_CartRamSize;
    uint8_t m_CartRegion;
    uint8_t m_LicenseeOld;
    uint8_t m_MaskVersion;
    
    bool m_bHasRam;
    bool m_bHasBattery;
    bool m_bHasRumble;
    
    bool m_bBootRomEnabled;
    
    uint16_t m_cartRomBank;
    uint8_t m_cartRamBank;
    
    bool m_bCartRamEnabled;
    
    bool m_bMBC1RomRamSelect;
    
    char* m_romFileName;
    char* m_saveFileName;
    
    void loadCartFile(char* filename);
    void loadBootRom(char* filename);
    
    void loadCartRam();
    void saveCartRam();
    
    void postCartLoadSetup();
    void loadCartArray(uint8_t* cart, uint16_t size);    
    
    uint8_t read_MBC(uint16_t address);
    void write_MBC(uint16_t address, uint8_t val);
    
  public:
    enum ROMType{
        ROM_ONLY = 0x00,
        ROM_MBC1 = 0x01,
        ROM_MBC1_RAM = 0x02,
        ROM_MBC1_RAM_BATT = 0x03,
        ROM_MBC2 = 0x05,
        ROM_MBC2_BATT = 0x06,
        ROM_RAM = 0x08,
        ROM_RAM_BATT = 0x09,
        ROM_MMM01 = 0x0B,
        ROM_MMM01_SRAM = 0x0C,
        ROM_MMM01_SRAM_BATT = 0x0D,
        ROM_MBC3_RAM = 0x12,
        ROM_MBC3_RAM_BATT = 0x13,
        ROM_MBC5 = 0x19,
        ROM_MBC5_RAM = 0x1A,
        ROM_MBC5_RAM_BATT = 0x1B,
        ROM_MBC5_RUMB = 0x1C,
        ROM_MBC5_RUMB_SRAM = 0x1D,
        ROM_MBC5_RUMB_SRAM_BATT = 0x1E,
        ROM_CAMERA = 0x1F,
        ROM_BANDAI = 0xFD,
        ROM_HUDSON_HUC3 = 0xFE,
        ROM_MBC3_TIMER_BATT = 0x0F,
        ROM_MBC3_TIMER_RAM_BATT = 0x10,
        ROM_MBC3 = 0x11,
        ROM_HUDSON_HUC1 = 0xFF
    };

    enum MBCType{
        MBC_NONE = 0,
        MBC_1,
        MBC_2,
        MBC_3,
        MBC_5,
        MBC_OTHER
    };
    
    enum ROMSize{
        ROM_32K = 0x00,
        ROM_64K = 0x01,
        ROM_128K = 0x02,
        ROM_256K = 0x03,
        ROM_512K = 0x04,
        ROM_1M   = 0x05,
        ROM_2M   = 0x06,
        ROM_1_1M = 0x52,
        ROM_1_2M = 0x53,
        ROM_1_5M = 0x54
    };
    
    enum RAMSize{
        RAM_NONE = 0x00,
        RAM_2K   = 0x01,
        RAM_8K   = 0x02,
        RAM_32K  = 0x03,
        RAM_128K = 0x04
    };
    
    GBCart(char* filename, char* bootrom);
    GBCart(uint8_t* cart, uint16_t size);
    ~GBCart();
    
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t val);
    
    //Saves cart ram, if cart has a battery backup
    void save();
    
    //Sets whether the first 0xFF bytes are read from the boot rom instead of cartridge
    void setBootRomEnabled(bool enabled);
    
    //Gets whether the first 0xFF bytes are read from boot rom instead of cartridge
    bool getBootRomEnabled();
    
    std::string getCartridgeTitle();
    void printCartInfo();
};
