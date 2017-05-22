#pragma once
#include <stdint.h>
#include <vector>
#include "../constants.h"
#include "gblcd.h"
#include "gbmem.h"
#include "gbpad.h"

#define PULSE_RESET 0x00
#define PULSE_HIGH  0x01
#define PULSE_LOW   0x02
#define PULSE_END   0x03

enum SGBCommand {
    SGB_COMMAND_PAL01 = 0,
    SGB_COMMAND_PAL23,
    SGB_COMMAND_PAL03,
    SGB_COMMAND_PAL12,
    SGB_COMMAND_ATTR_BLK,
    SGB_COMMAND_ATTR_LIN,
    SGB_COMMAND_ATTR_DIV,
    SGB_COMMAND_ATTR_CHR,
    SGB_COMMAND_SOUND,
    SGB_COMMAND_SOU_TRN,
    SGB_COMMAND_PAL_SET,
    SGB_COMMAND_PAL_TRN,
    SGB_COMMAND_ATRC_EN,
    SGB_COMMAND_TEST_EN,
    SGB_COMMAND_ICON_EN,
    SGB_COMMAND_DATA_SND,
    SGB_COMMAND_DATA_TRN,
    SGB_COMMAND_MLT_REG,
    SGB_COMMAND_JUMP,
    SGB_COMMAND_CHR_TRN,
    SGB_COMMAND_PCT_TRN,
    SGB_COMMAND_ATTR_TRN,
    SGB_COMMAND_ATTR_SET,
    SGB_COMMAND_MASK_EN,
    SGB_COMMAND_OBJ_TRN,
    SGB_COMMAND_UNDEFINED
};

class SGBHandler {
    public:
        SGBHandler(GBMem* memory, GBLCD* lcd, GBPad* pad);
        ~SGBHandler();
        void init();
        void tick(float deltaTime);

        void sendPacketPulse(uint8_t pulse);

        void colorFrame(RGBColor** frame);

    private:
        GBMem* m_mem;
        GBLCD* m_lcd;
        GBPad* m_pad;
        
        //Whether or not we are currently listening for packets.
        bool bReceivingPacket = false;

        //The current packet being built up.
        uint8_t m_currentPacket[16];

        //The index of the current packet byte.
        uint8_t m_currentPacketIndex = 0;

        //The bit being set on the current packet byte
        uint8_t m_currentPacketBitIndex = 0;

        //Number of remaining packets, if multiple packets are bieng sent.
        uint8_t m_remainingPackets = 0;

        //Current SGB command being parsed
        SGBCommand m_currentCommand;

        //Color 0 is the same across all color palettes
        RGBColor m_color0;

        //Color palettes.
        RGBColor** m_colorPalettes;

        //Color palette indicies for each 8x8 tile on screen
        uint8_t** m_framePaletteIndicies;

        void appendBit(uint8_t bit);

        void parseCurrentPacket();

        void commandSetPalettes(uint8_t palette1Index, uint8_t palette2Index, uint8_t* data);

        void commandSetAttributeBlock(uint8_t* data);

        void commandSetAttributeLine(uint8_t* data);

        void commandSetAttributeDivide(uint8_t* data);

        void commandSetAttributeCharacter(uint8_t* data);

        //Unused commands between implementations here...

        void commandMultiplayerRegister(uint8_t* data);

        //Unused commands between implemenations here...

        void commandWindowMask(uint8_t* data);
};