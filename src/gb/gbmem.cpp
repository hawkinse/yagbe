#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include "gbmem.h"
#include "gbz80cpu.h" //Needed for CLOCK_GB

using namespace std;

GBMem::GBMem(Platform systemType){
    m_mem[ADDRESS_IF] = 0;
	m_WRamBank = 1;
	m_systemType = systemType;
	m_bDoubleClockSpeed = false;
	m_bPrepareForSpeedSwitch = false;
}

GBMem::~GBMem(){
}

void write_unusable(uint16_t address);
void read_unusable(uint16_t address);

void GBMem::increment_RegisterDIV(long long hz){
    //Store any unused Hz so that we can factor it into the next update
static long long timeRollover = 0;

//Include any previous clock unused from the last update
hz += timeRollover;

//Increment the DIV register
while (hz >= DIV_INCREMENT_CLOCK) {
	m_mem[ADDRESS_DIV]++;
	hz -= DIV_INCREMENT_CLOCK;
}

//Store leftover, unused clock
if (hz >= 0) {
	timeRollover = hz;
}
}

void GBMem::increment_RegisterTIMA(long long hz) {
	//Store any unused Hz so that we can factor it into the next update
	static long long timeRollover = 0;

	//Only increment register if timer is enabled.
	if (!(m_mem[ADDRESS_TAC] & 0x04)) {
		timeRollover = 0;
		return;
	}

	//Include any previous clock unused from the last update
	hz += timeRollover;

	//Gets the clock used to increment the TIMA register
	int TAC_Clock = 0;
	uint8_t TAC_ClockSelect = m_mem[ADDRESS_TAC] & 3;
	switch (TAC_ClockSelect) {
	case TAC_CLOCK_4096:
		TAC_Clock = 1024;
		break;
	case TAC_CLOCK_262144:
		TAC_Clock = 16;
		break;
	case TAC_CLOCK_65536:
		TAC_Clock = 64;
		break;
	case TAC_CLOCK_16384:
		TAC_Clock = 256;
		break;
	default:
		if (CONSOLE_OUTPUT_ENABLED) std::cout << "Invalid TAC Clock select " << TAC_ClockSelect << std::endl;
		break;
	}

	//Increment the TIMA register
	while (hz >= TAC_Clock) {
		//Check if we need to fire the timer interrupt
		if ((m_mem[ADDRESS_TIMA] + 1) > 0xFF) {
			if (CONSOLE_OUTPUT_ENABLED) std::cout << "TIMA register rolled over. Setting interrupt flag" << std::endl;

			//Set the timmer interrupt flag
			write(ADDRESS_IF, m_mem[ADDRESS_IF] | INTERRUPT_FLAG_TIMER);

			//TIMA is set to TMA on overflow
			m_mem[ADDRESS_TIMA] = m_mem[ADDRESS_TMA];
		}
		else {
			m_mem[ADDRESS_TIMA]++;
		}

		hz -= TAC_Clock;
	}

	//Store leftover, unused clock
	if (hz >= 0) {
		timeRollover = hz;
	}
}

