#if defined(_WIN32) || defined(__APPLE__)
#include <SDL.h>
#include <SDL_audio.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#endif
#include "SDLAudioPlayer.h"
#include "gb/gbaudio.h" //Is this needed?

SDLAudioPlayer::SDLAudioPlayer(){
    SDL_AudioSpec requestedSpec;
    requestedSpec.freq = PLAYBACK_FREQUENCY;
    requestedSpec.format = AUDIO_S16SYS;
    requestedSpec.channels = 2;
    requestedSpec.samples = BUFFER_SAMPLES;
    requestedSpec.callback = sdlCallback;
    
    SDL_AudioSpec realSpec;
    
    SDL_OpenAudio(&requestedSpec, &realSpec);
    
    //Start playback
    SDL_PauseAudio(0);
}

SDLAudioPlayer::~SDLAudioPlayer(){
    SDL_CloseAudio();
}

void SDLAudioPlayer::update(Sound* buffer, int length){
    
}

void /*SDLAudioPlayer::*/sdlCallback(void* unused, uint8_t* buffer, int length){
    //Fill buffer with silence
    SDL_memset(buffer, 0, length);
}
