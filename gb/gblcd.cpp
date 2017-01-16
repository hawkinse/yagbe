#include <iostream>
#include <cstring> //Needed for memset
#include <math.h>
#include "gblcd.h"
#include "gbmem.h"
#include "bytehelpers.h"

GBLCD::GBLCD(GBMem* mem){
    m_gbmemory = mem;
    
    //Clear pointers so we don't risk pre-existing garbage triggering a buffer update
    m_displayRenderer = NULL;
    
    m_Frames = 0;
    m_Framebuffer0 = new RGBColor*[FRAMEBUFFER_WIDTH];
    m_Framebuffer1 = new RGBColor*[FRAMEBUFFER_WIDTH];
    for(int col = 0; col < FRAMEBUFFER_WIDTH; col++){
        m_Framebuffer0[col] = new RGBColor[FRAMEBUFFER_HEIGHT];
        m_Framebuffer1[col] = new RGBColor[FRAMEBUFFER_HEIGHT];
    }
    
    if(!m_gbmemory->getBootRomEnabled()){
        setSTAT(0x85);
        setLCDC(0x91);
        setBGPalette(0xFC);
        setSpritePalette0(0xFF);
        setSpritePalette1(0xFF);
    } else {
        setSTAT(0);
        setLCDC(0);
    }   
    
    //Swap buffers to ensure screen is cleared at startup
    swapBuffers();
}

GBLCD::~GBLCD(){
    //TODO - properly delete each line in the arrays in addition to base pointer
    delete m_Framebuffer0;
    delete m_Framebuffer1;
}

void GBLCD::tick(long long hz){    
    //Store any unused Hz so that we can factor it into the next update
    static long long timeRollover = 0;
    
    //Include any previous clock unused from the last update
    hz += timeRollover;
    
    uint8_t currentLCDMode = getSTAT() & STAT_MODE_FLAG;
    
    bool bTimeForTasks = true; //TODO - rename this to something more elegant
    while(bTimeForTasks){
        //Figure out how much time we need to execute the mode
        int currentModeTimeLength = 0;
        
        
        switch(currentLCDMode){
            case STAT_MODE0_HBLANK:
                currentModeTimeLength = CYCLES_LCD_MODE0;
                break;
            case STAT_MODE1_VBLANK:
                currentModeTimeLength = CYCLES_VBLANK_LYINCREMENT_INTERVAL/*CYCLES_VBLANK_LENGTH*/;
                break;
            case STAT_MODE2_OAM:
                currentModeTimeLength = CYCLES_LCD_MODE2;
                break;
            case STAT_MODE3_TRANSFER:
                currentModeTimeLength = CYCLES_LCD_MODE3;
                break;
        }
        
        uint8_t currentSTAT = getSTAT();
        
        //If we have time, perform needed logic and advance mode
        if(hz > currentModeTimeLength){            
            switch(currentLCDMode){                
                case STAT_MODE0_HBLANK:
                    performHBlank();
                    
                    currentSTAT &= 0xFC;
                    
                    if(getLY() >= 144){
                        currentSTAT |= STAT_MODE1_VBLANK;
                        performVBlank();
                    } else {
                        currentSTAT |= STAT_MODE2_OAM;
                    }
                    
                    //Set STAT directly so we can write the bottom three bits
                    m_gbmemory->direct_write(ADDRESS_STAT, currentSTAT);
                    
                    break;
                case STAT_MODE1_VBLANK:
                    static int LYIncrementCount = 0;
                    if(LYIncrementCount == 0){                        
                        incrementLY();
                        
                        //Fire event if enabled and coincidence is hit
                        if((currentSTAT & STAT_COINCIDENCE_INTERRUPT) && (getLY() == getLYC())){
                            uint8_t interruptFlags = m_gbmemory->direct_read(ADDRESS_IF);
                            m_gbmemory->write(ADDRESS_IF, interruptFlags | INTERRUPT_FLAG_STAT);
                        }
                        
                        LYIncrementCount++;
                    } else if (LYIncrementCount >= VBLANK_LYINCREMENT_COUNT){
                        //VBlank is followed by mode 2
                        currentSTAT &= ~STAT_MODE1_VBLANK;
                        currentSTAT |= STAT_MODE2_OAM;
                        LYIncrementCount = 0;
                        m_gbmemory->direct_write(ADDRESS_LY, 0);
                    } else {
                        incrementLY();
                        
                        //Fire event if enabled and coincidence is hit
                        if((currentSTAT & STAT_COINCIDENCE_INTERRUPT) && (getLY() == getLYC())){
                            uint8_t interruptFlags = m_gbmemory->direct_read(ADDRESS_IF);
                            m_gbmemory->write(ADDRESS_IF, interruptFlags | INTERRUPT_FLAG_STAT);
                        }
                        
                        LYIncrementCount++;
                    }
                    
                    //Set STAT directly so we can write the bottom three bits
                    m_gbmemory->direct_write(ADDRESS_STAT, currentSTAT);
                    
                    if(getLY() > 153){
                        if(CONSOLE_OUTPUT_ENABLED) std::cout << "WARNING - LY IS GREATER THAN 153! " << +getLY() << std::endl;
                        
                        m_gbmemory->direct_write(ADDRESS_LY, 153);
                    }
                    
                    break;
                case STAT_MODE2_OAM:
                    performOAMSearch();
                    
                    currentSTAT &= 0xFC;
                    
                    //OAM search is followed by mode 3
                    currentSTAT &= ~STAT_MODE2_OAM;
                    currentSTAT |= STAT_MODE3_TRANSFER;
                    
                    //Set STAT directly so we can write the bottom three bits
                    m_gbmemory->direct_write(ADDRESS_STAT, currentSTAT);
                    break;
                case STAT_MODE3_TRANSFER:
                    performOAMTransfer();
                    
                    currentSTAT &= 0xFC;
                    
                    //Transfer is followed by mode 0
                    currentSTAT &= ~STAT_MODE3_TRANSFER;
                    currentSTAT |= STAT_MODE0_HBLANK;
                    
                    //Set STAT directly so we can write the bottom three bits
                    m_gbmemory->direct_write(ADDRESS_STAT, currentSTAT);
                    break;
            }
            
            currentLCDMode = currentSTAT & STAT_MODE_FLAG;
            
            hz -= currentModeTimeLength;
        } else {
            bTimeForTasks = false;
        }
    }
    
    //Store positive remaining time
    timeRollover = (hz > 0) ? hz : 0;
}

