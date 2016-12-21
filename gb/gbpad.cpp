#include <iostream>
#include "gbpad.h"
#include "gbmem.h"

GBPad::GBPad(GBMem* mem){
    m_gbmemory = mem;
    m_bButtonA = false;
    m_bButtonB = false;
    m_bButtonStart = false;
    m_bButtonSelect = false;
    m_bButtonLeft = false;
    m_bButtonRight = false;
    m_bButtonUp = false;
    m_bButtonDown = false;
    //Upper two bits are always set.
    m_JoypadRegister = 0x10; //Start with direction buttons set 
}

GBPad::~GBPad(){
}
    
void GBPad::write(uint8_t val){
    m_JoypadRegister = val;
}

uint8_t GBPad::read(){
    //Update register with current button states
    updateState(false);
    return m_JoypadRegister;
}

void GBPad::updateState(bool fireInterrupt){
    //Get selected joypad input on bits 4 and 5
    uint8_t joypadSelect = (m_JoypadRegister >> 4) & 3;
    
    //Clear all button bits to indicate all buttons are pressed
    m_JoypadRegister &= 0xF0;
    
    //Set bits for unpressed buttons
    if(joypadSelect & JOYPAD_SELECT_BUTTONS){
        
        if(!m_bButtonA){
            m_JoypadRegister |= JOYPAD_INPUT_RIGHT_A;
        }
        
        if(!m_bButtonB){
            m_JoypadRegister |= JOYPAD_INPUT_LEFT_B;
        }
        
        if(!m_bButtonSelect){
            m_JoypadRegister |= JOYPAD_INPUT_UP_SELECT;
        }
        
        if(!m_bButtonStart){
            m_JoypadRegister |= JOYPAD_INPUT_DOWN_START;
        }
        
    } else if ((joypadSelect & JOYPAD_SELECT_DPAD)){
        
        if(!m_bButtonRight){
            m_JoypadRegister |= JOYPAD_INPUT_RIGHT_A;
        }
        
        if(!m_bButtonLeft){
            m_JoypadRegister |= JOYPAD_INPUT_LEFT_B;
        }
        
        if(!m_bButtonUp){
            m_JoypadRegister |= JOYPAD_INPUT_UP_SELECT;
        }
        
        if(!m_bButtonDown){
            m_JoypadRegister |= JOYPAD_INPUT_DOWN_START;
        }        
        
    } else {
        std::cout << "Unrecognized joypad select state!";
        m_JoypadRegister |= 0x0F;
    }
    
    if(fireInterrupt){
        uint8_t interruptFlags = m_gbmemory->direct_read(ADDRESS_IF);
        m_gbmemory->direct_write(ADDRESS_IF, interruptFlags | INTERRUPT_FLAG_JOYPAD);
    }
}
        
void GBPad::setA(bool state){
    //Only update the state if it changes
    bool bUpdateState = !(m_bButtonA == state);
    
    if(bUpdateState){
        m_bButtonA = state;
        updateState(true);
    }    
}

void GBPad::setB(bool state){
    //Only update the state if it changes
    bool bUpdateState = !(m_bButtonB == state);
    
    if(bUpdateState){
        m_bButtonB = state;
        updateState(true);
    }    
}

void GBPad::setStart(bool state){
    //Only update the state if it changes
    bool bUpdateState = !(m_bButtonStart == state);
    
    if(bUpdateState){
        m_bButtonStart = state;
        updateState(true);
    }    
}

void GBPad::setSelect(bool state){
    //Only update the state if it changes
    bool bUpdateState = !(m_bButtonSelect == state);
    
    if(bUpdateState){
        m_bButtonSelect = state;
        updateState(true);
    }    
}

void GBPad::setUp(bool state){
    //Only update the state if it changes
    bool bUpdateState = !(m_bButtonUp == state);
    
    if(bUpdateState){
        m_bButtonUp = state;
        updateState(true);
    }    
}

void GBPad::setDown(bool state){
    //Only update the state if it changes
    bool bUpdateState = !(m_bButtonDown == state);
    
    if(bUpdateState){
        m_bButtonDown = state;
        updateState(true);
    }    
}

void GBPad::setLeft(bool state){
    //Only update the state if it changes
    bool bUpdateState = !(m_bButtonLeft == state);
    
    if(bUpdateState){
        m_bButtonLeft = state;
        updateState(true);
    }    
}

void GBPad::setRight(bool state){
    //Only update the state if it changes
    bool bUpdateState = !(m_bButtonRight == state);
    
    if(bUpdateState){
        m_bButtonRight = state;
        updateState(true);
    }    
}

       
bool GBPad::getA(){
    return m_bButtonA;  
}

bool GBPad::getB(){
    return m_bButtonB;
}

bool GBPad::getStart(){
    return m_bButtonStart;
}

bool GBPad::getSelect(){
    return m_bButtonSelect;
}

bool GBPad::getUp(){
    return m_bButtonUp;
}

bool GBPad::getDown(){
    return m_bButtonDown;
}

bool GBPad::getLeft(){
    return m_bButtonLeft;
}

bool GBPad::getRight(){
    return m_bButtonRight;
}
