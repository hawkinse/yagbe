#include <iostream>
#include <fstream>
#include <cstring>
#include <string.h>
#include <stdio.h>

#include "gbmem.h"
#include "gbcart.h"

GBCart::GBCart(char* filename, char* bootrom){
    m_bBootRomEnabled = false;
    loadCartFile(filename);
    
    m_romFileName = filename;
    
    m_cartRomBank = 1;
    m_cartRamBank = 0;
    m_bCartRamEnabled = false;
    m_bMBC1RomRamSelect = false;
    if(bootrom != NULL){
        loadBootRom(bootrom);
    }
    
    if(m_bHasBattery){
        //Generate name for save file based on rom filename
        int romNameLength = std::strlen(filename);
        m_saveFileName = new char[romNameLength + 5];
        memcpy(m_saveFileName, m_romFileName, romNameLength);
        char* saveExtension = ".sav";
        memcpy(m_saveFileName + romNameLength, saveExtension, 4);
        m_saveFileName[romNameLength + 4] = '\0';
        
        loadCartRam();
    }
}

GBCart:: GBCart(uint8_t* cart, uint16_t size){
    loadCartArray(cart, size);
    //TODO - generate save file name based on cart header title
    m_saveFileName = m_CartTitle;
}

GBCart::~GBCart(){
    if(m_cartDataLength > 0){
        delete m_cartRom;
    }
    
    if(m_cartRamLength > 0){
        delete m_cartRam;
    }
}

void GBCart::loadCartFile(char* filename){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Loading cartridge " << filename << std::endl;
    std::ifstream cart (filename, std::ios_base::binary);
    if(cart.is_open()){
        cart.seekg(0, std::ios_base::end);
        std::streamsize size = cart.tellg();
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Size of cartridge: " << size << std::endl;
        cart.seekg(0, std::ios_base::beg);
        
        m_cartRom = new uint8_t[size];
        m_cartDataLength = size;
        
        cart.read((char*)m_cartRom, size);
        
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Finished reading cart into cart buffer" << std::endl;
        
        std::streamsize readBytes = cart.gcount();
        if(readBytes != size){
            std::cout << "WARNING - only " << readBytes << " read!" << std::endl;
            std::cin.get();
        }
        
        cart.close();
        
        postCartLoadSetup();
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Failed to load cart" << std::endl;
        exit(1);
    }
}

void GBCart::loadBootRom(char* filename){
    //Todo - detect if given bootrom file is even present
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Loading boot rom " << filename << std::endl;
    std::ifstream bootRom (filename, std::ios_base::binary);
    if(bootRom.is_open()){
        bootRom.seekg(0, std::ios_base::end);
        std::streamsize size = bootRom.tellg();
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Size of boot rom: " << size << std::endl;
        bootRom.seekg(0, std::ios_base::beg);
        
        m_bootRom = new uint8_t[size];
        
        bootRom.read((char*)m_bootRom, size);
        
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Finished loading boot rom" << std::endl;
        
        bootRom.close();
        
        m_bBootRomEnabled = true;
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Failed to load boot rom" << std::endl;
        exit(1);
    }
}

void GBCart::loadCartArray(uint8_t* cart, uint16_t size){
    m_cartRom = new uint8_t[size];
    memcpy(m_cartRom, cart, size);
    postCartLoadSetup();
}    

void GBCart::loadCartRam(){
    std::cout << "Loading cartridge ram " << m_saveFileName << std::endl;
    std::ifstream cart (m_saveFileName, std::ios_base::binary);
    if(cart.is_open()){
        cart.seekg(0, std::ios_base::end);
        std::streamsize size = cart.tellg();
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Size of save: " << size << std::endl;
        cart.seekg(0, std::ios_base::beg);
        
        cart.read((char*)m_cartRam, size);
        
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Finished reading cart ram into buffer" << std::endl;
        
        std::streamsize readBytes = cart.gcount();
        if(readBytes != m_cartRamLength){
            std::cout << "WARNING - read " << readBytes << " but save data should be " << m_cartRamLength << std::endl;
            std::cin.get();
        }
        
        cart.close();
        std::cout << "Cart ram loaded successfully" << std::endl;
    } else {
        std::cout << "Failed to load cart ram" << std::endl;
        
        //Clear cart ram
        memset(m_cartRam, 0xFF, m_cartRamLength);
    }
}

