#include <iostream>
#include "sgbhandler.h"
#include "gbmem.h"
#include "gblcd.h"
#include "gbpad.h"

SGBHandler::SGBHandler(GBMem* memory, GBLCD* lcd, GBPad* pad) {
    m_mem = memory;
    m_lcd = lcd;
    m_pad = pad;

    bReceivingPacket = false;
    m_currentPacketIndex = 0;
    m_currentPacketBitIndex = 0;
    m_remainingPackets = 0;
    m_currentCommand = SGBCommand::SGB_COMMAND_UNDEFINED;
    m_color0 = RGBColor(155, 188, 15);//COLOR_WHITE;

    m_colorPalettes = new RGBColor*[4];
    for (int paletteIndex = 0; paletteIndex < 4; paletteIndex++) {
        m_colorPalettes[paletteIndex] = new RGBColor[4];
        m_colorPalettes[paletteIndex][0] = RGBColor(155, 188, 15);//COLOR_WHITE;
        m_colorPalettes[paletteIndex][1] = RGBColor(139, 172, 15);//COLOR_LIGHTGRAY;
        m_colorPalettes[paletteIndex][2] = RGBColor(48, 98, 48);// COLOR_DARKGRAY;
        m_colorPalettes[paletteIndex][3] = RGBColor(15, 56, 15);//COLOR_BLACK;
    }

    m_framePaletteIndicies = new uint8_t*[FRAMEBUFFER_WIDTH/TILE_WIDTH];
    for (int tileX = 0; tileX < FRAMEBUFFER_WIDTH / TILE_WIDTH; tileX++) {
        m_framePaletteIndicies[tileX] = new uint8_t[FRAMEBUFFER_HEIGHT/TILE_HEIGHT];
        for (int tileY = 0; tileY < FRAMEBUFFER_HEIGHT / TILE_HEIGHT; tileY++){
            m_framePaletteIndicies[tileX][tileY] = 0;
        }

    }
}

SGBHandler::~SGBHandler() {

}

void SGBHandler::init() {
    std::cout << "Unimplemented Super Gameboy Init!" << std::endl;
}

void SGBHandler::tick(float deltaTime) {
    std::cout << "Unimplemented Super Gameboy Handler Tick!" << std::endl;
}

void SGBHandler::sendPacketPulse(uint8_t pulse) {
    //std::cout << "Pulse: " << +pulse << std::endl;
    switch (pulse) {
        case PULSE_RESET:
            std::cout << "SGB Reset pulse recieved" << std::endl;
            //Reset packet variables.
            m_currentPacketIndex = 0;
            m_currentPacketBitIndex = 0;

            //Set receiving packet state to true.
            bReceivingPacket = true;
            std::cout << "SGB has started receiving a packet!" << std::endl;
            break;
        case PULSE_END:
            //End pulse should be ignored.
            break;
        case PULSE_HIGH:
            //Bit 1
            if (bReceivingPacket) {
                appendBit(1);
            }
            break;
        case PULSE_LOW:
            if (bReceivingPacket) {
                //Check if we are at the end of the packet.
                //If so, this is a terminator. Otherwise, bit 0.
                if (m_currentPacketIndex > 15) {
                    //We have hit the stop bit. Parse the packet.
                    parseCurrentPacket();
                    bReceivingPacket = false;
                    std::cout << "SGB has finished receiving a packet" << std::endl;
                } else {
                    appendBit(0);
                }
            }
            break;
    }
}

void SGBHandler::colorFrame(RGBColor** frame) {
    for (int tileX = 0; tileX < FRAMEBUFFER_WIDTH / TILE_WIDTH; tileX++) {
        for (int tileY = 0; tileY < FRAMEBUFFER_HEIGHT / TILE_HEIGHT; tileY++) {
            for (int pixelX = 0; pixelX < TILE_WIDTH; pixelX++) {
                for (int pixelY = 0; pixelY < TILE_HEIGHT; pixelY++) {
                    uint8_t realPixelX = tileX * TILE_WIDTH + pixelX;
                    uint8_t realPixelY = tileY * TILE_HEIGHT + pixelY;
                    RGBColor currentPixel = frame[realPixelX][realPixelY];

                    if (currentPixel.equals(COLOR_WHITE)) {
                        currentPixel = m_color0;
                    } else if (currentPixel.equals(COLOR_LIGHTGRAY)) {
                        currentPixel = m_colorPalettes[m_framePaletteIndicies[tileX][tileY]][1];
                    } else if (currentPixel.equals(COLOR_DARKGRAY)) {
                        currentPixel = m_colorPalettes[m_framePaletteIndicies[tileX][tileY]][2];
                    } else if (currentPixel.equals(COLOR_BLACK)) {
                        currentPixel = m_colorPalettes[m_framePaletteIndicies[tileX][tileY]][3];
                    }

                    frame[realPixelX][realPixelY] = currentPixel;
                }
            }
        }
    }
}

