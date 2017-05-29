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
    m_totalPackets = 0;
    m_currentCommand = SGBCommand::SGB_COMMAND_UNDEFINED;
    m_color0 = COLOR_WHITE;

    m_colorPalettes = new RGBColor*[4];
    for (int paletteIndex = 0; paletteIndex < 4; paletteIndex++) {
        m_colorPalettes[paletteIndex] = new RGBColor[4];
        m_colorPalettes[paletteIndex][0] = COLOR_WHITE;
        m_colorPalettes[paletteIndex][1] = COLOR_LIGHTGRAY;
        m_colorPalettes[paletteIndex][2] = COLOR_DARKGRAY;
        m_colorPalettes[paletteIndex][3] = COLOR_BLACK;
    }
    
    m_systemPalettes = new RGBColor*[512];
    for(int paletteIndex = 0; paletteIndex < 512; paletteIndex++){
        m_systemPalettes[paletteIndex] = new RGBColor[4];
        m_systemPalettes[paletteIndex][0] = COLOR_WHITE;
        m_systemPalettes[paletteIndex][1] = COLOR_LIGHTGRAY;
        m_systemPalettes[paletteIndex][2] = COLOR_DARKGRAY;
        m_systemPalettes[paletteIndex][3] = COLOR_BLACK;        
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
    //std::cout << "Unimplemented Super Gameboy Handler Tick!" << std::endl;
}

void SGBHandler::sendPacketPulse(uint8_t pulse) {
    pulse &= 0x3;
    switch (pulse) {
        case PULSE_RESET:
            //std::cout << "SGB Reset pulse recieved" << std::endl;
            if(!bReceivingPacket){
                //TODO - proper indent if this fixes things
                if(m_currentPacketIndex != 0 && m_currentPacketIndex != 16){
                    std::cout << "Warning - previous packet may have been interrupted!" << std::endl;
                    std::cout << "Last accessed byte/bit before reset: " << +m_currentPacketIndex << "/" << +m_currentPacketBitIndex << std::endl;
                }
            
                //Reset packet variables.
                m_currentPacketIndex = 0;
                m_currentPacketBitIndex = 0;
                m_totalPackets = 0;
                
                //Ensure that all bytes are 0 since bits are ORed in.
                for(uint8_t index = 0; index < 16; index++){
                    m_currentPacket[index] = 0;
                }
                
                //Set receiving packet state to true.
                bReceivingPacket = true;
                
                //std::cout << "SGB has started receiving a packet!" << std::endl;
            } else {
                std::cout << "Warning - packet interrupted!" << std::endl;
                m_currentPacketIndex = 0;
                m_currentPacketBitIndex = 0;
                m_totalPackets = 0;
                
                //Ensure that all bytes are 0 since bits are ORed in.
                for(uint8_t index = 0; index < 16; index++){
                    m_currentPacket[index] = 0;
                }
                
            }
            break;
        case PULSE_END:
            //End pulse should be ignored.
            //std::cout << "End pulse" << std::endl;
            break;
        case PULSE_HIGH:
            //Bit 1
            if (bReceivingPacket) {
                //std::cout << "High pulse" << std::endl;
                //std::cout << "Recieved bit 1" << std::endl;
                appendBit(1);
            }
            break;
        case PULSE_LOW:
            if (bReceivingPacket) {
                //std::cout << "low pulse" << std::endl;
                //Check if we are at the end of the packet.
                //If so, this is a terminator. Otherwise, bit 0.
                if (m_currentPacketIndex > 15) {
                    //We have hit the stop bit. Parse the packet.
                    parseCurrentPacket();
                    bReceivingPacket = false;
                    //std::cout << "SGB has finished receiving a packet" << std::endl;
                } else {
                    //std::cout << "Recieved bit 0" << std::endl;
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
                    
                    if(m_framePaletteIndicies[tileX][tileY] < 0 || m_framePaletteIndicies[tileX][tileY] > 3){
                        std::cout << "Current frame palette index is out of bounds: " << +m_framePaletteIndicies[tileX][tileY] << std::endl;
                    }
                }
            }
        }
    }
}