void GBLCD::performHBlank(){
    
    if(getLY() <= 144){
		
		//Fire event if enabled and coincidence is hit
        if((getSTAT() & STAT_COINCIDENCE_INTERRUPT) && (getLY() == getLYC())){
            uint8_t interruptFlags = m_gbmemory->direct_read(ADDRESS_IF);
            m_gbmemory->write(ADDRESS_IF, interruptFlags | INTERRUPT_FLAG_STAT);
        }
		
        //Render line
        renderLine();
        
        //Set line for next blank
        incrementLY();
    }
}

void GBLCD::performVBlank(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "VBlank" << std::endl;
    
    swapBuffers();
    
    //Update background map display
    if(m_displayRenderer){
        m_displayRenderer->update(getCompleteFrame(), FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
    }
    
    //Fires event for main vblank interrupt
    m_gbmemory->write(ADDRESS_IF, m_gbmemory->direct_read(ADDRESS_IF) | INTERRUPT_FLAG_VBLANK);
    
    //Fires event for vblank stat interrupt
    if(getSTAT() & STAT_MODE1_VBLANK_INTERRUPT){
        uint8_t interruptFlags = m_gbmemory->direct_read(ADDRESS_IF);
        m_gbmemory->write(ADDRESS_IF, interruptFlags | INTERRUPT_FLAG_STAT);
    }
}

void GBLCD::performOAMSearch(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "OAM Search" << std::endl;
    
    //Fire event if enabled (bit
    if(getSTAT() & STAT_MODE2_OAM_INTERRUPT){
        uint8_t interruptFlags = m_gbmemory->direct_read(ADDRESS_IF);
        m_gbmemory->write(ADDRESS_IF, interruptFlags | INTERRUPT_FLAG_STAT);
    }
}

void GBLCD::performOAMTransfer(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "OAM Transfer" << std::endl;
}

void GBLCD::incrementLY(){
    m_gbmemory->direct_write(ADDRESS_LY, getLY() + 1);
}

RGBColor GBLCD::getColor(uint8_t palette, uint8_t colorIndex){
    RGBColor toReturn;
    
    uint8_t colorPalette[4];
    colorPalette[0] = palette & 0x03;
    colorPalette[1] = (palette >> 2) & 0x03;
    colorPalette[2] = (palette >> 4) & 0x03;
    colorPalette[3] = (palette >> 6) & 0x03;
    
    
    switch(colorPalette[colorIndex]){
        case PALETTE_BW_WHITE:
            toReturn = COLOR_WHITE;
            break;
        case PALETTE_BW_LIGHTGRAY:
            toReturn = COLOR_LIGHTGRAY;
            break;
        case PALETTE_BW_DARKGRAY:
            toReturn = COLOR_DARKGRAY;
            break;
        case PALETTE_BW_BLACK:
            toReturn = COLOR_BLACK;
            break;
        default:
            if(CONSOLE_OUTPUT_ENABLED) std::cout << "Unrecognized color: " << colorIndex << std::endl;
            break;
    }
    
    //The 0th color index indicates either a transparent sprite color or a background color which doesn't block transparent sprites.
    if(colorIndex == 0){
        toReturn.transparent = true;
    }
    
    return toReturn;
}

void GBLCD::updateBackgroundLine(RGBColor** frameBuffer){    
    int bgPixelY = (getLY() + getScrollY()) % (BACKGROUND_MAP_HEIGHT * TILE_HEIGHT);
    
    uint16_t tileLocation = (getLCDC() & LCDC_BG_TILE_MAP_DISPLAY_SELECT) ? BG_TILE_MAP_DISPLAY_1 : BG_TILE_MAP_DISPLAY_0;
    int tileLocOffset = bgPixelY / TILE_WIDTH * BACKGROUND_MAP_WIDTH;
    
    //Store leftmost tile for when we need to wrap around
    int leftmostTileLoc = tileLocation + tileLocOffset;
    
    //Add X offset to tile offset
    tileLocOffset += (getScrollX() / TILE_HEIGHT) % BACKGROUND_MAP_WIDTH;
    
    tileLocation += tileLocOffset;
    
    int bgPixelX = 0;
    bool bScrolledTileDrawn = false;
    while(bgPixelX < FRAMEBUFFER_WIDTH){
        uint8_t rawTileIndex = m_gbmemory->direct_read(tileLocation);
        int tileIndex = 0;
        uint16_t tilePatternAddress = 0;
        
        //Shift from 0-128 to -128-127 based on tile location select
        if(getLCDC() & LCDC_BG_WINDOW_TILE_SELECT){            
            tileIndex = rawTileIndex;
            tilePatternAddress = TILE_PATTERN_TABLE_1;
        } else {
            tileIndex = reinterpret_cast<int8_t &>(rawTileIndex);
            tilePatternAddress = TILE_PATTERN_TABLE_0_TILE_0;
        }
        
        getTileLine(m_TempTile, tilePatternAddress, tileIndex, (bgPixelY % TILE_HEIGHT));
        
        for(int tileX = (bScrolledTileDrawn ? 0 : (getScrollX() % TILE_WIDTH)); tileX < TILE_WIDTH; tileX++){
            if(bgPixelX  >= FRAMEBUFFER_WIDTH) break;
            frameBuffer[bgPixelX][getLY()] = getColor(getBGPalette(), m_TempTile[tileX]);
            
            bgPixelX++;
        }
        
        if(!bScrolledTileDrawn){
            bScrolledTileDrawn = true;
        }
        
        //Increment tile location, wrapping around if tile is at edge of background map
        tileLocation = (tileLocation + 1 < leftmostTileLoc + BACKGROUND_MAP_WIDTH) ? (tileLocation + 1) : leftmostTileLoc;
    }
}

void GBLCD::updateWindowLine(RGBColor** frameBuffer){    
    if(getWindowX() >= 0 && getWindowX() < FRAMEBUFFER_WIDTH){
        if(getWindowY() <= getLY() /*&& getWindowY() < FRAMEBUFFER_HEIGHT*/){
            uint16_t tileLocation = (getLCDC() & LCDC_WINDOW_TILE_MAP_DISPLAY_SELECT) ? BG_TILE_MAP_DISPLAY_1 : BG_TILE_MAP_DISPLAY_0;
            
            //Set tile location offset. Don't need to worry about X offset since we are always starting from leftmost tile?
            //Even though the Window can only show the size of the framebuffer, the map is still the size of the background map since they share the same location in memory
            tileLocation += ((getLY() - getWindowY()) / TILE_WIDTH * BACKGROUND_MAP_WIDTH);
            tileLocation += ((getWindowX()) / TILE_WIDTH);
            
            //std::cout << "Window is shown. Tile loc: " << +tileLocation << std::endl;
            //std::cout << "LY - window Y: " << +(getLY() - getWindowY()) << std::endl;
            
            for(int bgPixelX = getWindowX(); bgPixelX < FRAMEBUFFER_WIDTH; bgPixelX += TILE_WIDTH){
                uint8_t rawTileIndex = m_gbmemory->direct_read(tileLocation);
                int tileIndex = 0;
                uint16_t tilePatternAddress = 0;
                
                if(getLCDC() & LCDC_BG_WINDOW_TILE_SELECT){            
                    tileIndex = rawTileIndex;
                    tilePatternAddress = TILE_PATTERN_TABLE_1;
                } else {
                    tileIndex = reinterpret_cast<int8_t &>(rawTileIndex);
                    tilePatternAddress = TILE_PATTERN_TABLE_0_TILE_0;
                }
                
                getTileLine(m_TempTile, tilePatternAddress, tileIndex, (getLY() - getWindowY()) % 8);
                
                for(int tileX = 0; tileX < TILE_WIDTH && (tileX + bgPixelX) < FRAMEBUFFER_WIDTH; tileX++){
                    //Tiles are stored with the columns and rows reversed
                    int colorIndex = m_TempTile[tileX];
            
                    if(bgPixelX + tileX < FRAMEBUFFER_WIDTH){
                        frameBuffer[(bgPixelX + tileX)][getLY()] = getColor(getBGPalette(), colorIndex);
                    } else {
                        break;
                    }
                }
                
                tileLocation++;
            }
        }
    }
}

//Updates the sprites for the line indicated by LY
void GBLCD::updateLineSprites(RGBColor** frameBuffer){
    //return;
    uint8_t spriteXPos = 0;
    uint8_t spriteYPos = 0;
    uint8_t spriteTileNum = 0;
    uint8_t spriteFlags = 0;
    bool bXFlip = false;
    bool bYFlip = false;
    
    //Whether or not we are in 8x16 sprite mode
    bool bDoubleHeight = (getLCDC() & LCDC_SPRITE_SIZE);
    
    //Loop through all sprite attributes
    for(uint16_t spriteAttributeAddress = OAM_START; spriteAttributeAddress <= OAM_END; spriteAttributeAddress += 4){
        //Get sprite attributes
        spriteYPos = m_gbmemory->direct_read(spriteAttributeAddress);
        spriteXPos = m_gbmemory->direct_read(spriteAttributeAddress + 1);
        spriteTileNum = m_gbmemory->direct_read(spriteAttributeAddress + 2);
        spriteFlags = m_gbmemory->direct_read(spriteAttributeAddress + 3);
        bXFlip = (spriteFlags & SPRITE_ATTRIBUTE_XFLIP) ? true : false;
        bYFlip = (spriteFlags & SPRITE_ATTRIBUTE_YFLIP) ? true : false;
        
        int realXPos = spriteXPos - SPRITE_X_OFFSET;
        int realYPos = spriteYPos - SPRITE_Y_OFFSET;
        
        uint8_t palette = (spriteFlags & SPRITE_ATTRIBUTE_PALLETE) ? getSpritePalette1() : getSpritePalette0();
        
        //Check if sprite is actually on screen
        //TODO - get a real fix for xPos being off
        if((realXPos > /*0*/-7 && realXPos < 168)){
            if((realYPos >= getLY() - (bDoubleHeight ? 15 : 7) && realYPos <= getLY())){
                int tileYLine = (getLY() - realYPos) % (TILE_HEIGHT * 2);
                int tileYSpriteLine = (getLY() - realYPos) % TILE_HEIGHT;
                
                if(bDoubleHeight && ((tileYLine > 7 && !bYFlip) || (tileYLine <= 7 & bYFlip))){
                    spriteTileNum++;
                } 
                
                if(bYFlip){
                    tileYSpriteLine = 7 - tileYSpriteLine;
                }
                
                getTileLine(m_TempTile, TILE_PATTERN_TABLE_1, spriteTileNum, tileYSpriteLine);
                
                for(int tileX = 0; tileX < TILE_WIDTH; tileX++){
                    //std::cout << "Rendering sprite pixel" << std::endl;
                    //std::cin.get();
                    int renderPosX = realXPos + tileX;
                    if(renderPosX > 0 && renderPosX < FRAMEBUFFER_WIDTH){
                        RGBColor pixel = getColor(palette, m_TempTile[(bXFlip ? (7 - tileX) : tileX)]);
                         
                        if(!pixel.transparent){
                            if(!(spriteFlags & SPRITE_ATTRIBUTE_BGPRIORITY) || frameBuffer[renderPosX][getLY()].transparent){
                                frameBuffer[renderPosX][getLY()] = pixel;
                            }
                        }
                        
                    }
                }
            }
        }
    }
    
}
        
//Renders the current line indicated by LY
void GBLCD::renderLine(){
    RGBColor** buffer = getNextUnfinishedFrame();
    
    //Check if background is enabled and render if so.
    if((getLCDC() & LCDC_BG_DISPLAY)){ 
        updateBackgroundLine(buffer);
    }
    
    //Check if the window is enabled and render if so
    if(getLCDC() & LCDC_WINDOW_DISPLAY_ENABLE){
        updateWindowLine(buffer);
    }
    
    //Check if sprites are enabled and render if so
    if(getLCDC() & LCDC_SPRITE_DISPLAY_ENABLE){
        //Render sprites.
        updateLineSprites(buffer);
    }
}

//Gets an 8 pixel line of tiles for the given tile index as an array of palette indicies
//tileIndex is a value from 0 to 255 or -128 to 127. 
void GBLCD::getTileLine(uint8_t* out, uint16_t tilePatternAddress, int tileIndex, int line){ 
    //Calculate the address of the tile's starting byte + offset for the current line.
    int tileAddress = tilePatternAddress;  //TILE_PATTERN_TABLE_0;
    tileAddress += (tileIndex * TILE_BYTES);
    tileAddress += (line * 2);
    
    //The first byte contains the least significant bits of the color palette index.
    //Second contains most significant.
    uint8_t leastSignificantColors = m_gbmemory->direct_read(tileAddress);
    uint8_t mostSignificantColors = m_gbmemory->direct_read(tileAddress + 1);
    
    //Combine the two bits for each pixel in the output
    for(int colorIndex = 0; colorIndex < TILE_WIDTH; colorIndex++){
        //Bit 7 is leftmost pixel in the color bytes
        out[colorIndex] = ((leastSignificantColors >> (TILE_WIDTH - colorIndex - 1)) & 1);
        out[colorIndex] |= ((mostSignificantColors >> (TILE_WIDTH - colorIndex - 1)) & 1) << 1;
        if(out[colorIndex] > 3){
            if(CONSOLE_OUTPUT_ENABLED) std::cout << "Warning - tile at " << +tileAddress << " has a color index greater than 3!" << std::endl;
        }        
    }
}

//TODO - convert this to uint8_t** format
//Gets the full 8x8 tile at the given index
void GBLCD::getTile(uint8_t** out, uint16_t tilePatternAddress, int tileIndex){
    //Create an array with axis swapped, so we can directly use getTileLine
    uint8_t** axisSwapped = new uint8_t*[TILE_HEIGHT];
    for(int axisCol = 0; axisCol < TILE_WIDTH; axisCol++){
        axisSwapped[axisCol] = new uint8_t[TILE_WIDTH];
        
        //Fill this new row with tile data
        getTileLine(axisSwapped[axisCol], tilePatternAddress, tileIndex, axisCol);
    }
    
    //Copy into output
    for(int rowX = 0; rowX < TILE_WIDTH; rowX++){
        for(int rowY = 0; rowY < TILE_HEIGHT; rowY++){
            out[rowX][rowY] = axisSwapped[rowY][rowX];
        }
    }
    
    for(int axisCol = 0; axisCol < TILE_WIDTH; axisCol++){
        delete axisSwapped[axisCol];
    }
    delete axisSwapped;
}
        
//Swaps buffers and clears the active buffer
void GBLCD::swapBuffers(){
    m_bSwapBuffers = !m_bSwapBuffers;
    
    //Increment frame count
    m_Frames++;
}

void GBLCD::setMainRenderer(IRenderer* renderer){
    m_displayRenderer = renderer;
}

//void write();
void GBLCD::setLCDC(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_LCDC, val);
    
    //When LCD is disabled, it switches to mode 1
    if(!(val & LCDC_DISPLAY_ENABLE)){
        
        uint8_t currentSTAT = getSTAT();
        currentSTAT &= 0xFC;
        currentSTAT |= STAT_MODE1_VBLANK;
        
        if (CONSOLE_OUTPUT_ENABLED) std::cout << "LCD Off" << std::endl;
        
        //Clear screen
        RGBColor** buffer = getCompleteFrame();
        if(buffer != NULL){
            for(uint8_t pixelX = 0; pixelX < FRAMEBUFFER_WIDTH; pixelX++){
                for(uint8_t pixelY = 0; pixelY < FRAMEBUFFER_HEIGHT; pixelY++){
                    buffer[pixelX][pixelY] = COLOR_WHITE;
                }
            }
            
            //Push blank screen to renderer
            if(m_displayRenderer){
                m_displayRenderer->update(buffer, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
            }
        }
    } else {
        if (CONSOLE_OUTPUT_ENABLED) std::cout << "LCD On" << std::endl;
    }
}

uint8_t GBLCD::getLCDC(){
    return m_gbmemory->direct_read(ADDRESS_LCDC);
}

void GBLCD::setSTAT(uint8_t val){
    //Bottom three bits are read-only
    val &= 0xF8;
    uint8_t currentSTAT = getSTAT();
    //Remove all but bottom three bits from STAT
    currentSTAT &= 0x07;
    //Combine with val
    currentSTAT |= val;
    m_gbmemory->direct_write(ADDRESS_STAT, currentSTAT);
}

uint8_t GBLCD::getSTAT(){
    return m_gbmemory->direct_read(ADDRESS_STAT);
}
        
void GBLCD::setScrollY(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_SCY, val);
}

