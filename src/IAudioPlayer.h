#pragma once
#include "constants.h"

#ifndef Sound
struct Sound;
#endif

class IAudioPlayer{
    public:
        //virtual void update(Sound* buffer, int length) = 0;
        virtual void addNote(uint16_t note) = 0;
        virtual void addNote(uint16_t note, long long hz) = 0;
        virtual void addNotes(uint16_t* notes, long long length) = 0;
        virtual void mixNotes(uint16_t* src, uint16_t* dest, long long length) = 0; //Ugly shim to make GB code not directly call sdl stuff
        //Used to get sample rate for generating data
        virtual uint32_t getSampleRate() = 0;
};