void SGBHandler::appendBit(uint8_t bit) {
    //std::cout << "Append bit " << +m_currentPacketBitIndex << " to byte " << +m_currentPacketIndex << std::endl;
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

    //If remaining packets is 0, then this is the first packet in this sequence.
    if (m_remainingPackets == 0) {
        //std::cout << "Packet header: " << +m_currentPacket[0] << std::endl;
        //Number of packets is in first three bits of first byte.
        m_totalPackets = m_currentPacket[0] & 0x07;
        m_remainingPackets = m_totalPackets;
        
        //Output packets to confirm bit order of first byte. Should be 1 most of the time!
        std::cout << "Total packets: " << +m_remainingPackets << std::endl;
        
        //Dividing packet by 8 will give current command.
        m_currentCommand = (SGBCommand)(m_currentPacket[0]/8);
    }

    switch (m_currentCommand) {
        case SGB_COMMAND_PAL01:
            commandSetPalettes(0, 1, m_currentPacket);
            break;
            break;
        case SGB_COMMAND_PAL23:
            commandSetPalettes(2, 3, m_currentPacket);
            break;
            break;
        case SGB_COMMAND_PAL03:
            commandSetPalettes(0, 3, m_currentPacket);
            break;
        case SGB_COMMAND_PAL12:
            commandSetPalettes(1, 2, m_currentPacket);
            break;
        case SGB_COMMAND_ATTR_BLK:
            commandSetAttributeBlock(m_currentPacket);
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
        case SGB_COMMAND_SOUND:
            commandSound(m_currentPacket);
            break;
        case SGB_COMMAND_SOU_TRN:
            commandSoundTransfer(m_currentPacket);
            break;
        case SGB_COMMAND_PAL_SET:
            commandSetSGBPaletteIndirect(m_currentPacket);
            break;
        case SGB_COMMAND_PAL_TRN:
            commandSetSystemPalette(m_currentPacket);
            break;
        case SGB_COMMAND_ATRC_EN:
            commandSetAttractionMode(m_currentPacket);
            break;
        case SGB_COMMAND_TEST_EN:
            commandTest(m_currentPacket);
            break;
        case SGB_COMMAND_ICON_EN:
            commandIcon(m_currentPacket);
            break;
        case SGB_COMMAND_DATA_SND:
            commandSNESDataSend(m_currentPacket);
            break;
        case SGB_COMMAND_DATA_TRN:
            commandSNESDataTransfer(m_currentPacket);
            break;
        case SGB_COMMAND_MLT_REG:
            commandMultiplayerRegister(m_currentPacket);
            break;
        case SGB_COMMAND_JUMP:
            commandSNESJump(m_currentPacket);
            break;
        case SGB_COMMAND_CHR_TRN:
            commandCharacterTransfer(m_currentPacket);
            break;
        case SGB_COMMAND_PCT_TRN:
            commandScreenDataColorTransfer(m_currentPacket);
            break;
        case SGB_COMMAND_ATTR_TRN:
            commandAttributeTransfer(m_currentPacket);
            break;
        case SGB_COMMAND_ATTR_SET:
            commandSetAttribute(m_currentPacket);
            break;
        case SGB_COMMAND_MASK_EN:
            commandWindowMask(m_currentPacket);
            break;
        case SGB_COMMAND_OBJ_TRN:
            commandSNESObjTransfer(m_currentPacket);
            break;
        default:
            std::cout << "Unimiplemented SGB command " << +m_currentCommand << std::endl;
    }

    /*
    for(int index = 0; index < 16; index++){
            std::cout << "Packet byte " << +index << ": " << +m_currentPacket[index] << std::endl;
    }*/
    
    m_remainingPackets--;
    if(m_remainingPackets == 0){
        m_currentCommand = SGB_COMMAND_UNDEFINED;
        //std::cout << "Finished parsing packet..." << std::endl;;
    } else if (m_remainingPackets < 0){
        std::cout << "Parsed too many packets!" << std::endl;
    }
}