void GBCart::saveCartRam(){
    std::ofstream file(m_saveFileName, std::ofstream::binary | std::ofstream::out);
    if(file && file.is_open()){
        std::cout << "Writing " << m_cartRamLength << " bytes from cart ram" << std::endl;
        file.write((char*)m_cartRam, m_cartRamLength);
        file.close();
    } else {
        std::cout << "Failed to save file" << std::endl;
    }
}
    
void GBCart::postCartLoadSetup(){
    //Set the variable rom bank to 1
    //switchRomBank(1);
    m_cartRomBank = 1;
    
    //Set game title
    m_CartTitle = new char[16];
    memcpy(m_CartTitle, getCartridgeTitle().c_str(), 16);
    
    //Set if Gameboy Color game
    m_bIsGBC = (m_cartRom[ADDRESS_CART_PLATFORM] == 0x80);
    
    //Set new licensee code (high)
    m_LicenseeNewHigh = m_cartRom[ADDRESS_CART_LICENSEE_HI];
    
    //Set new licensee code (low)
    m_LicenseeNewLow = m_cartRom[ADDRESS_CART_LICENSEE_LO];
    
    //Set if Super Gameboy Game
    m_bIsSGB = (m_cartRom[ADDRESS_CART_SGB] == 0x03);
    
    //Set cart type
    m_CartType = m_cartRom[ADDRESS_CART_TYPE];
    
    //TODO - more setup based on cart type here
    
    //Set rom size
    m_CartRomSize = m_cartRom[ADDRESS_CART_ROM_SIZE];
    
    //Set cart ram size
    m_CartRamSize = m_cartRom[ADDRESS_CART_RAM_SIZE];
    
    //TODO - init cart ram
    switch(m_CartRamSize){
        case RAM_NONE:
            m_cartRamLength = 0;
            break;
        case RAM_2K:
            m_cartRamLength = 2048;
            break;
        case RAM_8K:
            m_cartRamLength = 8192;
            break;
        case RAM_32K:
            m_cartRamLength = 32768;
            break;
        case RAM_128K:
            m_cartRamLength = 131072;
            break;
        default:
            std::cout << "Unable to determine cart ram quantity" << std::endl;
    }
    
    m_cartRam = new uint8_t[m_cartRamLength];
    
    //Set region
    m_CartRegion = m_cartRom[ADDRESS_CART_REGION];
    
    //Set old licensee code:
    m_LicenseeOld = m_cartRom[ADDRESS_CART_LICENSEE_OLD];
    
    //Mask rom version
    m_MaskVersion = m_cartRom[ADDRESS_CART_MASK_ROM_VER];
    
    //Set up read/write function pointers
    switch(m_CartType){
        case ROM_ONLY:
            std::cout << "Cart is a ROM-only cart" << std::endl;
            cartRead = &GBCart::read_NoMBC;
            cartWrite = &GBCart::write_NoMBC;
            break;
        case ROM_MBC1:
        case ROM_MBC1_RAM:
        case ROM_MBC1_RAM_BATT:
            std::cout << "Cart is MBC1" << std::endl;
            cartRead = &GBCart::read_MBC1;
            cartWrite = &GBCart::write_MBC1;
            break;
        case ROM_MBC2:
        case ROM_MBC2_BATT:
            std::cout << "Cart is MBC2" << std::endl;
            cartRead = &GBCart::read_MBC2;
            cartWrite = &GBCart::write_MBC2;
            ///MBC2 has a fixed ram size, set here.
            m_cartRamLength = 512;
            delete m_cartRam;
            m_cartRam = new uint8_t[m_cartRamLength];
            break;
        case ROM_MBC3:
        case ROM_MBC3_RAM_BATT:
        case ROM_MBC3_TIMER_BATT:
        case ROM_MBC3_TIMER_RAM_BATT:
            std::cout << "Cart is MBC3" << std::endl;
            cartRead = &GBCart::read_MBC3;
            cartWrite = &GBCart::write_MBC3;
            break;
        case ROM_MBC5:
        case ROM_MBC5_RAM:
        case ROM_MBC5_RAM_BATT:
        case ROM_MBC5_RUMB:
        case ROM_MBC5_RUMB_SRAM:
        case ROM_MBC5_RUMB_SRAM_BATT:
            std::cout << "Cart is MBC5" << std::endl;
            cartRead = &GBCart::read_MBC5;
            cartWrite = &GBCart::write_MBC5;
            //break;
            std::cout << "Until MBC5 is properly implemented, treating as MBC3 via fallthrough" << std::endl;
        default:
            std::cout << "Unable to determine cart type. Assuming MBC3!" << std::endl;
            cartRead = &GBCart::read_MBC3;
            cartWrite = &GBCart::write_MBC3;
            break;
    }
    
    //Sets whether or not we have a battery
    switch(m_CartType){
        case ROM_MBC1_RAM_BATT:
        case ROM_MBC2_BATT:
        case ROM_MBC3_RAM_BATT:
        case ROM_MBC3_TIMER_BATT:
        case ROM_MBC3_TIMER_RAM_BATT:
        case ROM_MBC5_RAM_BATT:
        case ROM_MBC5_RUMB_SRAM_BATT:
            m_bHasBattery = true;
            break;
        default:
            m_bHasBattery = false;
            break;
    }
}

