#pragma once
#include <stdint.h>
#include <stdlib.h>
#include "../IAudioPlayer.h"
#include "../constants.h"

class GBMem;

struct Sound{
    uint8_t note;
    uint8_t length;
};

class GBAudio{
    private:
        GBMem* m_gbmemory;
        IAudioPlayer* m_player;
        
    public:
        GBAudio(GBMem* mem);
        ~GBAudio();

        void tick(long long hz);
        void setPlayer(IAudioPlayer* player);

        //Square Wave 1 Channel
        void setNR10(uint8_t val); //Sweep period, negate and shift
        uint8_t getNR10();
        void setNR11(uint8_t val); //Duty, length load
        uint8_t getNR11();
        void setNR12(uint8_t val); //Starting volume, envelope mode, period
        uint8_t getNR12();
        void setNR13(uint8_t val); //Frequency low byte
        uint8_t getNR13();
        void setNR14(uint8_t val); //trigger, has length, frequency high byte
        uint8_t getNR14();

        //Square Wave 2 Channel
        void setNR21(uint8_t val); //Duty, length load
        uint8_t getNR21();
        void setNR22(uint8_t val); //Start volume, envelope mode, period
        uint8_t getNR22();
        void setNR23(uint8_t val); //Frequency low byte
        uint8_t getNR23();
        void setNR24(uint8_t val); //Trigger, has length, frequency high byte
        uint8_t getNR24();

        //Wave Table Channel
        void setNR30(uint8_t val); //DAC power
        uint8_t getNR30();
        void setNR31(uint8_t val); //Length load
        uint8_t getNR31();
        void setNR32(uint8_t val); // Volume code
        uint8_t getNR32();
        void setNR33(uint8_t val); //Frequency low byte
        uint8_t getNR33();
        void setNR34(uint8_t val); //Trigger, has length, frequency high byte
        uint8_t getNR34();

        //Noise Channel
        void setNR41(uint8_t val); //Length load
        uint8_t getNR41();
        void setNR42(uint8_t val); //Starting volume, envelope mode, period
        uint8_t getNR42();
        void setNR43(uint8_t val); //Clock shift, LFSR width, divisor code
        uint8_t getNR43();
        void setNR44(uint8_t val); //Trigger, has length
        uint8_t getNR44();

        //Audio control
        void setNR50(uint8_t val); //Vin left/right enable, volume left/right.
        uint8_t getNR50();
        void setNR51(uint8_t val); //Left/right enables
        uint8_t getNR51();
        void setNR52(uint8_t val); //Power status, channel length status
        uint8_t getNR52();
};