void GBMem::write(uint16_t address, uint8_t value) {

	//Output results of Blaarg test roms on console if LCD isn't working properly.
	if (BLAARG_TEST_OUTPUT && (address == 0xFF02) && (value == 0x81)) {
	std::cout << read(0xFF01);
	}

	if (((address >= ROM_BANK_0_START) && (address <= ROM_BANK_N_END))) {
		if (m_gbcart != NULL) {
			m_gbcart->write(address, value);
		}
		else {
			if (CONSOLE_OUTPUT_ENABLED) std::cout << "Unable to write data to cartridge - cartridge object pointer is null!" << std::endl;
		}
	}
	else if ((address >= EXTRAM_START) && (address <= EXTRAM_END)) {
  		m_gbcart->write(address, value);
	}
	else if (address == ADDRESS_BOOTROM) {
		//Disable bootrom access
		if (value == 0x01) {
			m_gbcart->setBootRomEnabled(false);
		}
	} else if (address == ADDRESS_SVBK) {
		//Only allow ram bank switching in GBC mode
		if (m_systemType == Platform::PLATFORM_GBC) {
			//Bank switching uses a backup and restore approach, to preserve functionality of direct access functions.

			//Back up current bank
			memcpy(m_wRamBanks + ((m_WRamBank - 1) * 4096), m_mem + WRAM_BANK_1_START, sizeof(uint8_t) * (WRAM_BANK_1_END - WRAM_BANK_1_START));

			//Work ram bank is only 3 bits
			m_WRamBank = value & 0x03;

			//Ensure that work ram bank is never set to 0.
			if (m_WRamBank == 0) {
				m_WRamBank = 1;
			}

			//Store register content so that direct access still works
			m_mem[ADDRESS_SVBK] = m_WRamBank;

			//Restore current ram bank
			memcpy(m_mem + WRAM_BANK_1_START, m_wRamBanks + ((m_WRamBank - 1) * 4096), sizeof(uint8_t) * (WRAM_BANK_1_END - WRAM_BANK_1_START));

			if (CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Bank switch to " << +m_WRamBank << std::endl;
		}
		
	} else if (address == ADDRESS_KEY1) {
		//Only bit 0 is writeable, indicates that the CPU is about to switch clock speeds
		m_bPrepareForSpeedSwitch = value & 1;
	} else if (address == ADDRESS_DIV) {
		  //Writing to the DIV register resets it
		m_mem[ADDRESS_DIV] = 0;
	} else if (address == ADDRESS_JOYP){
		  m_gbpad->write(value);
	} else if (address == ADDRESS_SERIAL_CONTROL){
		  if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Writing to serial control\n" << std::endl;
		m_gbserial->write_control(value);
		if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "\n"; //Write a new line so we can clearly see the serial output
	} else if((address >= ECHO_RAM_START) && (address <= ECHO_RAM_END)){
	   uint16_t echoAddress = address - (ECHO_RAM_START - WRAM_BANK_0_START);
	   if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Attempting to write to echo ram address " << +address << ", redirecting to " << echoAddress << std::endl;
	   m_mem[echoAddress] = value;
	} else if ((address >= VRAM_START) && (address <= VRAM_END)){
	    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Writing VRam" << std::endl;
	   m_gblcd->writeVRam(address, value);
	} else if ((address >= OAM_START) && (address <= OAM_END)){
		if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Writing Sprite Attribute Table" << std::endl;
		m_gblcd->writeVRamSpriteAttribute(address, value);
	} else if (address == ADDRESS_LCDC){
		if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Writing LCDC" << std::endl;
		m_gblcd->setLCDC(value);
	} else if (address == ADDRESS_STAT){
		if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Writing STAT" << std::endl;
		m_gblcd->setSTAT(value);
	} else if (address == ADDRESS_LY){
		m_gblcd->setLY(value);
	} else if (address == ADDRESS_LYC){
		if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Writing LYC-LY Compare" << std::endl;
		m_gblcd->setLYC(value);
	} else if(address == ADDRESS_DMA){
		if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Writing address for OAM DMA Transfer" << std::endl;
		m_gblcd->startDMATransfer(value);
	} else if (address == ADDRESS_NR10){
		m_gbaudio->setNR10(value);
	} else if (address == ADDRESS_NR11){
		m_gbaudio->setNR11(value);
	} else if (address == ADDRESS_NR12){
	    m_gbaudio->setNR12(value);
	} else if (address == ADDRESS_NR13){
		m_gbaudio->setNR13(value);
	} else if (address == ADDRESS_NR14){
		m_gbaudio->setNR14(value);
	} else if (address == ADDRESS_NR21){
		m_gbaudio->setNR21(value);
	} else if (address == ADDRESS_NR22){
		m_gbaudio->setNR22(value);
	} else if (address == ADDRESS_NR23){
		m_gbaudio->setNR23(value);
	} else if (address == ADDRESS_NR24){
		m_gbaudio->setNR24(value);
	} else if (address == ADDRESS_NR30){
		m_gbaudio->setNR30(value);
	} else if (address == ADDRESS_NR31){
		m_gbaudio->setNR31(value);
	} else if (address == ADDRESS_NR32){
		m_gbaudio->setNR32(value);
	} else if (address == ADDRESS_NR33){
		m_gbaudio->setNR33(value);
	} else if (address == ADDRESS_NR34){
		m_gbaudio->setNR34(value);
	} else if (address == ADDRESS_NR41){
		m_gbaudio->setNR41(value);
	} else if (address == ADDRESS_NR42){
		m_gbaudio->setNR42(value);
	} else if (address == ADDRESS_NR43){
		m_gbaudio->setNR43(value);
	} else if (address == ADDRESS_NR44){
		m_gbaudio->setNR44(value);
	} else if (address == ADDRESS_NR50){
		m_gbaudio->setNR50(value);
	} else if (address == ADDRESS_NR51){
		m_gbaudio->setNR51(value);
	} else if (address == ADDRESS_NR52){
		m_gbaudio->setNR52(value);
	} else if (address == ADDRESS_IF){
		if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Writing interrupt flags";
		m_mem[ADDRESS_IF] = value;
	}else {
		if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << std::hex << "Standard write address " << address << ", val " << +value << std::endl;
		m_mem[address] = value;
	}
}

uint8_t GBMem::read(uint16_t address){  
  uint8_t toReturn = 0xFF;
  bool bDirectReadAddress = false;
  if(CONSOLE_OUTPUT_ENABLED) std::cout << std::hex;
  
  if((address >= ROM_BANK_0_START) && (address <= ROM_BANK_N_END)){
      if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Cartridge read " << +address << std::endl;
      toReturn = m_gbcart->read(address);
  } else if ((address >= EXTRAM_START) && (address <= EXTRAM_END)){
      toReturn = m_gbcart->read(address);
  } else if (address == ADDRESS_SVBK) {
	  toReturn = m_WRamBank;
  } else if (address == ADDRESS_KEY1) {
	  //Bit 7 is the current CPU speed. Bit 0 is whether or not the CPU will switch speeds when executing STOP
	  toReturn = 0;
	  toReturn |= m_bPrepareForSpeedSwitch;
	  toReturn |= m_bDoubleClockSpeed << 7;

  } else if((address >= ECHO_RAM_START) && (address <= ECHO_RAM_END)){
      uint16_t echoAddress = address - (ECHO_RAM_START - WRAM_BANK_0_START);
      if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Attempting to read to echo ram address " << +address << ", redirecting to " << echoAddress << std::endl;
      toReturn = m_mem[echoAddress];
  } else if ((address >= UNUSABLE_START) && (address <= UNUSABLE_END)){
      if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Attempt to read from unreadable ram! Address " << +address << std::endl;
  } else if (address == ADDRESS_JOYP){
      if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Reading gamepad buttons" << std::endl;
      toReturn = m_gbpad->read();
  } else if ((address >= VRAM_START) && (address <= VRAM_END)){
      if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Reading VRam" << std::endl;
      toReturn = m_gblcd->readVRam(address);
  } else if ((address >= OAM_START) && (address <= OAM_END)){
      if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Reading Sprite Attribute Table" << std::endl;
      toReturn = m_gblcd->readVRamSpriteAttribute(address);
  }  else {
      if(CONSOLE_OUTPUT_ENABLED && CONSOLE_OUTPUT_IO) std::cout << "Standard read address " << +address << std::endl;
      bDirectReadAddress = true;
  }
  
  if(bDirectReadAddress){
      toReturn = m_mem[address];
  }
  
  return toReturn;
}

//Direct read and write, to bypass logic for hardware reads and writes
//Ex. to better implement display
void GBMem::direct_write(uint16_t address, uint8_t value){
    m_mem[address] = value;
}

uint8_t GBMem::direct_read(uint16_t address){
    return m_mem[address];
}
    
void GBMem::loadCart(GBCart* cart) {
	m_gbcart = cart;

	//If platform is set to auto, set based on cart type
	if (m_systemType == Platform::PLATFORM_AUTO) {
		if (m_gbcart->cartSupportsGBC()) {
			m_systemType = Platform::PLATFORM_GBC;
		} else {
			if (m_gbcart->cartSupportsSGB()) {
				m_systemType = Platform::PLATFORM_SGB;
			} else {
				m_systemType = Platform::PLATFORM_DMG;
			}
		}
	}

}

void GBMem::setLCD(GBLCD* lcd){
    m_gblcd = lcd;
}

void GBMem::setAudio(GBAudio* audio){
    m_gbaudio = audio;
}

void GBMem::setPad(GBPad* pad){
    m_gbpad = pad;
}

void GBMem::setSerial(GBSerial* serial){
    m_gbserial = serial;
}

//Get whether or not we are in GBC mode
bool GBMem::getGBCMode() {
	return (m_systemType == Platform::PLATFORM_GBC);
}

//Get whether or not we are reading from boot rom instead of cartridge
//Allows other components to check without having direct rom access.
bool GBMem::getBootRomEnabled(){
    return m_gbcart->getBootRomEnabled();
}

//Get current speed
bool GBMem::getDoubleSpeedMode() {
	return m_bDoubleClockSpeed;
}

//Get whether or not the CPU can switch speeds
bool GBMem::getPreparedForSpeedSwitch() {
	return m_bPrepareForSpeedSwitch;
}

//Toggles the current speed
void GBMem::toggleDouleSpeedMode() {
	//Clear speed switch flag.
	m_bPrepareForSpeedSwitch = false;

	//toggle speed
	m_bDoubleClockSpeed = !m_bDoubleClockSpeed;
}

//Used to update timer registers
void GBMem::tick(long long hz){
    increment_RegisterDIV(hz);
    increment_RegisterTIMA(hz);
}