uint8_t GBCart::read_NoMBC(uint16_t address){
    return m_cartRom[address];
}

void GBCart::write_NoMBC(uint16_t address, uint8_t val){
    std::cout << "Unimplemented ROM-only cart write" << std::endl;
}
    
uint8_t GBCart::read_MBC1(uint16_t address){
    uint8_t toReturn = 0xFF;
    if(address >= ROM_BANK_N_START && address <= ROM_BANK_N_END){
        //int realAddr = address + ((m_cartRomBank - 1) * ROM_BANK_N_START);
        uint8_t realRomBank = (m_bMBC1RomRamSelect ? m_cartRomBank & 0x1F : m_cartRomBank & 0x3F);
        
        //If the lower bits are 0, increment. 
        
        if((realRomBank & 0x1F) == 0){
            realRomBank |= 1;
        }
        
        int realAddr = (address - ROM_BANK_N_START) + (realRomBank * ROM_BANK_N_START);
        if(realAddr >= m_cartDataLength){
            std::cout << "Attempt to read cart rom at address " << +realAddr << " when cart only has " << +m_cartDataLength << " bytes!" << std::endl;
        } else {
            toReturn = m_cartRom[realAddr];
        }
    } else if (address >= EXTRAM_START && address <= EXTRAM_END){
        if(m_bCartRamEnabled){
            std::cout << "MBC1 ram read" << std::endl;
            uint16_t realRamBank = (m_bMBC1RomRamSelect ? ((m_cartRomBank & 0x1F) >> 6) : 0);
            
            int realAddr = (EXTRAM_START * realRamBank) + (address - EXTRAM_START);
            if(realAddr >= m_cartRamLength){
                std::cout << "Attempt to read cart ram at address " << +address << " when cart only has " << +m_cartRamLength << " bytes!" << std::endl;
            } else {
                toReturn = m_cartRam[realAddr];
            }
        }
    } 
    
    return toReturn;
}

void GBCart::write_MBC1(uint16_t address, uint8_t val){
    std::cout << "Cart write. Address: " << +address << ", value: " << +val << std::endl;
    
    if(address >= ADDRESS_CART_RAM_ENABLE_START && address <= ADDRESS_CART_RAM_ENABLE_END){
        m_bCartRamEnabled = ((val & 0x0F) == CART_RAM_VALUE_ENABLED);
    } else if(address >= EXTRAM_START && address <= EXTRAM_END){
        std::cout << "Writing to cart ram!" << std::endl;
        if(m_bCartRamEnabled || true){
            int realAddr = (EXTRAM_START * m_cartRamBank) + (address - EXTRAM_START);
            if(realAddr >= m_cartRamLength){
                std::cout << "Attempt to write cart ram at address " << +address << " when cart only has " << +m_cartRamLength << " bytes!" << std::endl;
            } else {
                m_cartRam[realAddr] = val;
            }
        }
    } else if ((address >= ADDRESS_MBC1_ROM_BANK_NUM_START) && (address <= ADDRESS_MBC1_ROM_BANK_NUM_END)){
        std::cout << "Writing cart bank register" << std::endl;
        
        //Only set the lower 5 bits
        m_cartRomBank = (m_cartRomBank & 0xE0) | (val & 0x1F);
        
        std::cout << "Rom bank is now " << +m_cartRomBank << std::endl;
    } else if(address >= ADDRESS_MBC1_RAM_BANK_NUMBER_START && address <= ADDRESS_MBC1_RAM_BANK_NUMBER_END){
        std::cout << "Attempt to set ram bank number or upper rom bank number bits!" << std::endl;
        
        //Set upper bits of rom bank even if not in rom mode. Bits will be ignored depending on rom/ram select.
        m_cartRomBank = (m_cartRomBank & 0x1F) | (0x60 & (val << 6));
        
    } else if (address >= ADDRESS_MBC1_MODE_SELECT_START && address <= ADDRESS_MBC1_MODE_SELECT_END){
        std::cout << "Attempt to set ROM or RAM write mode!" << std::endl;
        m_bMBC1RomRamSelect = val & 0x01;        
    } else {
        std::cout << "Attempting to write to an unsupported address" << std::endl;
        std::cin.get();
    }
}
    
