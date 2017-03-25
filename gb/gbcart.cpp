#include <iostream>
#include <fstream>
#include <cstring>
#include <string.h>
#include <stdio.h>
#include <ctime>

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

    uint8_t m_rtcSeconds = 0;
    uint8_t m_rtcMinutes = 0;
    uint8_t m_rtcHours = 0;
    uint8_t m_rtcLowerDayCounter = 0;
    uint8_t m_rtcFlags = 0;
    bool m_bRTCLatched = false;

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
    
    //Set rom size
    m_CartRomSize = m_cartRom[ADDRESS_CART_ROM_SIZE];
    
    //Set cart ram size
    m_CartRamSize = m_cartRom[ADDRESS_CART_RAM_SIZE];
    
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
        default:
            std::cout << "Unable to determine cart ram quantity. Defaulting to max 128k" << std::endl;
        case RAM_128K:
            m_cartRamLength = 131072;
            break;
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
            m_MBCType = MBC_NONE;
            break;
        case ROM_MBC1:
        case ROM_MBC1_RAM:
        case ROM_MBC1_RAM_BATT:
            std::cout << "Cart is MBC1" << std::endl;
            m_MBCType = MBC_1;
            break;
        case ROM_MBC2:
        case ROM_MBC2_BATT:
            std::cout << "Cart is MBC2" << std::endl;
            ///MBC2 has a fixed ram size, set here.
            m_cartRamLength = 512;
            delete m_cartRam;
            m_cartRam = new uint8_t[m_cartRamLength];
            m_MBCType = MBC_2;
            break;
        case ROM_MBC3:
        case ROM_MBC3_RAM_BATT:
        case ROM_MBC3_TIMER_BATT:
        case ROM_MBC3_TIMER_RAM_BATT:
            std::cout << "Cart is MBC3" << std::endl;
            //cartRead = &GBCart::read_MBC3;
            //cartWrite = &GBCart::write_MBC3;
            m_MBCType = MBC_3;
            break;
        case ROM_MBC5:
        case ROM_MBC5_RAM:
        case ROM_MBC5_RAM_BATT:
        case ROM_MBC5_RUMB:
        case ROM_MBC5_RUMB_SRAM:
        case ROM_MBC5_RUMB_SRAM_BATT:
            std::cout << "Cart is MBC5" << std::endl;
            m_MBCType = MBC_5;
            break;
        default:
            std::cout << "Unable to determine cart type. Treating as MBC1" << std::endl;
            m_MBCType = MBC_1;
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
    
void GBCart::updateRTC(){
    //TODO - figure out how to calculate day counter from system time. Possible?
    std::cout << "Unimplemented update MBC RTC!" << std::endl;
    if(m_bRTCLatched){
        //RTC is latched, don't update. 
        return;
    }

    //Get the current time since epoch
    std::time_t unixTime = std::time(NULL);
    std::tm* localTime = std::localtime(&unixTime);
    m_rtcSeconds = localTime->tm_sec;
    m_rtcMinutes = localTime->tm_min;
    m_rtcHours = localTime->tm_hour;

}