void SGBHandler::commandSetPalettes(uint8_t palette1Index, uint8_t palette2Index, uint8_t* data) {
    std::cout << "SGB command set palette " << +palette1Index << " " << +palette2Index << "!" << std::endl;
    
    uint16_t color0 = data[1] | (data[2] << 8);
    uint16_t color1 = data[3] | (data[4] << 8);
    uint16_t color2 = data[5] | (data[6] << 8);
    uint16_t color3 = data[7] | (data[8] << 8);
    uint16_t color4 = data[9] | (data[10] << 8);
    uint16_t color5 = data[11] | (data[12] << 8);
    uint16_t color6 = data[13] | (data[14] << 8);
    
    
    RGBColor color;
    color.r = (color0 & 0x1F) * 255/31;
    color.g = ((color0 & 0x3E0) >> 5) * 255/31;
    color.b = ((color0 & 0x7C00) >> 10) * 255/31;
    
    m_color0 = color;
    m_colorPalettes[palette1Index][0] = m_color0;
    m_colorPalettes[palette2Index][0] = m_color0;
    
    color.r = (color1 & 0x1F) * 255/31;
    color.g = ((color1 & 0x3E0) >> 5) * 255/31;
    color.b = ((color1 & 0x7C00) >> 10) * 255/31;
    
    m_colorPalettes[palette1Index][1] = color;
    
    color.r = (color2 & 0x1F) * 255/31;
    color.g = ((color2 & 0x3E0) >> 5) * 255/31;
    color.b = ((color2 & 0x7C00) >> 10) * 255/31;
    
    m_colorPalettes[palette1Index][2] = color;
    
    color.r = (color3 & 0x1F) * 255/31;
    color.g = ((color3 & 0x3E0) >> 5) * 255/31;
    color.b = ((color3 & 0x7C00) >> 10) * 255/31;
    
    m_colorPalettes[palette1Index][3] = color;
    
    color.r = (color4 & 0x1F) * 255/31;
    color.g = ((color4 & 0x3E0) >> 5) * 255/31;
    color.b = ((color4 & 0x7C00) >> 10) * 255/31;
    
    m_colorPalettes[palette2Index][1] = color;
    
    color.r = (color5 & 0x1F) * 255/31;
    color.g = ((color5 & 0x3E0) >> 5) * 255/31;
    color.b = ((color5 & 0x7C00) >> 10) * 255/31;
    
    m_colorPalettes[palette2Index][2] = color;
    
    color.r = (color6 & 0x1F) * 255/31;
    color.g = ((color6 & 0x3E0) >> 5) * 255/31;
    color.b = ((color6 & 0x7C00) >> 10) * 255/31;
    
    m_colorPalettes[palette2Index][3] = color;
}

