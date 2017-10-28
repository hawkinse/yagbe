#pragma once
#include <stdlib.h>
#include <iostream>
#include <queue>
#include <cmath>
#if defined(_WIN32) || defined(__APPLE__)
#include <SDL.h>
#include <SDL_audio.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#endif
#include "IAudioPlayer.h"
#include "gb/gbaudio.h"

#define PLAYBACK_FREQUENCY 41100
#define BUFFER_SAMPLES 2048
#define FULL_BUFFER_SIZE 4194304

//SDL Audio format to use.
//Mac OS seems to not support Unsigned sample playback, so use signed on that platform.
#if defined(__APPLE__)
#define OUTPUT_AUDIO_FORMAT AUDIO_S16SYS
#else
#define OUTPUT_AUDIO_FORMAT AUDIO_U16SYS
#endif

using namespace std; 

//DO NOT like making this global, but apparently SDL needs this? Refuses to compile otherwise!
//TODO - Find a way to include this in the player class as a private function!
void sdlCallback(void* unused, uint8_t* buffer, int length);

class SDLAudioPlayer : public IAudioPlayer {
    private:
        bool bUsingAudioQueue;
        long long timeSinceLastUpdate;
        //std::vector<Sound> soundBuffer;
        //std::vector<uint8_t> soundBuffer;
        uint16_t* soundBuffer;
        uint32_t bufferSize = 0;
        //void sdlCallback(void* unused, uint8_t* buffer, int lenth);
        
    public:
        
        //UseAudioQueue specifies whether to use a callback or explicit audio queueing.
        SDLAudioPlayer(bool bUseAudioQueue = true);
        ~SDLAudioPlayer();
        
        //Adds new audio data from gameboy
        //void update(Sound* buffer, int length);
        void addNote(uint16_t note);
        void addNote(uint16_t note, long long hz);
        void addNotes(uint16_t* notes, long long length);
        void mixNotes(uint16_t* src, uint16_t* dest, long long length); //Ugly shim to make GB code not directly call sdl stuff

        //Called by SDL audio callback to fill buffer for output.
        void fillBuffer(uint8_t* buffer, int length);

        //Fills SDL audio buffer when not using callbacks.
        void play(long long hz);

        uint32_t getSampleRate();
};
