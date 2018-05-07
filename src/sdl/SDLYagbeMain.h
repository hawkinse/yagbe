#pragma once
#if defined (_WIN32) || defined(__APPLE__)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include "../gb/gameboy.h"
#include "SDLBufferRenderer.h"
#include "SDLAudioPlayer.h"
#include "SDLInputChecker.h"

//TODO - is name appropriate? Technically more than just a window. 
//TODO - move core functionality to an interface? Would allow you to define a tick hander/subsystem handler genericly
class SDLYagbeMain{
private:
    //SDL Objects
    SDL_Window* m_SDLWindow;
    SDL_Renderer* m_SDLWindowRenderer;
    
    //SDL Interface Implementations
    SDLBufferRenderer* m_mainBufferRenderer;
    SDLAudioPlayer* m_audioPlayer;
    SDLInputChecker* m_inputChecker;

    //Task handling flags
    bool m_doTick, m_doWindow, m_doAudio, m_doInput;

    Gameboy* m_gameboy;
    
    thread* m_audioThread = nullptr;

    bool init_sdl(float windowSacle = 1.0f);
    void destroy_sdl();

    void mainThreadLoop();
    void audioThreadLoop();

    bool parseArgs(int argc, char** argv, float& windowScale);
public:
    SDLYagbeMain(Gameboy* gameboy, bool doTick, bool doWindow, bool doAudio, bool doInput, int argc, char** argv);
    ~SDLYagbeMain();
};

