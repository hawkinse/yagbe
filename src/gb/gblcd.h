#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../IRenderer.h"
#include "../constants.h"

//LCDC Bits
#define LCDC_DISPLAY_ENABLE 128
#define LCDC_WINDOW_TILE_MAP_DISPLAY_SELECT 64
#define LCDC_WINDOW_DISPLAY_ENABLE 32
#define LCDC_BG_WINDOW_TILE_SELECT 16
#define LCDC_BG_TILE_MAP_DISPLAY_SELECT 8
#define LCDC_SPRITE_SIZE 4
#define LCDC_SPRITE_DISPLAY_ENABLE 2
#define LCDC_BG_DISPLAY 1

#define BG_TILE_MAP_DISPLAY_0       0x9800
#define BG_TILE_MAP_DISPLAY_1       0x9C00
#define TILE_PATTERN_TABLE_0        0x8800
#define TILE_PATTERN_TABLE_0_TILE_0 0x9000
#define TILE_PATTERN_TABLE_1        0x8000

//STAT Bits
#define STAT_COINCIDENCE_INTERRUPT 64
#define STAT_MODE2_OAM_INTERRUPT 32
#define STAT_MODE1_VBLANK_INTERRUPT 16
#define STAT_MODE0_HBLANK_INTERRUPT 8
#define STAT_COINCIDENCE_FLAG 4
#define STAT_MODE_FLAG 3

//GBC DMA Transfer value mode
#define GBC_DMA_MODE 0x80

//Modes
#define STAT_MODE0_HBLANK   0
#define STAT_MODE1_VBLANK   1
#define STAT_MODE2_OAM      2
#define STAT_MODE3_TRANSFER 3

//GBC BG Map Tile Attributes
#define BGMAP_ATTRIBUTE_PALETTE 0x07
#define BGMAP_ATTRIBUTE_VRAM_BANK 0x08
#define BGMAP_ATTRIBUTE_HORIZONTAL_FLIP 0x20
#define BGMAP_ATTRIBUTE_VERTICAL_FLIP 0x40
#define BGMAP_ATTRIBUTE_OAM_PRIORITY 0x80

//Sprite Attribute Bits
#define SPRITE_ATTRIBUTE_BGPRIORITY 0x80
#define SPRITE_ATTRIBUTE_YFLIP 0x40
#define SPRITE_ATTRIBUTE_XFLIP 0x20
#define SPRITE_ATTRIBUTE_PALLETE 0x10
#define SPRITE_ATTRIBUTE_VRAM_BANK 0x08
#define SPRITE_ATTRIBUTE_GBC_PALETTE 0x07

//Sprite location offsets
#define SPRITE_X_OFFSET 8
#define SPRITE_Y_OFFSET 16

//BW Color pallet shades.
#define PALETTE_BW_WHITE 0
#define PALETTE_BW_LIGHTGRAY 1
#define PALETTE_BW_DARKGRAY 2
#define PALETTE_BW_BLACK 3

//B&W Colors in RGB format
#define COLOR_WHITE     RGBColor(0xFF, 0xFF, 0xFF)
#define COLOR_DARKGRAY  RGBColor(0xA9, 0xA9, 0xA9)
#define COLOR_LIGHTGRAY RGBColor(0xD3, 0xD3, 0xD3)
#define COLOR_BLACK     RGBColor(0x00, 0x00, 0x00)

//The maximum any r, g or b value can be for a GBC color 
#define GBC_RGB_MAX_VALUE 0x1F
#define RETURN_VRAM_INACCESSABLE 0xFF

#define FRAMEBUFFER_WIDTH 160
#define FRAMEBUFFER_HEIGHT 144
#define FRAMEBUFFER_SIZE FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT
#define BACKGROUND_BUFFER_WIDTH 256
#define BACKGROUND_BUFFER_HEIGHT 256
#define BACKGROUND_BUFFER_SIZE BACKGROUND_BUFFER_WIDTH * BACKGROUND_BUFFER_HEIGHT
#define BACKGROUND_MAP_WIDTH 32
#define BACKGROUND_MAP_HEIGHT 32
#define BACKGROUND_MAP_SIZE BACKGROUND_MAP_WIDTH * BACKGROUND_MAP_HEIGHT


//Timings
#define CYCLES_LCD_MODE0 204 //Listed as 201-207?
#define CYCLES_LCD_MODE2 80 //Listed as 77-83?
#define CYCLES_LCD_MODE3 172 //Listed as 169-175?
#define CYCLES_VBLANK_LENGTH 4560 //Length of vblank, not when it starts. 
#define CYCLES_VBLANK_LYINCREMENT_INTERVAL 456 //4560/10
#define VBLANK_LYINCREMENT_COUNT 10

//Tiles
#define TILE_WIDTH 8
#define TILE_HEIGHT 8
#define TILE_BYTES 16

class GBMem;

struct RGBColor{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    bool transparent;
    