uint8_t GBCart::read_MBC(uint16_t address){
    uint8_t toReturn = 0xFF;

    if(address >= ROM_BANK_N_START && address <= ROM_BANK_N_END){
        //Largest possible bank value is 9-bit from MBC5
        uint16_t realRomBank = m_cartRomBank & 0x01FF;

        //Test for cart types and mask off appropriate bits
        if(m_MBCType == MBC_1){
            if(m_bMBC1RomRamSelect){
                realRomBank &= 0x1F;
            } else {
                realRomBank &= 0x3F;
            }
        } else if(m_MBCType == MBC_3){
            realRomBank = m_cartRomBank & 0x7F;
        }
        
        //If the lower bits are 0 on MBC1, increment.
        if((m_MBCType == MBC_1) && ((realRomBank & 0x1F) == 0)){
            realRomBank |= 1;
        }

        //If entire rom bank is 0 on non-MBC5, increment
        if((m_MBCType != MBC_5) && (realRomBank == 0)){
            realRomBank |= 1;
        }
        
        //TODO - check if in bounds of cart and wrap bank around until it fits!
        // This will need to be done on write, not on read?
        //https://github.com/Gekkio/mooneye-gb/blob/master/docs/accuracy.markdown
        int realAddr = (address - ROM_BANK_N_START) + (realRomBank * ROM_BANK_N_START);
        toReturn = m_cartRom[realAddr];
    } else if (address >= EXTRAM_START && address <= EXTRAM_END){
        if(m_bCartRamEnabled){
            std::cout << "MBC1 ram read" << std::endl;
            uint16_t realRamBank = 0;
 
            //In MBC1, bank switching only occures if more than 8KB memory
            if((m_MBCType != MBC_1) || (m_bMBC1RomRamSelect)){
                realRamBank = m_cartRamBank;
            }

            //MBC3 uses ram bank to controll access to real time clock registers.
            if((m_MBCType == MBC_3) && (realRamBank >= RTC_BANK_SECONDS)){
                //Update real time clock registers
                updateRTC();

                //Fetch the appropriate value
                switch(realRamBank){
                    case RTC_BANK_SECONDS:
                        toReturn = m_rtcSeconds;
                        break;
                    case RTC_BANK_MINUTES:
                        toReturn =  m_rtcMinutes;
                        break;
                    case RTC_BANK_HOURS:
                        toReturn = m_rtcHours;
                        break;
                    case RTC_BANK_DAYCOUNTER:
                        toReturn = m_rtcLowerDayCounter;
                        break;
                    case RTC_BANK_FLAGS:
                        toReturn = m_rtcFlags;
                        break;
                     default:
                        std::cout << "Unrecognized MBC3 RTC register " << +realRamBank << std::endl;
                        break;
                }
            } else {
                int realAddr = ((EXTRAM_END - EXTRAM_START) * realRamBank) + (address - EXTRAM_START);
                toReturn = m_cartRam[realAddr];

                //MBC2 uses 4 bit values. Need to mask off upper bits
                if(m_MBCType == MBC_2){
                    toReturn &= 0x0F;
                }
            }
        }
    } 
    
    return toReturn;
}