void SGBHandler::commandSetAttributeBlock(uint8_t* data) {
    std::cout << "SGB command Set Attribute Block" << std::endl;
    static uint8_t dataSetCount = 0;
    
    uint8_t currentPacketIndex = 0;
    
    static uint8_t dataSetControlCode = 0;
    static uint8_t dataSetPalette = 0;
    static uint8_t dataSetCoordX1 = 0;
    static uint8_t dataSetCoordY1 = 0;
    static uint8_t dataSetCoordX2 = 0;
    static uint8_t dataSetCoordY2 = 0;
    static uint8_t dataSetCurrentByte = 0;
    
    //Check if this is the first packet so paramters can be set. 
    if(m_remainingPackets == m_totalPackets){
        dataSetCount = data[1];
        
        //For first byte, data starts on third byte. Otherwise, first byte.
        currentPacketIndex = 2;
        
        std::cout << "Packet dataset count: " << +dataSetCount << std::endl;
    }
    
    while(currentPacketIndex < 16 && dataSetCount > 0){
        switch(dataSetCurrentByte){
            case 0:
                dataSetControlCode = data[currentPacketIndex];
                dataSetCurrentByte++;
                break;
            case 1:
                dataSetPalette = data[currentPacketIndex];
                dataSetCurrentByte++;
                break;
            case 2:
                dataSetCoordX1 = data[currentPacketIndex];
                dataSetCurrentByte++;
                break;
            case 3:
                dataSetCoordY1 = data[currentPacketIndex];
                dataSetCurrentByte++;
                break;
            case 4:
                dataSetCoordX2 = data[currentPacketIndex];
                dataSetCurrentByte++;
                break;
            case 5:
                dataSetCoordY2 = data[currentPacketIndex];
                dataSetCurrentByte = 0;
                dataSetCount--;
                std::cout << "outside color: " << +((dataSetPalette >> 4) & 0x3) << std::endl;
                std::cout << "border color: " << +((dataSetPalette >> 2) & 0x3) << std::endl;;
                std::cout << "inside color: " << +((dataSetPalette) & 0x3) << std::endl;;
                for(uint8_t coordX = 0; coordX < FRAMEBUFFER_WIDTH/TILE_WIDTH; coordX++){
                    for(uint8_t coordY = 0; coordY < FRAMEBUFFER_HEIGHT/TILE_HEIGHT; coordY++){
                        //Set palette for outside the border
                        if((dataSetControlCode & 0x04) && 
                            (coordX < dataSetCoordX1 || coordX > dataSetCoordX2) && 
                            (coordY < dataSetCoordY1 || coordY > dataSetCoordY2)){
                            m_framePaletteIndicies[coordX][coordY] = (dataSetPalette >> 4) & 0x3;
                        } else if ((dataSetControlCode & 0x01) &&
                            (coordX > dataSetCoordX1 || coordX < dataSetCoordX2) && 
                            (coordY > dataSetCoordY1 || coordY < dataSetCoordY2)){
                            m_framePaletteIndicies[coordX][coordY] = (dataSetPalette & 0x3);
                        } else {
                            //If the inside or outside are the only colors set, the border should get that color as well. Otherwise, respect change border flag.
                            if(dataSetControlCode & 0x02){
                                m_framePaletteIndicies[coordX][coordY] = (dataSetPalette >> 2) & 0x3;
                            } else if ((dataSetControlCode & 0x04) && !(dataSetControlCode & 0x01)){
                                m_framePaletteIndicies[coordX][coordY] = (dataSetPalette >> 4) & 0x3;
                            } else if (!(dataSetControlCode & 0x04) && (dataSetControlCode & 0x01)){
                                m_framePaletteIndicies[coordX][coordY] = (dataSetPalette & 0x3);
                            }
                        }
                    }
                }
                break;
        }
        currentPacketIndex++;
    }
}

void SGBHandler::commandSetAttributeLine(uint8_t* data) {
    std::cout << "SGB command Set Attribute Line" << std::endl;
    
    static uint8_t dataSetCount = 0;
    
    uint8_t currentPacketIndex = 0;
    
    //Check if this is the first packet so paramters can be set. 
    if(m_remainingPackets == m_totalPackets){
        dataSetCount = data[1];
        
        //For first byte, data starts on third byte. Otherwise, first byte.
        currentPacketIndex = 2;
    }
    
    while(currentPacketIndex < 16 && dataSetCount > 0){
        uint8_t lineNumber = data[currentPacketIndex] & 0x0F;
        uint8_t paletteNumber = (data[currentPacketIndex] >> 5) & 0x3;
        bool horizontalLine = data[currentPacketIndex] >> 7;
        
        if(horizontalLine){
            for(uint8_t coordX = 0; coordX < FRAMEBUFFER_WIDTH/TILE_WIDTH; coordX++){
                m_framePaletteIndicies[coordX][lineNumber] = paletteNumber;
            }
        } else {
            for(uint8_t coordY = 0; coordY < FRAMEBUFFER_HEIGHT/TILE_HEIGHT; coordY++){
                m_framePaletteIndicies[lineNumber][coordY] = paletteNumber;
            }
        }
        
        dataSetCount--;
        currentPacketIndex++;
    }
}

