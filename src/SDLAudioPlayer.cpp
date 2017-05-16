#ifdef _WIN32
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
    requestedSpec.format = AUDIO_U16SYS;
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
        //Old code - scaled down large buffer in SDLAudioPlayer
        /*
        int totalGeneratedSamples = 0;
        while (bufferSize >= BUFFER_SAMPLES) {
            //Build up next samples
            //96 at time of comment sounds almost right, but buzzing and still some stutter!
            int posSkip = 87/2;//87; //Changed to 1 as this should be handled in gameboy side now!

            //If we don't have enough audio to fill the buffer, wait until next tick.
            //if (posSkip == 0) return;
            //if (bufferSize / posSkip < BUFFER_SAMPLES) return;

            uint8_t* scaledOutput = new uint8_t[BUFFER_SAMPLES];
            SDL_memset(scaledOutput, 0, BUFFER_SAMPLES);
            //Straight skip playback. Stuttery, not perfect!

            int scaledSamples = 0;
            for (; scaledSamples < BUFFER_SAMPLES; scaledSamples++) {
                scaledOutput[scaledSamples] = soundBuffer[scaledSamples * posSkip];
            }
            totalGeneratedSamples += scaledSamples;

            //soundBuffer.clear();
            //memset(soundBuffer, 0, scaledSamples);

            //Attempt to shift down unused samples
            if (scaledSamples < BUFFER_SAMPLES) {
                //memcpy(soundBuffer, soundBuffer + scaledSamples, bufferSize - scaledSamples);
                memmove(soundBuffer, soundBuffer + ((scaledSamples + 1) * posSkip), bufferSize - ((scaledSamples + 1) * posSkip));
                
                SDL_memset(soundBuffer + bufferSize, 0, FULL_BUFFER_SIZE - bufferSize);
                bufferSize -= (scaledSamples + 1) * posSkip;
            } else {
                bufferSize = 0;
                SDL_memset(soundBuffer, 0, FULL_BUFFER_SIZE);
            }

            //Add new samples to playback
            SDL_QueueAudio(1, scaledOutput, scaledSamples);
            delete scaledOutput;
        }*/

        //Wait for existing samples to finish playback
        /*
        while (SDL_GetQueuedAudioSize(1) > totalGeneratedSamples * 2) {
            SDL_Delay(1);
        }*/

        //Attempt to wait until buffer is full
        
        if (bufferSize >= BUFFER_SAMPLES) {
            while (SDL_GetQueuedAudioSize(1) > BUFFER_SAMPLES * sizeof(uint16_t)) {
                SDL_Delay(1);
            }

            SDL_QueueAudio(1, soundBuffer + (bufferSize - BUFFER_SAMPLES), BUFFER_SAMPLES * sizeof(uint16_t));
            //memcpy(soundBuffer, soundBuffer + bufferSize, bufferSize - BUFFER_SAMPLES);
            //bufferSize = bufferSize - BUFFER_SAMPLES;
            memset(soundBuffer, 0, bufferSize * sizeof(uint16_t));
            bufferSize = 0;
        }
        

        //Just play everything.\
        SDL_QueueAudio(1, soundBuffer, bufferSize);
        //memset(soundBuffer, 0, bufferSize);
        //bufferSize = 0;
        /*
        while (SDL_GetQueuedAudioSize(1) > bufferSize) {
            SDL_Delay(1);
        }*/
        /*
        while (SDL_GetQueuedAudioSize(1) > BUFFER_SAMPLES /2 ) {
            SDL_Delay(1);
        }*/
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
    /*
    if (bufferSize - length > 0) {
        memcpy(soundBuffer, soundBuffer + length, bufferSize - length);
        bufferSize -= length;
        memset(soundBuffer + bufferSize, 0, FULL_BUFFER_SIZE - bufferSize);
    } else {
        memset(soundBuffer, 0, bufferSize);
        bufferSize = 0;
    }*/
}

void /*SDLAudioPlayer::*/sdlCallback(void* unused, uint8_t* buffer, int length) {
    ((SDLAudioPlayer*)unused)->fillBuffer(buffer, length);

    //Fill buffer with silence
    //SDL_memset(buffer, 0, length);

    //CODE IS UNTESTED, LIKELY DOES NOT WORK, DO NOT ASSUME OK TO KEEP!
    //std::cout << "Audio sdl callback. Length: " << length << std::endl;
    /*
    SDLAudioPlayer* player = (SDLAudioPlayer*)unused;
    if (player->soundBuffer.size() > 0) {
        Sound currentSound = player->soundBuffer.front();
        for (int index = 0; index < length; index++) {
            buffer[index] = currentSound.note * 1000;
            currentSound.length--;
            if (currentSound.length == 0) {
                if (player->soundBuffer.size() > 0) {
                    currentSound = player->soundBuffer.front();
                } else {
                    break;
                }
            }
        }
    }
    */

    /*
    uint16_t* test = (uint16_t*)buffer;

    for (int i = 0; i < length; i += 2) {
        test[i] = 5125 * (i % 4 ? 1 : -1);
        test[i + 1] = test[i];
        //buffer[i + 1] = buffer[i];
    }*/
}
