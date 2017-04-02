#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include "SDLInputChecker.h"
#include "gb/gbpad.h"

SDLInputChecker::SDLInputChecker(GBPad* pad){
    m_pad = pad;
    m_bExitPerformed = false;
    m_speedMultiplier = 1.0;
}

void SDLInputChecker::checkForInput(){    
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0){
        switch(e.type){
            case SDL_QUIT:
                m_bExitPerformed = true;
                return;
                break;
            case SDL_KEYDOWN:
                switch(e.key.keysym.sym){
                    case SDLK_RETURN:
                        m_pad->setStart(true);
                        break;
                    case SDLK_RSHIFT:
                        m_pad->setSelect(true);
                        break;
                    case SDLK_x:
                        m_pad->setA(true);
                        break;
                    case SDLK_z:
                        m_pad->setB(true);
                        break;
                    case SDLK_UP:
                        m_pad->setUp(true);
                        break;
                    case SDLK_DOWN:
                        m_pad->setDown(true);
                        break;
                    case SDLK_LEFT:
                        m_pad->setLeft(true);
                        break;
                    case SDLK_RIGHT:
                        m_pad->setRight(true);
                        break;
                    case SDLK_F1:
                        if(m_speedMultiplier >= MIN_SPEED_MULTIPLIER){
                            m_speedMultiplier = (m_speedMultiplier - SPEED_MULTIPLIER_INCREMENT < MIN_SPEED_MULTIPLIER) ? MIN_SPEED_MULTIPLIER : m_speedMultiplier - SPEED_MULTIPLIER_INCREMENT;
                        }
                        break;
                    case SDLK_F2:
                        if(m_speedMultiplier <= MAX_SPEED_MULTIPLIER){
                            m_speedMultiplier = (m_speedMultiplier + SPEED_MULTIPLIER_INCREMENT > MAX_SPEED_MULTIPLIER) ? MAX_SPEED_MULTIPLIER : m_speedMultiplier + SPEED_MULTIPLIER_INCREMENT;
                        }
                        break;
                }
                break;
            case SDL_KEYUP:
                switch(e.key.keysym.sym){
                    case SDLK_RETURN:
                        m_pad->setStart(false);
                        break;
                    case SDLK_RSHIFT:
                        m_pad->setSelect(false);
                        break;
                    case SDLK_x:
                        m_pad->setA(false);
                        break;
                    case SDLK_z:
                        m_pad->setB(false);
                        break;
                    case SDLK_UP:
                        m_pad->setUp(false);
                        break;
                    case SDLK_DOWN:
                        m_pad->setDown(false);
                        break;
                    case SDLK_LEFT:
                        m_pad->setLeft(false);
                        break;
                    case SDLK_RIGHT:
                        m_pad->setRight(false);
                }
        }
    }
}

bool SDLInputChecker::exitPerformed(){
    return m_bExitPerformed;
}

float SDLInputChecker::getSpeedMultiplier(){
    return m_speedMultiplier;
}