void GBCart::write_MBC(uint16_t address, uint8_t val){
    std::cout << "Cart write. Address: " << +address << ", value: " << +val << std::endl;
    
    if(address >= ADDRESS_CART_RAM_ENABLE_START && address <= ADDRESS_CART_RAM_ENABLE_END){
        bool bAllowRamToggle = true;

        if(m_MBCType == MBC_2){
            if((address & 0x0100)){
                bAllowRamToggle = false;
            }
        }

        if(bAllowRamToggle){
            m_bCartRamEnabled = ((val & CART_RAM_VALUE_ENABLED) > 0);
        }

        std::cout << "Cart ram is now: " << (m_bCartRamEnabled ? "enabled" : "disabled") << std::endl;
    } else if(address >= EXTRAM_START && address <= EXTRAM_END){
        std::cout << "Writing to cart ram!" << std::endl;
        if(m_bCartRamEnabled){
            int realAddr = ((EXTRAM_END - EXTRAM_START) * m_cartRamBank) + (address - EXTRAM_START);
            m_cartRam[realAddr] = val;
        }
    } else if ((address >= ADDRESS_MBC1_ROM_BANK_NUM_START) && (address <= ADDRESS_MBC1_ROM_BANK_NUM_END)){
        std::cout << "Writing cart bank register lower bits" << std::endl;
        
        //Set rom bank according to bank controller
        if(m_MBCType == MBC_1){
            //Lower 5 bits only
            m_cartRomBank = (m_cartRomBank & 0x60) | (val & 0x1F);

        } else if (m_MBCType == MBC_2){
            //4-bit only
            m_cartRomBank = val & 0x0F;
        } else if (m_MBCType == MBC_3){
            //7-bit only
            m_cartRomBank = /*(m_cartRomBank & 0x80) |*/ (val & 0x7F);
        } else if (m_MBCType == MBC_5){
            //TODO - DELCARE CONSTANTS
            if(address < 0x3000){
                //Set lower 8 bits
                m_cartRomBank = (m_cartRomBank & 0x0100) | val;
            } else {
                //Set 9th bit
                 m_cartRomBank = (m_cartRomBank & 0x0FF) | ((val & 0x01) << 9);
            }
        }
       
        std::cout << "Rom bank is now " << +m_cartRomBank << std::endl;
    } else if(address >= ADDRESS_MBC1_RAM_BANK_NUMBER_START && address <= ADDRESS_MBC1_RAM_BANK_NUMBER_END){
        std::cout << "Writing ram bank register" << std::endl;
 
        //TODO - REWRITE OF BELOW IN PROGRESS        
        switch(m_MBCType){
            case MBC_1:
                //In MBC1, this can set both the rom or ram bank
                if(m_bMBC1RomRamSelect){
                    m_cartRamBank = val & 0x03;
                } else {
                    m_cartRomBank = (m_cartRomBank & 0x1F) | (0x03 & (val << 5));
                }
                break;
            case MBC_2:
                std::cout << "Attempt to set ram bank on MBC2!" << std::endl;
                break;
            case MBC_3:
            case MBC_5:
                //MBC3 only has banks 0-3, but also can be set to values 08-0C for RTC access
                //MBC5 uses the full 4 bit value of the bank.
                m_cartRamBank = val & 0x0F;
                break;
        }

        /*
        //Rewrite below section in terms of switch statement to account for MBC3 RTC
        //If MBC1, ram bank should only be adjusted if in ram select mode
        if((m_MBCType != MBC_1) || (m_bMBC1RomRamSelect)){
            //MBC1 and MBC3 use 2-bit bank select, MBC5 uses 4-bit. MBC2 does not use ram banks.
            m_cartRamBank = (m_MBCType == MBC_5) ? (val & 0x0F) : (val & 0x03);
            std::cout << "Ram bank is now " << +m_cartRamBank << std::endl;
        } else if (m_MBCType == MBC_1)/*
        //On MBC1, need to update upper rom bank bits to match.
        if(m_MBCType == MBC_1){*//*{
            m_cartRomBank = (m_cartRomBank & 0x1F) | (0x03 & (val << 5));
            std::cout << "MBC1 with RomRam select on Ram. Instead, switching cart bank to " << +m_cartRomBank << std::endl;
        }*/
        
    } else if (address >= ADDRESS_MBC1_MODE_SELECT_START && address <= ADDRESS_MBC1_MODE_SELECT_END){
        if(m_MBCType == MBC_1){
            std::cout << "Attempt to set ROM or RAM write mode!" << std::endl;
            m_bMBC1RomRamSelect = (val & 0x01) > 0;      
            std::cout << "Attempt to set ROM or RAM write mode! Current mode: " << (m_bMBC1RomRamSelect ? "RAM" : "ROM")  << std::endl;
        } else if (m_MBCType == MBC_3) {
            //RTC Latch is toggled in this register if a write of 0 is followed by a write of 1.
            static uint8_t lastVal = 0xFF;
            if((lastVal == 0x00) && (val == 0x01)){
                //Since RTC is latched on access, need to update value first.
                updateRTC();
                m_bRTCLatched = !m_bRTCLatched;

            }
            lastVal = val; 
        }
    } else {
        std::cout << "Attempting to write to an unsupported address" << std::endl;
        std::cin.get();
    }
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
            toReturn = read_MBC(address);
        }
    }
    
    return toReturn;
}

void GBCart::write(uint16_t address, uint8_t val){
    write_MBC(address, val);
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
