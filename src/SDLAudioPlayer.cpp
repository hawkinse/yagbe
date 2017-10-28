#if defined(_WIN32) || defined(__APPLE__)
#include <SDL.h>
#include <SDL_audio.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#endif
#include <math.h>
#include <set>
#include "SDLAudioPlayer.h"
#include "gb/gbaudio.h"

SDLAudioPlayer::SDLAudioPlayer(bool bUseAudioQueue) {
    SDL_AudioSpec requestedSpec;
    requestedSpec.freq = PLAYBACK_FREQUENCY;
    requestedSpec.format = OUTPUT_AUDIO_FORMAT;
    requestedSpec.channels = 2;
    requestedSpec.samples = BUFFER_SAMPLES;
    requestedSpec.callback = (bUseAudioQueue ? NULL : sdlCallback);
    requestedSpec.userdata = this;

    bUsingAudioQueue = bUseAudioQueue;

    SDL_AudioSpec realSpec;

    SDL_OpenAudio(&requestedSpec, &realSpec);
    std::cout << std::dec;
    std::cout << "Real spec: " << std::endl;
    std::cout << "Freq: " << realSpec.freq << std::endl;
    std::cout << "format: " << realSpec.format << std::endl;
    std::cout << "Fromat is as requested: " << (realSpec.format == requestedSpec.format) << std::endl;
    std::cout << "channels: " << realSpec.channels << std::endl;
    std::cout << "Expected samples: " << realSpec.samples << std::endl;
    std::cout << std::hex;

    soundBuffer = new uint16_t[FULL_BUFFER_SIZE];
    SDL_memset(soundBuffer, 0, FULL_BUFFER_SIZE * sizeof(uint16_t));
    bufferSize = 0;
    //Start playback
    SDL_PauseAudio(0);
}

SDLAudioPlayer::~SDLAudioPlayer() {
    SDL_CloseAudio();
}

void SDLAudioPlayer::addNote(uint16_t note) {
    //Attempt to rewrite this in terms of audio queues instead of audio callbacks. Might help with rendering?
    //soundBuffer.push_back(note);
    //soundBuffer[bufferSize++];
    if (bufferSize + 1 < FULL_BUFFER_SIZE) {
        soundBuffer[bufferSize++] = note;
    } else {
        std::cout << "Audio buffer overflow!" << std::endl;
        memset(soundBuffer, 0, FULL_BUFFER_SIZE * sizeof(uint16_t));
        bufferSize = 0;
    }
}

void SDLAudioPlayer::addNote(uint16_t note, long long hz) {
    addNote(note);
    return;
    /*
    if (hz + bufferSize < FULL_BUFFER_SIZE) {
        memset(soundBuffer + bufferSize, note, hz);
        bufferSize += hz;
        //SDL_QueueAudio(1, &note, 1);
    } else {
        std::cout << "Audio Buffer overflow!" << std::endl;
        memset(soundBuffer, 0, FULL_BUFFER_SIZE);
        bufferSize = 0;
    }*/
}

void SDLAudioPlayer::addNotes(uint16_t* notes, long long length) {
    if (length + bufferSize < FULL_BUFFER_SIZE) {
        memcpy(soundBuffer + bufferSize, notes, length * sizeof(uint8_t));
        bufferSize += length;
    } else {
        std::cout << "Audio buffer overflow!" << std::endl;
        memset(soundBuffer, 0, FULL_BUFFER_SIZE);
        memcpy(soundBuffer + bufferSize, notes, length);
        bufferSize = 0;
    }
}

void SDLAudioPlayer::mixNotes(uint16_t* src, uint16_t* dest, long long length) {
    //*2 is workaround for max volume seemingly expecting signed values
    SDL_MixAudioFormat((uint8_t*)dest, (uint8_t*)src, AUDIO_U16SYS, length*2, SDL_MIX_MAXVOLUME);
}

void SDLAudioPlayer::play(long long hz) {
    //Renders audio using audio queue instead of callback.
    
    if (bUsingAudioQueue) {
        //Attempt to wait until buffer is full
        
        if (bufferSize >= BUFFER_SAMPLES) {
            while (SDL_GetQueuedAudioSize(1) > BUFFER_SAMPLES * sizeof(uint16_t)) {
                SDL_Delay(1);
            }

            SDL_QueueAudio(1, soundBuffer + (bufferSize - BUFFER_SAMPLES), BUFFER_SAMPLES * sizeof(uint16_t));
            memset(soundBuffer, 0, bufferSize * sizeof(uint16_t));
            bufferSize = 0;
        }
    }
}

//Used to give Gameboy emulation the playback frequency so it knows how often it needs to gather samples.
uint32_t SDLAudioPlayer::getSampleRate() {
    return PLAYBACK_FREQUENCY;
}

//Called by SDL audio callback to fill buffer for output.
void SDLAudioPlayer::fillBuffer(uint8_t* buffer, int length) {
    memset(buffer, 0, length);
    memcpy(buffer, soundBuffer, length);

    memset(soundBuffer, 0, bufferSize);
    bufferSize = 0;
}

void /*SDLAudioPlayer::*/sdlCallback(void* unused, uint8_t* buffer, int length) {
    ((SDLAudioPlayer*)unused)->fillBuffer(buffer, length);
}