uint8_t GBLCD::getScrollY(){
    return m_gbmemory->direct_read(ADDRESS_SCY);
}

void GBLCD::setScrollX(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_SCX, val);
}

uint8_t GBLCD::getScrollX(){
    return m_gbmemory->direct_read(ADDRESS_SCX);
}
        
uint8_t GBLCD::getLY(){
    return m_gbmemory->direct_read(ADDRESS_LY);
}

uint8_t GBLCD::setLY(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_LY, 0);
}
        
void GBLCD::setLYC(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_LYC, val);
}

uint8_t GBLCD::getLYC(){
    return m_gbmemory->direct_read(ADDRESS_LYC);
}
        
void GBLCD::setWindowY(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_WY, val);
}

uint8_t GBLCD::getWindowY(){
    return m_gbmemory->direct_read(ADDRESS_WY);
}

//Sets the X position, offset by 7px
void GBLCD::setWindowX(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_WX, val);
}

//Gets the X position, offset by 7px
uint8_t GBLCD::getWindowX(){
    m_gbmemory->direct_read(ADDRESS_WX);
}
        
void GBLCD::setBGPalette(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_BGP, val);
}

uint8_t GBLCD::getBGPalette(){
    return m_gbmemory->direct_read(ADDRESS_BGP);
}

