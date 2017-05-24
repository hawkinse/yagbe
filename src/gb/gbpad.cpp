#include <iostream>
#include "gbpad.h"
#include "gbmem.h"
#include "sgbhandler.h"

GBPad::GBPad(GBMem* mem){
    m_gbmemory = mem;
    m_sgbMultiplayerEnabled = false;
    m_sgbPlayerCount = 1;
    m_sgbCurrentPlayer = 1;
    m_bButtonA = false;
    m_bButtonB = false;
    m_bButtonStart = false;
    m_bButtonSelect = false;
    m_bButtonLeft = false;
    m_bButtonRight = false;
    m_bButtonUp = false;
    m_bButtonDown = false;
    m_sgbhandler = NULL;
    //Upper two bits are always set.
    m_JoypadRegister = 0x10; //Start with direction buttons set 
}

GBPad::~GBPad(){
}
    
void GBPad::setSGBHandler(SGBHandler* handler) {
    m_sgbhandler = handler;
}

void GBPad::write(uint8_t val){
    m_JoypadRegister = val;

    //If SGB is present, send the joypad select value as a pulse.
    if (m_sgbhandler != NULL) {
        m_sgbhandler->sendPacketPulse((val >> 4) & 3);
        
        //Advance current player if enabled
        if(m_sgbPlayerCount > 1){
            m_sgbCurrentPlayer = ((m_sgbCurrentPlayer + 1) % (m_sgbPlayerCount));
        } else {
            //Reset to 1 just in case multiplayer was turned off.
            m_sgbCurrentPlayer = 1;
        }
    }
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
    if(joypadSelect == JOYPAD_SELECT_BUTTONS){
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
        
    } else if (joypadSelect == JOYPAD_SELECT_DPAD){
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
        if(m_sgbhandler != NULL && m_sgbPlayerCount > 1){
            switch(m_sgbCurrentPlayer){
                case 0:
                    m_JoypadRegister |= JOYPAD_SGB_ID_1;
                    break;
                case 1:
                    m_JoypadRegister |= JOYPAD_SGB_ID_2;
                    break;
                case 2:
                    m_JoypadRegister |= JOYPAD_SGB_ID_3;
                    break;
                case 3:
                    m_JoypadRegister |= JOYPAD_SGB_ID_4;
                    break;
            }
        }
    }
    
    if(fireInterrupt){
        uint8_t interruptFlags = m_gbmemory->direct_read(ADDRESS_IF);
        m_gbmemory->direct_write(ADDRESS_IF, interruptFlags | INTERRUPT_FLAG_JOYPAD);
    }
}
    
void GBPad::setPlayerCount(int playerCount){
    if(m_sgbhandler != NULL){
        m_sgbMultiplayerEnabled = playerCount > 1;
        m_sgbPlayerCount = playerCount;
        m_sgbCurrentPlayer = 1;
        std::cout << "Player count is now " << +m_sgbPlayerCount << std::endl;
    } else {
        std::cout << "Attempt to change joypad multiplayer status without super gameboy!" << std::endl;
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