void SGBHandler::appendBit(uint8_t bit) {
    //Add the current bit.
    m_currentPacket[m_currentPacketIndex] = m_currentPacket[m_currentPacketIndex] | (bit << m_currentPacketBitIndex);

    //Increment bit. If bit is on 8, then we need to advance the byte index.
    m_currentPacketBitIndex++;
    if (m_currentPacketBitIndex == 8) {
        m_currentPacketBitIndex = 0;
        m_currentPacketIndex++;
    }
}

void SGBHandler::parseCurrentPacket() {
    std::cout << "Unimplemented parse current packet!";

    //If remaining packets is 0, then this is the first packet in this sequence.
    if (m_remainingPackets == 0) {
        std::cout << "Packet header: " << +m_currentPacket[0] << std::endl;
        //Number of packets is in first three bits of first byte.
        m_remainingPackets = m_currentPacket[0] & 0x07;

        //Current command is in upper bits of first byte.
        m_currentCommand = (SGBCommand)(m_currentPacket[0] >> 3);
    }

    switch (m_currentCommand) {
        case SGB_COMMAND_PAL01:
            commandSetPalettes(0, 1, m_currentPacket + 1);
            break;
            break;
        case SGB_COMMAND_PAL23:
            commandSetPalettes(2, 3, m_currentPacket + 1);
            break;
            break;
        case SGB_COMMAND_PAL03:
            commandSetPalettes(0, 3, m_currentPacket + 1);
            break;
        case SGB_COMMAND_PAL12:
            commandSetPalettes(1, 2, m_currentPacket + 1);
            break;
        case SGB_COMMAND_ATTR_BLK:
            commandSetAttributeBlock(m_currentPacket); //Attributes can contain multiple packets... how to manage data offset for later packets?
            break;
        case SGB_COMMAND_ATTR_LIN:
            commandSetAttributeLine(m_currentPacket);
            break;
        case SGB_COMMAND_ATTR_DIV:
            commandSetAttributeDivide(m_currentPacket);
            break;
        case SGB_COMMAND_ATTR_CHR:
            commandSetAttributeCharacter(m_currentPacket);
            break;
        case SGB_COMMAND_MLT_REG:
            commandMultiplayerRegister(m_currentPacket);
            break;
        case SGB_COMMAND_MASK_EN:
            commandWindowMask(m_currentPacket);
            break;
        default:
            std::cout << "Unimiplemented SGB command " << +m_currentCommand << std::endl;
    }

    m_remainingPackets--;
}

void SGBHandler::commandSetPalettes(uint8_t palette1Index, uint8_t palette2Index, uint8_t* data) {
    std::cout << "Unimplemented SGB command set palette!" << std::endl;
}

void SGBHandler::commandSetAttributeBlock(uint8_t* data) {
    std::cout << "Unimplemented SGB command Set Attribute Block!" << std::endl;
}

void SGBHandler::commandSetAttributeLine(uint8_t* data) {
    std::cout << "Unimplemented SGB command Set Attribute Line!" << std::endl;
}

void SGBHandler::commandSetAttributeDivide(uint8_t* data) {
    std::cout << "Unimplemented SGB command Set Attribute Divide!" << std::endl;
}

void SGBHandler::commandSetAttributeCharacter(uint8_t* data) {
    std::cout << "Unimplemeneted SGB command Set Attribute Character!" << std::endl;
}

//Unused commands between implementations here...

void SGBHandler::commandMultiplayerRegister(uint8_t* data) {
    std::cout << "Unimplemented SGB command Multiplayer Register!" << std::endl;
}

//Unused commands between implemenations here...

void SGBHandler::commandWindowMask(uint8_t* data) {
    std::cout << "Unimplemented SGB command Window Mask!" << std::endl;
}