void SGBHandler::commandSetAttributeDivide(uint8_t* data) {
    std::cout << "SGB command Set Attribute Divide" << std::endl;
    
    uint8_t palette1 = data[1] & 0x3;
    uint8_t palette2 = (data[1] >> 2) & 0x3;
    uint8_t paletteLine = (data[1] >> 4) & 0x3;
    bool verticalSplit = (data[1] >> 6) & 0x1;
    uint8_t splitCoordinate = data[2];
    
    for(uint8_t coordX = 0; coordX < FRAMEBUFFER_WIDTH/TILE_WIDTH; coordX++){
        for(uint8_t coordY = 0; coordY < FRAMEBUFFER_HEIGHT/TILE_HEIGHT; coordY++){
            uint8_t compareCoord = (verticalSplit ? coordY : coordX);
            if(compareCoord < splitCoordinate){
                m_framePaletteIndicies[coordX][coordY] = palette2;
            } else if (compareCoord > splitCoordinate){
                m_framePaletteIndicies[coordX][coordY] = palette1;
            } else {
                m_framePaletteIndicies[coordX][coordY] = paletteLine;
            }
        }
    }
}

void SGBHandler::commandSetAttributeCharacter(uint8_t* data) {
    std::cout << "Unimplemeneted SGB command Set Attribute Character!" << std::endl;
}

void SGBHandler::commandSound(uint8_t* data){
    std::cout << "Unimplemented SGB command Sound!" << std::endl;
}
        
void SGBHandler::commandSoundTransfer(uint8_t* data){
    std::cout << "Unimplemented SGB command Sound Transfer!" << std::endl;
}

//PAL_SET        
void SGBHandler::commandSetSGBPaletteIndirect(uint8_t* data){
    std::cout << "SGB command Set SGB Palette Indirect" << std::endl;
    
    uint16_t palettes[4];
    
    palettes[0] = data[1] | (((uint16_t)data[2] << 8) & 0x01FF);
    palettes[1] = data[3] | (((uint16_t)data[4] << 8) & 0x01FF);
    palettes[2] = data[5] | (((uint16_t)data[6] << 8) & 0x01FF);
    palettes[3] = data[7] | (((uint16_t)data[8] << 8) & 0x01FF);
    
    
    uint8_t attributeFileInfo = data[9];
    
    if(attributeFileInfo & 0x80){
        std::cout << "Packet uses unimplemented attribute file!" << std::endl;
    }
    
    if(attributeFileInfo & 0x40){
        std::cout << "Packet uses unimplemented cancel mask!" << std::endl;
    }
    
    std::cout << "Palettes: " << +palettes[0] << " " << +palettes[1] << " " << +palettes[2] << " " << +palettes[3] << " " << std::endl;
    
    for(uint8_t paletteIndex = 0; paletteIndex < 4; paletteIndex++){
        std::cout << "Palette: " << +palettes[paletteIndex] << std::endl;
        for(uint8_t subIndex = 0; subIndex < 4; subIndex++){
            std::cout << "Color " << +subIndex << ": " << +m_systemPalettes[palettes[paletteIndex]][subIndex].r << " " << +m_systemPalettes[palettes[paletteIndex]][subIndex].g << " " << +m_systemPalettes[palettes[paletteIndex]][subIndex].b << std::endl;
        }
    }
    
    //m_color0 = m_systemPalettes[palettes[0]][0];
    
    m_colorPalettes[0][0] = m_systemPalettes[palettes[0]][0];
    m_colorPalettes[0][1] = m_systemPalettes[palettes[0]][1];
    m_colorPalettes[0][2] = m_systemPalettes[palettes[0]][2];
    m_colorPalettes[0][3] = m_systemPalettes[palettes[0]][3];
    
    m_colorPalettes[1][0] = m_systemPalettes[palettes[1]][0];
    m_colorPalettes[1][1] = m_systemPalettes[palettes[1]][1];
    m_colorPalettes[1][2] = m_systemPalettes[palettes[1]][2];
    m_colorPalettes[1][3] = m_systemPalettes[palettes[1]][3];
    
    m_colorPalettes[2][0] = m_systemPalettes[palettes[2]][0];
    m_colorPalettes[2][1] = m_systemPalettes[palettes[2]][1];
    m_colorPalettes[2][2] = m_systemPalettes[palettes[2]][2];
    m_colorPalettes[2][3] = m_systemPalettes[palettes[2]][3];
    
    m_colorPalettes[3][0] = m_systemPalettes[palettes[3]][0];
    m_colorPalettes[3][1] = m_systemPalettes[palettes[3]][1];
    m_colorPalettes[3][2] = m_systemPalettes[palettes[3]][2];
    m_colorPalettes[3][3] = m_systemPalettes[palettes[3]][3];
    
    /*
    for(uint8_t colorPaletteIndex = 0; colorPaletteIndex < 4; colorPaletteIndex++){
        uint16_t currentSystemPalette = palettes[colorPaletteIndex];
        for(uint8_t subIndex = 0; subIndex < 4; subIndex++){            
            m_colorPalettes[colorPaletteIndex][subIndex] = m_systemPalettes[currentSystemPalette][subIndex];
        }
        
        m_color0 = m_colorPalettes[colorPaletteIndex][0];
    }*/
}
        
