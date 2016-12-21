#include <stdlib.h>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "gb/opcodes.h"
#include "gb/gbz80cpu.h"
#include "gb/gbmem.h"
#include "gb/gbcart.h"
#include "gb/gbpad.h"
#include "gb/gblcd.h"
#include "gb/gbserial.h"
#include "IRenderer.h"
#include "SDLBufferRenderer.h"
#include "SDLInputChecker.h"

using namespace std;

//Input parameters
char* cartRomPath;
char* bootRomPath;
float windowScale;

//Gameboy objects
GBCart* m_gbcart;
GBMem* m_gbmem;
GBLCD* m_gblcd;
GBZ80* m_gbcpu;
GBPad* m_gbpad;
GBSerial* m_gbserial;

//SDL objects
SDL_Window* m_SDLWindow;
SDL_Renderer* m_SDLWindowRenderer;

//SDL Implementations of Gameboy interaction interfaces
SDLBufferRenderer* m_MainBufferRenderer;
SDLInputChecker* m_InputChecker;


void init_gb(char* filename, char* bootrom = NULL){
    m_gbcart = new GBCart(filename, bootrom);
    m_gbmem = new GBMem();
    m_gbmem->loadCart(m_gbcart);
    m_gblcd = new GBLCD(m_gbmem);
    m_gbcpu = new GBZ80(m_gbmem, m_gblcd);
    m_gbpad = new GBPad(m_gbmem);
    m_gbserial = new GBSerial(m_gbmem);
    
    m_gbcart->printCartInfo();
    m_gbmem->setPad(m_gbpad);
    m_gbmem->setLCD(m_gblcd);
    m_gbmem->setSerial(m_gbserial);
}

void destroy_gb(){
    delete m_gbpad;
    delete m_gbcpu;
    delete m_gblcd;
    delete m_gbmem;
    delete m_gbcart;
    delete m_gbserial;
}

bool init_sdl(float windowScale = 1.0f, bool showBackgroundMap = false){
    bool bSuccess = true;
    int RenderFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    
    if(SDL_Init(SDL_INIT_VIDEO) >= 0){
	//Generate window title
	char* windowTitle = new char[24];
        memcpy(windowTitle, "Yagbe: ", 7);
	memcpy(windowTitle + 7, m_gbcart->getCartridgeTitle().c_str(), 16);
        
        //Create main window
        m_SDLWindow = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, FRAMEBUFFER_WIDTH * windowScale, FRAMEBUFFER_HEIGHT * windowScale, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        
        if(m_SDLWindow){
            //Initialize with VSync so we don't waste processor cycles needlessly and burn battery life
            m_SDLWindowRenderer = SDL_CreateRenderer(m_SDLWindow, -1, RenderFlags);
            
            if(!m_SDLWindowRenderer){
                std::cout << "SDL could not create the main renderer. SDL Error: " << SDL_GetError() << std::endl;
            }
        } else {
            std::cout << "SDL could not create the main window. SDL Error: " << SDL_GetError() << std::endl;
            bSuccess = false;
        }
        
        
        
    } else {
        std::cout << "SDL could not initialize. SDL Error: " << SDL_GetError() << std::endl;
        bSuccess = false;
    }
    
    return bSuccess;
}

void destroy_sdl(){
    //Free any texture resources here
    SDL_DestroyRenderer(m_SDLWindowRenderer);
    m_SDLWindowRenderer = NULL;
    SDL_DestroyWindow(m_SDLWindow);
    m_SDLWindow = NULL;
    
    SDL_Quit();    
}

void mainLoop(){
    bool bRun = true;
    float speedMultiplier = 1.0f;
    float totalTime = 0;
    float deltaTime = 0;
    unsigned long long countedSDLFrames = 0;
    
    unsigned long long countedGBFrames = 0;
            
    while(bRun){        
        m_InputChecker->checkForInput();
        
        if(m_InputChecker->exitPerformed()){
            bRun = false;
            
            //Tell cart to save if game supports it.
            m_gbcart->save();            
            break;
        }
        
        if(speedMultiplier != m_InputChecker->getSpeedMultiplier()){
            speedMultiplier = m_InputChecker->getSpeedMultiplier();
            std::cout << "Speed multiplier is now: " << speedMultiplier << std::endl;
        }
        
        //Update gameboy framerate report
        long lastGBFrameCount = countedGBFrames;
        countedGBFrames = m_gblcd->getFrames();
        
        //tick CPU, only if delta time is under a second.
        m_gbcpu->tick((deltaTime > 1.0f) ? 0 : deltaTime * speedMultiplier);
        //m_gbcpu->tick(deltaTime);
        m_MainBufferRenderer->render();
        
        //Update frame counting
        countedSDLFrames++;
        deltaTime = (SDL_GetTicks() / 1000.0f) - totalTime;
        totalTime = (SDL_GetTicks() / 1000.0f);
        
    }
}

int main(int argc, char** argv){
  //Initialize gameboy with cartridge specified in argv
  if(argc > 1){
      cartRomPath = argv[1];
      if(ENABLE_BOOTROM){
        init_gb(cartRomPath, "bootrom.bin");
      } else {
        init_gb(cartRomPath);
      }
  } else {
      std::cout << "Must include path to cart file as argument" << std::endl;
      exit(1);
  }
  
  //Initialize SDL with no window scaling and background map enabled
  init_sdl(2.0f, false);
  
  //Connect SDL to the gameboy emulator display
  m_MainBufferRenderer = new SDLBufferRenderer(m_SDLWindowRenderer);
  
  //Connect SDL to LCD emulation
  m_gblcd->setMainRenderer(m_MainBufferRenderer);
  
  //Set up pad input
  m_InputChecker = new SDLInputChecker(m_gbpad);
  m_gbcpu->setInputChecker(m_InputChecker);
  
  //Emulation main loop
  mainLoop();
  
  //Clean up before exit  
  destroy_gb();
  destroy_sdl();
  
  return 0;
}