uint8_t GBCart::read_MBC2(uint16_t address){
    //std::cout << "Unimplemented MBC2 read" << std::endl;
    
    //Reading MBC2 appears to be compatible with reading MBC1
    return read_MBC1(address);
}

void GBCart::write_MBC2(uint16_t address, uint8_t val){
    //std::cout << "Unimplemented MBC2 write" << std::endl;
    
    if(address >= ADDRESS_CART_RAM_ENABLE_START && address <= ADDRESS_CART_RAM_ENABLE_END){
        //Only set cart ram if least significant bit of most significant byte is 0.
        if(~address & 0x0100){
            m_bCartRamEnabled = ((val & 0x0F) == CART_RAM_VALUE_ENABLED);
        }
    } else if(address >= EXTRAM_START && address <= EXTRAM_END){
        if(m_bCartRamEnabled || true){
            //No ram banks in MBC2.
            int realAddr = EXTRAM_START + (address - EXTRAM_START);
            if(realAddr >= m_cartRamLength){
                std::cout << "Attempt to write cart ram at address " << +address << " when cart only has " << +m_cartRamLength << " bytes!" << std::endl;
            } else {
                //Only lower four bits are used for ram in MBC2.
                m_cartRam[realAddr] = val & 0x0F;
            }
        }
    } else if ((address >= ADDRESS_MBC2_ROM_BANK_NUM_START) && (address <= ADDRESS_MBC2_ROM_BANK_NUM_END)){
        
        //Only write if bit 5 is 0.
        if(address & 0x0100){
            std::cout << "Writing cart bank register" << std::endl;
        
            //Only set the lower 4 bits
            m_cartRomBank = val & 0x0F;
        
            std::cout << "Rom bank is now " << +m_cartRomBank << std::endl;
        }
    }
}
    
uint8_t GBCart::read_MBC3(uint16_t address){
    uint8_t toReturn = 0xFF;
    if(address <= ROM_BANK_N_END){
        //Reading regular ROM data is the same as MBC1
        toReturn = read_MBC1(address);
    } else if (address >= EXTRAM_START && address <= EXTRAM_END){
        //Placeholder - use MBC1 ram reading. Think this is ok?
        toReturn = read_MBC1(address);
    }
    
    return toReturn;
}

