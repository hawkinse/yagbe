#pragma once
#include <stdlib.h>
#include <iostream>
#if defined(_WIN32) || defined(__APPLE__)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include "../IInputChecker.h"
#include "../gb/gbpad.h"

#define MAX_SPEED_MULTIPLIER 4.0
#define MIN_SPEED_MULTIPLIER 0.25
#define SPEED_MULTIPLIER_INCREMENT 0.01

using namespace std;

class SDLInputChecker : public IInputChecker {
    private:
      GBPad* m_pad;
      bool m_bExitPerformed;
      bool m_bAudioSquare1Enabled = true;
      bool m_bAudioSquare2Enabled = true;
      bool m_bAudioWaveEnabled = true;
      bool m_bAudioNoiseEnabled = true;
      float m_speedMultiplier;
    public:
      SDLInputChecker(GBPad* pad);
      void checkForInput();
      float getSpeedMultiplier();
      bool exitPerformed();
      bool getAudioSquare1Enabled();
      bool getAudioSquare2Enabled();
      bool getAudioWaveEnabled();
      bool getAudioNoiseEnabled();
};