    RGBColor(){
        r = 0;
        g = 0;
        b = 0;
        transparent = false;
    }
    
    RGBColor(uint8_t red, uint8_t green, uint8_t blue){
        r = red;
        g = green;
        b = blue;
        transparent = false;
    }
    
    bool equals(RGBColor other){
        return (r == other.r && g == other.g && b == other.b);
    }
};

class GBLCD{
    private:
        GBMem* m_gbmemory;
        
        //The rendering object for the actual display
        IRenderer* m_displayRenderer;
        
        //Double buffer frames so that we can always get a completly drawn frame
        //Store frames as an aray of directly usable colors
        RGBColor** m_Framebuffer0;
        RGBColor** m_Framebuffer1;
        
		//GBC Color palettes
		//Stored as uint8_t pointers intead of RGBColor pointers due to how GBC sets color values.
		uint8_t* m_gbcBGPalettes;
		uint8_t* m_gbcOAMPalettes;

        //Used as a temporary buffer to hold a current working tile.
        //Global so we don't waste speed constantly destroying and recreating the buffer
        uint8_t m_TempTile[TILE_WIDTH];
        
        //Control for which buffer is currently the completed frame
        bool m_bSwapBuffers;
        
        //Number of frames rendered
        long m_Frames;
        
		//Addresses and length for GBC HDMA transfer
		uint16_t m_hdmaSourceAddress;
		uint16_t m_hdmaDestinationAddress;
		uint16_t m_hdmaLength;
		bool m_bHBlankDMAInProgress;

        //Mode functions
        void performHBlank();
        void performVBlank();
        void performOAMSearch();
        void performOAMTransfer();
        
        //Increments LY
        void incrementLY();
        
		//Gets DMG color from the given palette
        RGBColor getColor(uint8_t palette, uint8_t colorIndex);
 
		//Gets GBC color from the given color index within the given palette index of the palette buffer.
		RGBColor getColorGBC(uint8_t* paletteBuffer, uint8_t paletteIndex, uint8_t colorIndex);
        
        //Updates the line indicated by LY and ScrollY in the background buffer
        void updateBackgroundLine(RGBColor** frameBuffer);
        
        //Updates the line indicated by LY in the window
        void updateWindowLine(RGBColor** frameBuffer);
        
        //Updates the sprites for the line indicated by LY
        void updateLineSprites(RGBColor** frameBuffer);
        
        //Renders the current line indicated by LY
        void renderLine();
        
        //Gets an 8 pixel line of tiles for the given tile index as an array of palette indicies
        //tileIndex is a value from 0 to 255 or -128 to 127. 
        void getTileLine(uint8_t* out, uint8_t vramBank, uint16_t tilePatternAddress, int tileIndex, int line);
        
        //Swaps buffers and clears the active buffer
        void swapBuffers();
        
		//Gets whether or not GBC mode HBlank DMA is active.
		bool isHBlankDMATransferActive();

		//Performs GBC mode VRam DMA
		void performDMATransferGBC();

    public:
        GBLCD(GBMem* mem);
        ~GBLCD();

        void tick(long long hz);
        
        void setMainRenderer(IRenderer* renderer);
        
        //void write();
        void setLCDC(uint8_t val);
        uint8_t getLCDC();
        void setSTAT(uint8_t val);
        uint8_t getSTAT();
        
        void setScrollY(uint8_t val);
        uint8_t getScrollY();
        void setScrollX(uint8_t val);
        uint8_t getScrollX();
        
        void setLY(uint8_t val);
        uint8_t getLY();
        
        void setLYC(uint8_t val);
        uint8_t getLYC();
        
        void setWindowY(uint8_t val);
        uint8_t getWindowY();
        void setWindowX(uint8_t val); //Sets the X position, offset by 7px
        uint8_t getWindowX(); //Gets the X position, offset by 7px
        
        void setBGPalette(uint8_t val);
        uint8_t getBGPalette();
        void setSpritePalette0(uint8_t val);
        uint8_t getSpritePalette0();
        void setSpritePalette1(uint8_t val);
        uint8_t getSpritePalette1();
        
        void startDMATransfer(uint8_t address);
        
		void startDMATransferGBC(uint8_t val);

        void writeVRam(uint16_t address, uint8_t val);
        uint8_t readVRam(uint16_t address);
        void writeVRamSpriteAttribute(uint16_t address, uint8_t val);
        uint8_t readVRamSpriteAttribute(uint16_t address);
        
		void writeBGPaletteGBC(uint8_t val);

		uint8_t readBGPaletteGBC();

		void writeOAMPaletteGBC(uint8_t val);

		uint8_t readOAMPaletteGBC();

        //Gets the completed frame
        RGBColor** getCompleteFrame();
        
        //Gets the frame currently being rendered
        RGBColor** getNextUnfinishedFrame();
        
        //Returns the number of frames since the last call to getFrames();
        long getFrames();
        
};