void GBCart::write_MBC3(uint16_t address, uint8_t val){
    std::cout << "Unimplemented MBC3 write" << std::endl;
    std::cout << "Address: " << +address << std::endl;
    std::cout << "Value: " << +val << std::endl;
    if(address >= ADDRESS_CART_RAM_ENABLE_START && address <= ADDRESS_CART_RAM_ENABLE_END){
        m_bCartRamEnabled = ((val & 0x0F) == CART_RAM_VALUE_ENABLED);
    } else if(address >= EXTRAM_START && address <= EXTRAM_END){
        if(m_bCartRamEnabled || true){
            int realAddr = (EXTRAM_START * m_cartRamBank) + (address - EXTRAM_START);
            if(realAddr >= m_cartRamLength){
                std::cout << "Attempt to write cart ram at address " << +address << " when cart only has " << +m_cartRamLength << " bytes!" << std::endl;
            } else {
                m_cartRam[realAddr] = val;
            }
        } else {
            std::cout << "Attempt to write to cart ram, but its not enabled!" << std::endl;
        }
    } else if (address >= ADDRESS_MBC3_ROM_BANK_NUM_START && address <= ADDRESS_MBC3_ROM_BANK_NUM_END){
        //Handle special cases
        if(val == 0){
            val = 1;
        } 
        //TODO - RTC access!
        
        //Only set the lower 7 bits
        m_cartRomBank = val;//(m_cartRomBank & 0x80) | val;
        std::cout << "Rom bank is now " << +m_cartRomBank << std::endl;
    } else if(address >= ADDRESS_MBC2_RAM_BANK_NUMBER_START && address <= ADDRESS_MBC2_RAM_BANK_NUMBER_END){
        std::cout << "Attempt to set ram bank number or RTC bits!" << std::endl;
        //TODO - RTC support
        m_cartRamBank = val;
        
    } else if (address >= ADDRESS_MBC1_MODE_SELECT_START && address <= ADDRESS_MBC1_MODE_SELECT_END){
        std::cout << "Attempt to set ROM or RAM write mode!" << std::endl;
        m_bMBC1RomRamSelect = val & 0x01;
    } else {
        std::cout << "Attempting to write to an unsupported address" << std::endl;
    }
}

uint8_t GBCart::read_MBC5(uint16_t address){
    std::cout << "Unimplemented MBC5 read" << std::endl;
}

void GBCart::write_MBC5(uint16_t address, uint8_t val){
    std::cout << "Unimplemented MBC5 write" << std::endl;
}
    
uint8_t GBCart::read(uint16_t address){
    uint8_t toReturn = 0xFF;
    if((address < 0x100) && m_bBootRomEnabled){
        toReturn = m_bootRom[address];
    } else {
        //Addresses up to the end of Bank 0 always return bank 0, regardless of cart type.
        if(address <= ROM_BANK_0_END){
            toReturn = m_cartRom[address];
        } else {
            //Function pointer cartRead holds an appropriate read function for the current cart type
            toReturn = (this->*cartRead)(address);
        }
    }
    
    return toReturn;//(m_bBootRomEnabled ? m_bootRom[address] : m_cartRom[address]);
}

void GBCart::write(uint16_t address, uint8_t val){
    //Function pointer cartRead holds an appropriate write function for the current cart type
    (this->*cartWrite)(address, val);
}

//Saves cart ram, if cart has a battery backup
void GBCart::save(){
    if(m_bHasBattery){
        std::cout << "Cart has a battery. Saving cart ram to " << m_saveFileName << std::endl;
        
        saveCartRam();
    }
}
   
//Sets whether the first 0xFF bytes are read from the boot rom or the cartridge
void GBCart::setBootRomEnabled(bool enabled){
    m_bBootRomEnabled = enabled;
}

bool GBCart::getBootRomEnabled(){
    return m_bBootRomEnabled;
}

std::string GBCart::getCartridgeTitle(){
    char gameTitle[16];
    
    memcpy(&gameTitle, m_cartRom + ADDRESS_CART_TITLE_START, ADDRESS_CART_TITLE_END - ADDRESS_CART_TITLE_START);
    
    return std::string(gameTitle);
}

void GBCart::printCartInfo(){
    //TODO - change this to use global variables instead of directly grbabing values out of array.
    std::cout << std::hex;
    std::cout << "Game: " << m_CartTitle << "\n";
    std::cout << "Targets GBC: " << m_bIsGBC << "\n";
    std::cout << "Licensee code (new, hi): " << std::hex << +m_LicenseeNewHigh << "\n";
    std::cout << "Licensee code (new, lo): " << std::hex << +m_LicenseeNewLow << "\n";
    std::cout << "Targets SGB: " << m_bIsSGB << "\n";
    std::cout << "Cart type: " << std::hex << +m_CartType << "\n";
    std::cout << "Cart rom: " << std::hex << +m_CartRomSize << " (" << std::dec <<  m_cartDataLength << " bytes)" << "\n";
    std::cout << "Cart ram: " << std::hex << +m_CartRamSize << " (" << std::dec <<  m_cartRamLength << " bytes)"<< "\n";
    std::cout << "Region: " << std::hex << +m_CartRegion << "\n";
    std::cout << "Licensee code (old): " << std::hex << +m_LicenseeOld << "\n";
    std::cout << "Mask rom version: " << std::hex << +m_MaskVersion << "\n";
}
