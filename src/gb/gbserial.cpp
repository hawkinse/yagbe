#include <iostream>
#include "gbmem.h"
#include "gbserial.h"

GBSerial::GBSerial(GBMem* mem){
    m_gbmemory = mem;
}

void GBSerial::write_control(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_SERIAL_CONTROL, val);
    
    //Set the input byte to 0xFF to indicate nohting on other end of link cable.
    m_gbmemory->direct_write(ADDRESS_SERIAL_DATA, 0xFF);
    
    if(!SERIAL_ENABLED) return;
    
    //If the transfer start bit is set, output the byte to console and set interrupt flag
    if(val & TRANSFER_CONTROL_START){
        std::cout << "Serial write: " << m_gbmemory->direct_read(ADDRESS_SERIAL_DATA) << std::endl;;
        uint8_t interruptFlags = m_gbmemory->direct_read(ADDRESS_IF);
        m_gbmemory->write(ADDRESS_IF, interruptFlags | INTERRUPT_FLAG_SERIAL);
    }
}
