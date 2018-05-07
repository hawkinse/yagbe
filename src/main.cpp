#include <stdlib.h>
#include <iostream>
#include <thread>
//SDL headers are not in an SDL2 directory on Windows and Mac
#if defined(_WIN32) || defined(__APPLE__)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include "gb/gameboy.h"
#include "sdl/SDLYagbeMain.h"

using namespace std;

//Whether or not we have an SDL window
bool bUseSDLWindow;

//Input parameters
char* cartRomPath;
char* bootRomPath;

Gameboy* m_gameboy;

bool parseArgs(int argc, char** argv, char* &bootRomPath, char* &cartRomPath, Platform &systemType) {
	bool bSuccess = true;
	int argIndex = 1;
    
	while (argIndex < argc) {
		if (strcmp(argv[argIndex], "-b") == 0) {
			bootRomPath = argv[argIndex + 1];
			argIndex++;
		} else if (strcmp(argv[argIndex], "-r") == 0) {
			cartRomPath = argv[argIndex + 1];
			argIndex++;
		} else if (strcmp(argv[argIndex], "-dmg") == 0) {
			std::cout << "Forcing system to DMG" << std::endl;
			systemType = Platform::PLATFORM_DMG;
		} else if (strcmp(argv[argIndex], "-sgb") == 0) {
			std::cout << "Forcing system to SGB" << std::endl;
			systemType = Platform::PLATFORM_SGB;
		}
		else if (strcmp(argv[argIndex], "-gbc") == 0) {
			std::cout << "Forcing system to GBC" << std::endl;
			systemType = Platform::PLATFORM_GBC;
		}

		argIndex++;
	}

	return bSuccess;
}

int main(int argc, char** argv){
  char* bootRomPath = NULL;
  char* cartRomPath = NULL;
    
  Platform systemType = Platform::PLATFORM_AUTO;

  bool bArgsValid = parseArgs(argc, argv, bootRomPath, cartRomPath, systemType);
    
  //Initialize GB components.
  if (bArgsValid) {
	  //TODO - allow gameboy to run without a loaded cartridge
      m_gameboy = new Gameboy(cartRomPath, (ENABLE_BOOTROM ? bootRomPath : NULL), systemType);
  }
  else {
	  std::cout << "Arguments invalid!" << std::endl;
	  exit(1);
  }
  
  //TODO - Figure out why window border is broken with input disabled, crash on audio disabled
  //Start handling of SDL stuff
  SDLYagbeMain* mainWindow = new SDLYagbeMain(m_gameboy, true, true, true, true, argc, argv);
    
  //Cleanup
  delete mainWindow;
    
  delete m_gameboy;
    
  return 0;
}
