#include <stdlib.h>
#include <iostream>
//SDL headers are not in an SDL2 directory on Windows
#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include "gb/opcodes.h"
#include "gb/gbz80cpu.h"
#include "gb/gbmem.h"
#include "gb/gbcart.h"
#include "gb/gbpad.h"
#include "gb/gblcd.h"
#include "gb/gbaudio.h"
#include "gb/gbserial.h"
#include "SDLBufferRenderer.h"
#include "SDLAudioPlayer.h"
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
GBAudio* m_gbaudio;
GBZ80* m_gbcpu;
GBPad* m_gbpad;
GBSerial* m_gbserial;

//SDL objects
SDL_Window* m_SDLWindow;
SDL_Renderer* m_SDLWindowRenderer;

//SDL Implementations of Gameboy interaction interfaces
SDLBufferRenderer* m_MainBufferRenderer;
SDLAudioPlayer* m_AudioPlayer;
SDLInputChecker* m_InputChecker;


void init_gb(char* filename, char* bootrom = NULL){
    m_gbcart = new GBCart(filename, bootrom);
    m_gbmem = new GBMem();
    m_gbmem->loadCart(m_gbcart);
    m_gblcd = new GBLCD(m_gbmem);
    m_gbmem->setLCD(m_gblcd);
    m_gbaudio = new GBAudio(m_gbmem);
    m_gbmem->setAudio(m_gbaudio);
    m_gbcpu = new GBZ80(m_gbmem, m_gblcd, m_gbaudio);
    m_gbpad = new GBPad(m_gbmem);
    m_gbmem->setPad(m_gbpad);
    m_gbserial = new GBSerial(m_gbmem);
    m_gbmem->setSerial(m_gbserial);
    
    m_gbcart->printCartInfo();
}

void destroy_gb(){
    delete m_gbpad;
    delete m_gbcpu;
    delete m_gbaudio;
    delete m_gblcd;
    delete m_gbmem;
    delete m_gbcart;
    delete m_gbserial;
}

bool init_sdl(float windowScale = 1.0f, bool showBackgroundMap = false){
    bool bSuccess = true;
    
    //Initialize with VSync so we don't waste processor cycles needlessly and burn battery life
    int RenderFlags = SDL_RENDERER_ACCELERATED/* | SDL_RENDERER_PRESENTVSYNC*/;
    
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) >= 0){
	//Generate window title
	char* windowTitle = new char[24];
        memcpy(windowTitle, "Yagbe: ", 7);
	memcpy(windowTitle + 7, m_gbcart->getCartridgeTitle().c_str(), 16);
        
        //Create main window
        m_SDLWindow = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, FRAMEBUFFER_WIDTH * windowScale, FRAMEBUFFER_HEIGHT * windowScale, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        
        if(m_SDLWindow){
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
        m_AudioPlayer->play(deltaTime);
        
        //Update frame counting
        countedSDLFrames++;
        deltaTime = (SDL_GetTicks() / 1000.0f) - totalTime;
        totalTime = (SDL_GetTicks() / 1000.0f);
        
    }
}

bool parseArgs(int argc, char** argv, char* &bootRomPath, char* &cartRomPath, float &windowScale) {
	bool bSuccess = true;
	int argIndex = 1;
	while (argIndex < argc) {
		if (strcmp(argv[argIndex], "-b") == 0) {
			bootRomPath = argv[argIndex + 1];
		} else if (strcmp(argv[argIndex], "-s") == 0) {
			windowScale = atof(argv[argIndex + 1]);
		}
		else if (strcmp(argv[argIndex], "-r") == 0) {
			cartRomPath = argv[argIndex + 1];
		}
		else {
			std::cout << "Unrecognized argument " << argv[argIndex] << std::endl;
			bSuccess = false;
			break;
		}

		argIndex += 2;
	}

	return bSuccess;
}

int main(int argc, char** argv){
  char* bootRomPath = NULL;
  char* cartRomPath = NULL;
  float windowScale = 1.0f;

  bool bArgsValid = parseArgs(argc, argv, bootRomPath, cartRomPath, windowScale);

  //If there is a cart path on the command line, initialize GB components.
  if (bArgsValid && (cartRomPath != NULL)) {
	  init_gb(cartRomPath, (ENABLE_BOOTROM ? bootRomPath : NULL));
  }
  else {
	  std::cout << "No cart specified or arguments invalid!" << std::endl;
	  exit(1);
  }
  
  //Initialize SDL
  init_sdl(windowScale, false);
  
  //Connect SDL to the gameboy emulator display
  m_MainBufferRenderer = new SDLBufferRenderer(m_SDLWindowRenderer);
  
  //Connect SDL to LCD emulation
  m_gblcd->setMainRenderer(m_MainBufferRenderer);
  
  //Connect SDL to Audio emulation
  m_AudioPlayer = new SDLAudioPlayer();
  m_gbaudio->setPlayer(m_AudioPlayer);
  
  //Set up pad input
  m_InputChecker = new SDLInputChecker(m_gbpad);
  m_gbcpu->setInputChecker(m_InputChecker);
  
  //Emulation main loop
  mainLoop();
  
  //Clean up before exit  
  destroy_gb();
  destroy_sdl();
  
  delete m_MainBufferRenderer;
  delete m_AudioPlayer;
  delete m_InputChecker;
  
  return 0;
}
