#pragma once
#include <stdlib.h>
#include <iostream>
#include <queue>
#include <cmath>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "IAudioPlayer.h"
#include "gb/gbaudio.h"

#define PLAYBACK_FREQUENCY 44100
#define BUFFER_SAMPLES 1024

using namespace std;

#ifndef Sound
struct Sound;
#endif 

//DO NOT like making this global, but apparently SDL needs this? Refuses to compile otherwise!
//TODO - Find a way to include this in the player class as a private function!
void sdlCallback(void* unused, uint8_t* buffer, int lenth);

class SDLAudioPlayer : public IAudioPlayer {
    private:
        std::queue<Sound> soundBuffer;
        //void sdlCallback(void* unused, uint8_t* buffer, int lenth);
        
    public:
        SDLAudioPlayer();
        ~SDLAudioPlayer();
        void update(Sound* buffer, int length);
};
