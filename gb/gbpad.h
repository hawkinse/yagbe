#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../constants.h"

//Note that state is inverted... 0 indicates selected!
#define JOYPAD_SELECT_BUTTONS 0x1
#define JOYPAD_SELECT_DPAD    0x2

//If these bits are set, the button is released
#define JOYPAD_INPUT_RIGHT_A    1
#define JOYPAD_INPUT_LEFT_B     2
#define JOYPAD_INPUT_UP_SELECT  4
#define JOYPAD_INPUT_DOWN_START 8

//Forward declaration for GBMem, since GBMem needs a reference to GBPad
class GBMem;

class GBPad{
    private:
        GBMem* m_gbmemory;
        
        //Stores the button state as it exists in gameboy memory
        uint8_t m_JoypadRegister;
        
        bool m_bButtonA;
        bool m_bButtonB;
        bool m_bButtonStart;
        bool m_bButtonSelect;
        bool m_bButtonUp;
        bool m_bButtonDown;
        bool m_bButtonLeft;
        bool m_bButtonRight;
        
        void updateState(bool fireInterrupt);
        
    public:
        GBPad(GBMem* mem);
        ~GBPad();    
        
        void write(uint8_t val);
        uint8_t read(); 
        
        void setA(bool state);
        void setB(bool state);
        void setStart(bool state);
        void setSelect(bool state);
        void setUp(bool state);
        void setDown(bool state);
        void setLeft(bool state);
        void setRight(bool state);
        
        bool getA();
        bool getB();
        bool getStart();
        bool getSelect();
        bool getUp();
        bool getDown();
        bool getLeft();
        bool getRight();
};