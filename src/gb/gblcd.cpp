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
    
	m_gbcBGPalettes = new uint8_t[0x3F];
	m_gbcOAMPalettes = new uint8_t[0x3F];

	//GBC palettes are initialized to white on startup
	memset(m_gbcBGPalettes, 0xFF, 0x3F);
	memset(m_gbcOAMPalettes, 0xFF, 0x3F);

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
    
	//Ensure scroll position is set to upper left on startup.
	setScrollX(0);
	setScrollY(0);

    //Swap buffers to ensure screen is cleared at startup
    swapBuffers();
}

GBLCD::~GBLCD(){
    //TODO - properly delete each line in the arrays in addition to base pointer
    delete m_Framebuffer0;
    delete m_Framebuffer1;
	delete m_gbcOAMPalettes;
	delete m_gbcBGPalettes;
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
                currentModeTimeLength = CYCLES_VBLANK_LYINCREMENT_INTERVAL;
                break;
            case STAT_MODE2_OAM:
                currentModeTimeLength = CYCLES_LCD_MODE2;
                break;
            case STAT_MODE3_TRANSFER:
                currentModeTimeLength = CYCLES_LCD_MODE3;
                break;
        }
        
        uint8_t currentSTAT = getSTAT();
		static int LYIncrementCount = 0;

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

		//If there is an HBlank DMA in progress, run it.
		if (isHBlankDMATransferActive()) {
			performDMATransferGBC();
		}
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

