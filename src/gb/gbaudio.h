#pragma once
#include <stdint.h>
#include <stdlib.h>
#include "../IAudioPlayer.h"
#include "../constants.h"

//Applies to NR10
#define SQUARE1_SWEEP_PERIOD  0x70
#define SQUARE1_NEGATE        0x08
#define SQUARE1_SHIFT         0x07

//Applies to NR11, NR21
#define SQUARE_DUTY           0xC0
#define SQUARE_LENGTH         0x3F

//Applies to NR30
#define WAVE_DAC              0x80

//Applies to NR32
#define WAVE_VOLUME           0x60

//Applies to NR12, NR22, NR42
#define CHANNEL_VOLUME_START   0xF0
#define CHANNEL_ENVELOPE       0x08
#define CHANNEL_PERIOD         0x07

//Applies to NR14, NR24, NR34, NR44
#define CHANNEL_TRIGGER       0x80
#define CHANNEL_LENGTH_ENABLE 0x40
#define CHANNEL_FREQUENCY_MSB 0x07

//Applies to NR50
#define CONTROL_ENABLE_LEFT  0x80
#define CONTROL_VOLUME_LEFT  0x70
#define CONTROL_ENABLE_RIGHT 0x08
#define CONTROL_VOLUME_RIGHT 0x07

//Applies to NR51
#define CONTROL_NOISE_LEFT_ENABLED    0x80
#define CONTROL_WAVE_LEFT_ENABLED     0x40
#define CONTROL_SQUARE2_LEFT_ENABLED  0x20
#define CONTROL_SQUARE1_LEFT_ENABLED  0x10
#define CONTROL_NOISE_RIGHT_ENABLED   0x08
#define CONTROL_WAVE_RIGHT_ENABLED    0x04
#define CONTROL_SQUARE2_RIGHT_ENABLED 0x02
#define CONTROL_SQUARE1_RIGHT_ENABLED 0x01

//Applies to NR52
#define CONTROL_POWER 0x80
//Not sure if these are meant to indicate length remaining?
#define STATUS_NOISE_LENGTH_REMAINING   0x08
#define STATUS_WAVE_LENGTH_REMAINING    0x04
#define STATUS_SQUARE2_LENGTH_REMAINING 0x02
#define STATUS_SQUARE1_LENGTH_REMAINING 0x01

#define SQUARE_DUTY_WAVEFORMS {{0,0,0,0,0,0,0,1}, {1,0,0,0,0,0,0,1}, {1,0,0,0,0,1,1,1}, {0,1,1,1,1,1,1,0}}

//Volume codes for NR32
#define WAVE_VOLUME_0   0x00
#define WAVE_VOLUME_100 0x01
#define WAVE_VOLUME_50  0x10
#define WAVE_VOLUME_25  0x11

#define LENGTH_LOAD       64
#define LENGTH_LOAD_WAVE 256

//CPU clock required to increment frame sequencer for a channel
#define FRAME_SEQUENCER_CLOCK 512
//The following clocks are based on Frame Sequencer clock, not CPU clock.
#define LENGTH_COUNTER_CLOCK  256
#define VOLUME_ENVELOPE_CLOCK  64
#define FREQUENCY_SWEEP_CLOCK 128

class GBMem;

struct Sound{
    uint8_t note;
    uint8_t length;
};

class GBAudio{
    private:
        uint8_t SQUARE_DUTY_WAVEFORM_TABLE[4][8] = SQUARE_DUTY_WAVEFORMS;
        
        GBMem* m_gbmemory;
        IAudioPlayer* m_player;
    
        bool m_square1Enabled = true;
        bool m_square2Enabled = true;
        bool m_waveEnabled = true;
        bool m_noiseEnabled = true;
    
        bool m_square1Triggered;
        bool m_square2Triggered;
        bool m_waveTriggered;
        bool m_noiseTriggered;
        
        //Are globals needed? What happens if a value changes after being triggered?
        uint8_t m_square1Duty;
        uint16_t m_square1Frequency;
        long long m_square1FrequencyTimer = 0;
        long long m_square1VolumeEnvelopeTimer = 0;
        long long m_square1LengthCounter = 0;
        uint8_t m_square1Volume = 0;
		uint16_t m_square1TriggerFrequency = 0;
		long long m_square1FrequencySweepTimer = 0;


        uint8_t m_square2Duty;
        uint16_t m_square2Frequency;
        long long m_square2FrequencyTimer = 0;
        long long m_square2VolumeEnvelopeTimer = 0;
        long long m_square2LengthCounter = 0;
        uint8_t m_square2Volume = 0;

        long long m_waveFrequencyTimer = 0;
        uint16_t m_waveFrequency = 0;
        long long m_waveLengthCounter = 0;
        uint8_t m_waveSampleByteIndex = 0;

        long long m_noiseFrequencyTimer = 0;
        long long m_noiseFrequency = 0;
        long long m_noiseLengthCounter = 0;
        uint8_t m_noiseVolume = 0;
        long long m_noiseVolumeEnvelopePeriod = 0;
        bool m_noiseLengthEnable = false;
        uint16_t m_lfsr = 0;

        uint16_t tickSquare1(long long hz);
        uint16_t tickSquare2(long long hz);
        uint16_t tickWave(long long hz);
        uint16_t tickNoise(long long hz);

        uint8_t getNoiseDivisor();
        
    public:
        GBAudio(GBMem* mem);
        ~GBAudio();

        void tick(long long hz);
        void setPlayer(IAudioPlayer* player);

        void setSquare1Enabled(bool enabled);
        void setSquare2Enabled(bool enabled);
        void setWaveEnabled(bool enabled);
        void setNoiseEnabled(bool enabled);
    
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
