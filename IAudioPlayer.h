#pragma once
#include "constants.h"

#ifndef Sound
struct Sound;
#endif

class IAudioPlayer{
    public:
        virtual void update(Sound* buffer, int length) = 0;
};