void SGBHandler::commandSetSystemPalette(uint8_t* data){
    std::cout << "SGB command Set System Palette" << std::endl;
    
    //SGB needs raw bytes, but LCD emulation converts to colors directly. Need to recreate bytes from B&W frame.
    RGBColor** currentFrame = m_lcd->getCompleteFrame();
    uint8_t rawFrameBytes[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT * 2];
    uint16_t currentByte = 0;
    for(uint8_t tileX = 0; tileX < FRAMEBUFFER_WIDTH/TILE_WIDTH; tileX++){
        for(uint8_t tileY = 0; tileY < FRAMEBUFFER_HEIGHT/TILE_HEIGHT; tileY++){
            uint8_t byte1 = 0;
            uint8_t byte2 = 0;
            
            for(uint8_t pixelY = 0; pixelY < 8; pixelY++){
                for(uint8_t pixelX = 0; pixelX < 8; pixelX++){
                    RGBColor currentPixel = currentFrame[(tileX * 8) + pixelX][(tileY * 8) +  pixelY];
                    uint8_t rawPixel = 0;
                    
                    if(currentPixel.equals(COLOR_WHITE)){
                        rawPixel = 0;
                    } else if (currentPixel.equals(COLOR_LIGHTGRAY)){
                        rawPixel = 1;
                    } else if (currentPixel.equals(COLOR_DARKGRAY)){
                        rawPixel = 2;
                    } else if (currentPixel.equals(COLOR_BLACK)){
                        rawPixel = 3;
                    }
                    
                    byte1 |= (rawPixel & 1) << (7 - pixelX);
                    byte2 |= ((rawPixel & 2) >> 1) << (7 - pixelX);
                }
                
                rawFrameBytes[currentByte] = byte1;
                currentByte++;
                rawFrameBytes[currentByte] = byte2;
                currentByte++;
                byte1 = 0;
                byte2 = 0;
            }            
        }
    }
    
    currentByte = 0;
    for(uint16_t paletteIndex = 0; paletteIndex < 512; paletteIndex++){
        
        uint16_t color0 = rawFrameBytes[currentByte];
        currentByte++;
        color0 |= ((uint16_t)rawFrameBytes[currentByte] << 8) & 0x7F00;
        currentByte++;
        
        uint16_t color1 = rawFrameBytes[currentByte];
        currentByte++;
        color1 |= ((uint16_t)rawFrameBytes[currentByte] << 8) & 0x7F00;
        currentByte++;
        
        uint16_t color2 = rawFrameBytes[currentByte];
        currentByte++;
        color2 |= ((uint16_t)rawFrameBytes[currentByte] << 8) & 0x7F00;
        currentByte++;
        
        uint16_t color3 = rawFrameBytes[currentByte];
        currentByte++;
        color3 |= ((uint16_t)rawFrameBytes[currentByte] << 8) & 0x7F00;
        currentByte++;
        
        RGBColor color;
        
        color.r = (color0 & 0x001f) * (255/31);
        color.g = ((color0 & 0x03E0) >> 5) * (255/31);
        color.b = ((color0 & 0x7C00) >> 10) * (255/31);
        
        m_systemPalettes[paletteIndex][0] = color;
        
        color.r = (color1 & 0x001f) * (255/31);
        color.g = ((color1 & 0x03E0) >> 5) * (255/31);
        color.b = ((color1 & 0x7C00) >> 10) * (255/31);
        
        m_systemPalettes[paletteIndex][1] = color;
        
        color.r = (color2 & 0x001f) * (255/31);
        color.g = ((color2 & 0x03E0) >> 5) * (255/31);
        color.b = ((color2 & 0x7C00) >> 10) * (255/31);
        
        m_systemPalettes[paletteIndex][2] = color;
        
        color.r = (color3 & 0x001f) * (255/31);
        color.g = ((color3 & 0x03E0) >> 5) * (255/31);
        color.b = ((color3 & 0x7C00) >> 10) * (255/31);
        
        m_systemPalettes[paletteIndex][3] = color;
    }
    
    //Debug - show 0th system palette.
    /*
    std::cout << "Debug - system palette 0: " << std::endl;
    for(uint8_t colorIndex = 0; colorIndex < 4; colorIndex++){
        std::cout << "Color " << +colorIndex << ": " << +m_systemPalettes[0][colorIndex].r << " " << +m_systemPalettes[0][colorIndex].g << " " << +m_systemPalettes[0][colorIndex].b << " "  << std::endl;
    }
    
    std::cout << "Debug - printing ALL of vram!" << std::endl;
    for(uint16_t address = VRAM_START; address < VRAM_END; address++){
        std::cout << +address << ": " << +m_mem->direct_read(address) << std::endl;
    }*/
}