//Gets GBC color from the given color index within the given palette index of the palette buffer.
RGBColor GBLCD::getColorGBC(uint8_t* paletteBuffer, uint8_t paletteIndex, uint8_t colorIndex) {
	//std::cout << "Unimplemented getColorGBC" << std::endl;
	RGBColor toReturn;

	paletteIndex &= 0x3F;

	//Set the start index for the current color. Four colors per palette, 2 bytes per color, so multiply by 8.
	uint8_t startIndex = (paletteIndex * 8) + (colorIndex * 2);
	uint16_t rawColor = paletteBuffer[startIndex] | (paletteBuffer[startIndex + 1] << 8);

	//Get color from bytes
	uint8_t blue = rawColor & 0x1F;
	uint8_t green = (rawColor >> 5) & 0x1F;
	uint8_t red = (rawColor >> 10) & 0x1F;
	
	//Create RGBColor, converting from 5-bit to 8-bit color in the process
	toReturn = RGBColor(red * (0xFF/0x1F), green * (0xFF/0x1F), blue * (0xFF/0x1F));

	//The 0th color index indicates either a transparent sprite color or a background color which doesn't block transparent sprites.
	toReturn.transparent = (colorIndex == 0);

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
        
		//TODO -- add support for flags other than vram bank, like flip or color palette.
		uint8_t gbcFlags = 0;
		if (m_gbmemory->getGBCMode()) {
			//Flags for background tiles are stored at the same address in bank 1 as where the tile map is in bank 0.
			gbcFlags = m_gbmemory->direct_vram_read(tileLocation - VRAM_START, 1);
		}

		//Set the line of the tile to fetch.
		uint8_t tileLineHeight = (bgPixelY % TILE_HEIGHT);

		//If the vertical flip flag is set, flip it.
		if (m_gbmemory->getGBCMode() && (gbcFlags & BGMAP_ATTRIBUTE_VERTICAL_FLIP)){
			tileLineHeight = TILE_HEIGHT - tileLineHeight - 1;
		}

        getTileLine(m_TempTile, (gbcFlags & BGMAP_ATTRIBUTE_VRAM_BANK) > 0, tilePatternAddress, tileIndex, tileLineHeight);
        
        for(int tileX = (bScrolledTileDrawn ? 0 : (getScrollX() % TILE_WIDTH)); tileX < TILE_WIDTH; tileX++){
            if(bgPixelX  >= FRAMEBUFFER_WIDTH) break;

			RGBColor pixelColor = COLOR_WHITE;

			//Set pixel color based on platform 
			if (m_gbmemory->getGBCMode()) {
				//If the horizontal flip flag is set, flip it.
				uint8_t orientedTileX = tileX;
				if (gbcFlags & BGMAP_ATTRIBUTE_HORIZONTAL_FLIP) {
					orientedTileX = TILE_WIDTH - tileX;
				}

				pixelColor = getColorGBC(m_gbcBGPalettes, gbcFlags & BGMAP_ATTRIBUTE_PALETTE, m_TempTile[orientedTileX]);
			}
			else {
				//DMG Color
				 pixelColor = getColor(getBGPalette(), m_TempTile[tileX]);
			}

			frameBuffer[bgPixelX][getLY()] = pixelColor;

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
        if(getWindowY() <= getLY()){
            uint16_t tileLocation = (getLCDC() & LCDC_WINDOW_TILE_MAP_DISPLAY_SELECT) ? BG_TILE_MAP_DISPLAY_1 : BG_TILE_MAP_DISPLAY_0;
            
            //Set tile location offset.
            //Even though the Window can only show the size of the framebuffer, the map is still the size of the background map since they share the same location in memory
            tileLocation += ((getLY() - getWindowY()) / TILE_WIDTH * BACKGROUND_MAP_WIDTH);
            tileLocation += ((getWindowX() - 7) / TILE_WIDTH);

            for(int bgPixelX = getWindowX() - 7; bgPixelX < FRAMEBUFFER_WIDTH; bgPixelX += TILE_WIDTH){
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
                
				//TODO - add support for vram bank switching!
                getTileLine(m_TempTile, 0, tilePatternAddress, tileIndex, (getLY() - getWindowY()) % 8);
                
                for(int tileX = 0; tileX < TILE_WIDTH && (tileX + bgPixelX) < FRAMEBUFFER_WIDTH; tileX++){
                    //Skip over any part of the tile off the left edge of the screen
                    if((tileX + bgPixelX) < 0){
                        continue;
                    }
                    
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
	uint8_t vramBank = 0;
	uint8_t gbcPaletteNumber = 0;
    
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
		if (m_gbmemory->getGBCMode()) {
			vramBank = (spriteFlags & SPRITE_ATTRIBUTE_VRAM_BANK) > 0;
			gbcPaletteNumber = spriteFlags & SPRITE_ATTRIBUTE_GBC_PALETTE;
		}

        int realXPos = spriteXPos - SPRITE_X_OFFSET;
        int realYPos = spriteYPos - SPRITE_Y_OFFSET;
        
		//TODO - once GBC palettes are added, add support for sprites.
        uint8_t palette = (spriteFlags & SPRITE_ATTRIBUTE_PALLETE) ? getSpritePalette1() : getSpritePalette0();
        
        //Check if sprite is actually on screen
        if((realXPos > -7 && realXPos < 168)){
            if((realYPos >= getLY() - (bDoubleHeight ? 15 : 7) && realYPos <= getLY())){
                int tileYLine = (getLY() - realYPos) % (TILE_HEIGHT * 2);
                int tileYSpriteLine = (getLY() - realYPos) % TILE_HEIGHT;
                
                if(bDoubleHeight && ((tileYLine > 7 && !bYFlip) || (tileYLine <= 7 & bYFlip))){
                    spriteTileNum++;
                } 
                
                if(bYFlip){
                    tileYSpriteLine = 7 - tileYSpriteLine;
                }
                
                getTileLine(m_TempTile, vramBank, TILE_PATTERN_TABLE_1, spriteTileNum, tileYSpriteLine);
                
                for(int tileX = 0; tileX < TILE_WIDTH; tileX++){
                    int renderPosX = realXPos + tileX;
                    if(renderPosX > 0 && renderPosX < FRAMEBUFFER_WIDTH){
						RGBColor pixel = COLOR_WHITE;
						if (m_gbmemory->getGBCMode()) {
							pixel = getColorGBC(m_gbcOAMPalettes, gbcPaletteNumber, m_TempTile[(bXFlip ? (7 - tileX) : tileX)]);
						} else {
							pixel = getColor(palette, m_TempTile[(bXFlip ? (7 - tileX) : tileX)]);
						}
                         
                        if(!pixel.transparent){
							//On Gameboy Color, when bit 0 of LCDC is cleared sprites always have priority independent of priority flags.
                            if((m_gbmemory->getGBCMode() && !(getLCDC() & LCDC_BG_DISPLAY)) || !(spriteFlags & SPRITE_ATTRIBUTE_BGPRIORITY) || frameBuffer[renderPosX][getLY()].transparent){
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
void GBLCD::getTileLine(uint8_t* out, uint8_t vramBank, uint16_t tilePatternAddress, int tileIndex, int line){ 
    //Calculate the address of the tile's starting byte + offset for the current line.
    int tileAddress = tilePatternAddress;  //TILE_PATTERN_TABLE_0;
    tileAddress += (tileIndex * TILE_BYTES);
    tileAddress += (line * 2);
	
	//Translate from system memory address to direct vram bank address
	uint16_t vramAddress = (tileAddress - VRAM_START) + (vramBank * 0x2000);

	//The first byte contains the least significant bits of the color palette index.
	//Second contains most significant.
	uint8_t leastSignificantColors = m_gbmemory->direct_vram_read(vramAddress, vramBank);
	uint8_t mostSignificantColors = m_gbmemory->direct_vram_read(vramAddress + 1, vramBank);

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
        
//Swaps buffers and clears the active buffer
void GBLCD::swapBuffers(){
    m_bSwapBuffers = !m_bSwapBuffers;
    
    //Increment frame count
    m_Frames++;
}

void GBLCD::setMainRenderer(IRenderer* renderer){
    m_displayRenderer = renderer;
}

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

void GBLCD::setLY(uint8_t val){
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
    return m_gbmemory->direct_read(ADDRESS_WX);
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
     
void GBLCD::startDMATransferGBC(uint8_t val) {
	//std::cout << "Unimplemented GBC DMA transfer!" << std::endl;

	//Correct src and dest addresses so that they ignore the first four bits.
	hdmaSourceAddress = (m_gbmemory->read(ADDRESS_HDMA1) << 8 | m_gbmemory->read(ADDRESS_HDMA2)) & 0xFFF0;
	hdmaDestinationAddress = (m_gbmemory->read(ADDRESS_HDMA3) << 8 | m_gbmemory->read(ADDRESS_HDMA4)) & 0xFFF0;

	//Correct destination address to ensure it is in VRam.
	hdmaDestinationAddress = (hdmaDestinationAddress & 0x1FFF) | 0x8000;

	//Length is stored in lower 7 bits, divided by 0x10 and subtracted by 1.
	hdmaLength = ((val & 0x7F) + 1) * 0x10;

	//If this is a general DMA transfer, start transfer.
	//HBlank transfers will be triggered in next HBlank.
	if (!(val & GBC_DMA_MODE)) {
		//std::cout << "GBC General DMA Transfer!" << std::endl;
		//performDMATransferGBC();

		//Performing copy here instead of in performDMATransferGBC seems to work for (some) text in Pokemon Crystal.
		//Possibly due to unimplemented VRam bank switching + tile support?
		for (uint8_t index = 0; index < hdmaLength; index++) {
			m_gbmemory->direct_write(hdmaDestinationAddress + index, m_gbmemory->read(hdmaSourceAddress + index));
			
		}
	}
	else {
		//std::cout << "Unimplemented GBC HBlank DMA Transfer!" << std::endl;
		//std::cout << "GBC HBlank DMA Transfer started" << std::endl;
	}
}

//Gets whether or not GBC mode HBlank DMA is active.
bool GBLCD::isHBlankDMATransferActive() {
	bool toReturn = false;

	//HBlank DMA only occures on GBC
	if (m_gbmemory->getGBCMode()) {
		//Check if the HBlank DMA flag is set.
		toReturn = (m_gbmemory->direct_read(ADDRESS_HDMA5) & GBC_DMA_MODE) > 0;
	}

	return toReturn;
}

//Performs GBC mode VRam DMA
void GBLCD::performDMATransferGBC(){
	//for now, ignore HBlank DMA
	//if (isHBlankDMATransferActive()) return;

	//In HDMA transfer, only 0x10 bytes can be tranfered at once.
	uint8_t length = (isHBlankDMATransferActive() ? 0x10 : hdmaLength);

	//Copy bytes
	for (uint8_t currentByte = 0; currentByte < length; currentByte++) {
		m_gbmemory->direct_write(hdmaDestinationAddress + currentByte, m_gbmemory->read(hdmaSourceAddress + currentByte));
	}

	//If this is an HBlank transfer, need to save current progress or set done flag.
	if (isHBlankDMATransferActive()) {
		//Adjust source address and length for next call if HBlank transfer.
		hdmaSourceAddress += length;
		hdmaLength -= length;

		//If there are no bytes left to copy, clear HBlank DMA flag.
		if (!hdmaLength) {
			//Only bit 7 must be 0, but rest of bits are undefined.
			m_gbmemory->direct_write(ADDRESS_HDMA5, 0);
		}
	} else {
		//Need to set HDMA5 register to 0xFF to indicate completion.
		m_gbmemory->direct_write(ADDRESS_HDMA5, 0xFF);
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

void GBLCD::writeBGPaletteGBC(uint8_t val) {
	//Info for writing background palette is in BCPS
	uint8_t bcps = m_gbmemory->direct_read(ADDRESS_BCPS);

	//Get index for this byte and write.
	uint8_t byteIndex = bcps & 0x3F;
	m_gbcBGPalettes[byteIndex] = val;

	//Check if the index should auto increment
	if (bcps & 0x80) {
		bcps++;
		m_gbmemory->direct_write(ADDRESS_BCPS, bcps);
	}
}

uint8_t GBLCD::readBGPaletteGBC() {
	//Get index to read from BCPS
	uint8_t byteIndex = m_gbmemory->direct_read(ADDRESS_BCPS) & 0x3F;

	//Return selected byte.
	return m_gbcBGPalettes[byteIndex];
}

void GBLCD::writeOAMPaletteGBC(uint8_t val) {
	//Info for writing OAM palette is in OCPS
	uint8_t ocps = m_gbmemory->direct_read(ADDRESS_OCPS);

	//Get index for this byte and write
	uint8_t byteIndex = ocps & 0x3F;
	m_gbcOAMPalettes[byteIndex] = val;

	//Check if the index should auto increment
	if (ocps & 0x80) {
		ocps++;
		m_gbmemory->direct_write(ADDRESS_OCPS, ocps);
	}
}

uint8_t GBLCD::readOAMPaletteGBC() {
	//Get index to read from OCPS
	uint8_t byteIndex = m_gbmemory->direct_read(ADDRESS_OCPS) & 0x3F;

	//Return selected byte.
	return m_gbcOAMPalettes[byteIndex];
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