void GBLCD::setSpritePalette0(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_OBP0, val);
}

uint8_t GBLCD::getSpritePalette0(){
    return m_gbmemory->direct_read(ADDRESS_OBP0);
}

void GBLCD::setSpritePalette1(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_OBP1, val);
}

uint8_t GBLCD::getSpritePalette1(){
    return m_gbmemory->direct_read(ADDRESS_OBP1);
}
        
void GBLCD::startDMATransfer(uint8_t address){
    //Only allow OAM writes during modes 2 and 3
    //Commented out as currently enforcing this breaks some games. 
    /*
    uint8_t modeFlag = getSTAT() & STAT_MODE_FLAG;
    if((modeFlag < 2) && (getLCDC() & LCDC_DISPLAY_ENABLE)){
        return;
    }*/
    
    //Address is upper bits of xx00-xx9F
    uint16_t source = address * 0x100;
    
    //Copy from address to sprite attribute table
    for(uint8_t offset = 0; offset <= (OAM_END - OAM_START); offset++){
        m_gbmemory->direct_write(OAM_START + offset, m_gbmemory->read(source + offset));
    }
}
        
void GBLCD::writeVRam(uint16_t address, uint8_t val){
    uint8_t modeFlag = getLCDC() & STAT_MODE_FLAG;
    
    //VRam can only be written in modes 0-2
    //Commented out as currently enforcing this breaks some games. 
    /*
    if((modeFlag < 3) || !(getLCDC() & LCDC_DISPLAY_ENABLE)){
        m_gbmemory->direct_write(address, val);
    }
    */
    
    //Allow direct writes, this fixes tetris sprites
    m_gbmemory->direct_write(address, val);
}