void SGBHandler::commandSetAttractionMode(uint8_t* data){
    std::cout << "SGB command Set Attraction Mode. Ignoring." << std::endl;
}
        
void SGBHandler::commandTest(uint8_t* data){
    std::cout << "USGB command Test. Ignoring." << std::endl;
}
        
void SGBHandler::commandIcon(uint8_t* data){
    std::cout << "SGB command Icon. Ignoring." << std::endl;
}
        
//SNES Data Send will usually be called 8 times by games for setup. Can safely ignore.
void SGBHandler::commandSNESDataSend(uint8_t* data){
    std::cout << "SGB command SNES Data Send. Ignoring." << std::endl;
}
        
void SGBHandler::commandSNESDataTransfer(uint8_t* data){
    std::cout << "SNES Data Transfer. Ignoring" << std::endl;
}


void SGBHandler::commandMultiplayerRegister(uint8_t* data) {
    //std::cout << "Unimplemented SGB command Multiplayer Register!" << std::endl;
    std::cout << "SGB Command Multiplayer Request" << std::endl;
    switch(data[1]){
        case MLT_REQ_ONE:
            m_pad->setPlayerCount(1);
            break;
        case MLT_REQ_TWO:
            m_pad->setPlayerCount(2);
            break;
        case MLT_REQ_FOUR:
            m_pad->setPlayerCount(4);
            break;
        default:
            std::cout << "Unrecognized player count for multiplayer request: " << +data[1] << std::endl;
            m_pad->setPlayerCount(1);
    }
}

void SGBHandler::commandSNESJump(uint8_t* data){
    //Since Yagbe does not include SNES emulation, this command cannot be used.
    std::cout << "SGB command SNES Jump!" << std::endl;
}
        
void SGBHandler::commandCharacterTransfer(uint8_t* data){
    //Yagbe does not support SGB borders.
    std::cout << "SGB command Character Transfer. Ignoring." << std::endl;
}
        
void SGBHandler::commandScreenDataColorTransfer(uint8_t* data){
    //Yagbe does not support SGB borders.
    std::cout << "SGB command Screen Data Color Transfer. Ignoring." << std::endl;
}
        
void SGBHandler::commandAttributeTransfer(uint8_t* data){
    std::cout << "Unimplemented SGB command Attribute Transfer!" << std::endl;
    std::cin.get();
}
        
void SGBHandler::commandSetAttribute(uint8_t* data){
    std::cout << "Unimplemented SGB command Set Attribute!" << std::endl;
    std::cin.get();
}

void SGBHandler::commandWindowMask(uint8_t* data) {
    std::cout << "Unimplemented SGB command Window Mask!" << std::endl;
}

void SGBHandler::commandSNESObjTransfer(uint8_t* data){
    std::cout << "Unimplemented SGB command SNES Object Transfer!" << std::endl;
}