uint8_t GBLCD::readVRam(uint16_t address){
    uint8_t toReturn = RETURN_VRAM_INACCESSABLE;
    uint8_t modeFlag = getSTAT() & STAT_MODE_FLAG;
    
    //VRam can only be read in modes 0-2
    //Commented out as currently enforcing this breaks some games. 
    /*
    if((modeFlag < 3) || !(getLCDC() & LCDC_DISPLAY_ENABLE)){
        toReturn = m_gbmemory->direct_read(address);
    }*/
    
    toReturn = m_gbmemory->direct_read(address);
    
    return toReturn;
}

void GBLCD::writeVRamSpriteAttribute(uint16_t address, uint8_t val){
    uint8_t modeFlag = getSTAT() & STAT_MODE_FLAG;
    
    //Sprite attribute table can only be written in modes 0 or 1, or if display is disabled
    ////Commented out as currently enforcing this breaks some games. 
    /*
    if((modeFlag < 2) || !(getLCDC() & LCDC_DISPLAY_ENABLE)){
        m_gbmemory->direct_write(address, val);
    }
    */
    
    //Allow direct writes until timing is fixed
    m_gbmemory->direct_write(address, val);
}

uint8_t GBLCD::readVRamSpriteAttribute(uint16_t address){
    uint8_t toReturn = RETURN_VRAM_INACCESSABLE;
    uint8_t modeFlag = getSTAT() & STAT_MODE_FLAG;
    
    /*
    //Sprite attribute table can only be read in modes 0 or 1, or if display is disabled
    //Commented out as currently enforcing this breaks some games. 
    if((modeFlag < 2) || !(getLCDC() & LCDC_DISPLAY_ENABLE)){
        toReturn = m_gbmemory->direct_read(address);
    }*/
    
    toReturn = m_gbmemory->direct_read(address);
    
    return toReturn;
}

//Gets the completed frame
RGBColor** GBLCD::getCompleteFrame(){
    return (m_bSwapBuffers ? m_Framebuffer0 : m_Framebuffer1);
}
        
//Gets the frame currently being rendered
RGBColor** GBLCD::getNextUnfinishedFrame(){
    return (m_bSwapBuffers ? m_Framebuffer1 : m_Framebuffer0);
}

//Returns the number of frames rendered;
long GBLCD::getFrames(){
    return m_Frames;
}
