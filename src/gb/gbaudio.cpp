#include "gbaudio.h"
#include <stdlib.h>
#include <iostream>
#include "gbmem.h"
#include "gbz80cpu.h" //Included for clock speed access. TODO - get actual clock speed

GBAudio::GBAudio(GBMem* mem){
    m_gbmemory = mem;
}

GBAudio::~GBAudio(){
    
}

void GBAudio::tick(long long hz){
    static long long rollover = 0;
    //std::cout << "Unimplemented audio tick " << +hz << " cycles" << std::endl;
    uint8_t* tempBuffer = new uint8_t[hz];
    uint8_t* mixedBuffer = new uint8_t[hz];
    uint16_t currentNote = 0;
    uint16_t mixedNote = 0;

    //memset(tempBuffer, 0, hz);
    //smemset(mixedBuffer, 0, hz);

    currentNote = tickSquare1(tempBuffer, hz);
    //m_player->mixNotes(tempBuffer, mixedBuffer, hz);
    m_player->mixNotes(&currentNote, &mixedNote, 1);

    currentNote = tickSquare2(tempBuffer, hz);
    //m_player->mixNotes(tempBuffer, mixedBuffer, hz);
    m_player->mixNotes(&currentNote, &mixedNote, 1);

    currentNote = tickWave(tempBuffer, hz);
    //m_player->mixNotes(tempBuffer, mixedBuffer, hz);
    m_player->mixNotes(&currentNote, &mixedNote, 1);
    
    currentNote = tickNoise(tempBuffer, hz);
    //m_player->mixNotes(tempBuffer, mixedBuffer, hz);
    m_player->mixNotes(&currentNote, &mixedNote, 1);

    //m_player->addNotes(mixedBuffer, hz);
    uint32_t skip = (CLOCK_GB * MHZ_TO_HZ) / m_player->getSampleRate() / 2;
    if ((rollover + hz) >= skip) {
        m_player->addNote(mixedNote*500, 1);
        rollover = rollover + hz - skip;
    } else {
        rollover += hz;
    }

    delete tempBuffer;
    delete mixedBuffer;
}

uint16_t GBAudio::tickSquare1(uint8_t* buffer, long long hz){
    //Copied verbatim from tickSquare2, aside from changing to appropriate registers. If updating square 2, make sure to update this!
    //Two counters: Length and frequency.
    static long long frequencyRollover = 0;
    static long long lengthRollover = 0;
    static uint8_t nextDutyIndex = 0;
    //return;
    //m_square2LengthCounter -= hz;

    //When length is decremented to 0, channel is disabled. 
    //On length counter hitting 0, set volume to 0 but do not cease generation of data!
    //Length counter decrements each 256 Hz.
    //Only decremented if length counter enabled.
    //Check if length counter is enabled
    bool bChannelEnabled = true;
    if (getNR14() & CHANNEL_LENGTH_ENABLE > 0) {
        if (hz + lengthRollover >= 256) {
            m_square1LengthCounter--;
            if (m_square1LengthCounter == 0) {
                bChannelEnabled = false;
            }
            lengthRollover = hz + lengthRollover - 256;
        } else {
            lengthRollover += hz;
        }
    }

    if (bChannelEnabled && !(getNR52() >> 7)) {
        bChannelEnabled = false;
    }

    //Stutters and low sounds may be due to inner hz >= m_square2FrequencyTimer check being wrong, ignoring good samples
    //Only getting a small portion of what we want?
    uint8_t note = 0;
    if (bChannelEnabled) {
        //TODO - see if making this a loop and continually adding notes, 
        //and then making notes for the remainder of the hz fixes stutter.

        if (hz >= m_square1FrequencyTimer) {
            //hz -= m_square2FrequencyTimer;
            // m_square2FrequencyTimer = (2048 - m_square2Frequency) * 4;
            //nextDutyIndex = (nextDutyIndex + 1) % 8;
            //Might be overcomplicating things with time! Try with just notes, and stretch according to sample buffer size!
            //This might work since we KNOW that in the SDL side, a second of data should fill a 44Khz buffer!

            //According to reddit post here, this is actually a sample a hz!
            //https://www.reddit.com/r/EmuDev/comments/5gkwi5/gb_apu_sound_emulation/
            //Recommends to cut down to only gathering a sample every 95 hz, from CPU_CLOCK/Playback Frequency
            while (hz >= m_square1FrequencyTimer && m_square1FrequencyTimer >= 0) {
                nextDutyIndex = (nextDutyIndex + 1) % 8;
                note = SQUARE_DUTY_WAVEFORM_TABLE[(getNR11() >> 6) & 0x03][nextDutyIndex];
                //Not sure why volume is so quiet...
                note *= m_square1Volume;
                //m_player->addNote(note, m_square2FrequencyTimer);
                memset(buffer, note, m_square1FrequencyTimer);
                buffer += m_square1FrequencyTimer;

                hz -= m_square1FrequencyTimer;
                m_square1FrequencyTimer = (2048 - m_square1Frequency) * 4;
                //std::cout << "Duty Waveform: " << ((getNR21() >> 6) & 0x03) << std::endl;
                //std::cout << "Duty height : " << (uint16_t)(SQUARE_DUTY_WAVEFORM_TABLE[(getNR21() >> 6) & 0x03][nextDutyIndex]) << std::endl;
            }

        } else {
            m_square1FrequencyTimer -= hz;
        }

        note = SQUARE_DUTY_WAVEFORM_TABLE[(getNR11() >> 6) & 0x03][nextDutyIndex];
        //Not sure why volume is so quiet...
        note *= m_square1Volume;

    } else {
        //Still need to output a 0 when channel disabled!
        //m_player->addNote(0, 1);
    }

    if (true || hz < 100000) {
        //m_player->addNote(note, hz);
        memset(buffer, note, hz);
    }

    return note;
}

uint16_t GBAudio::tickSquare2(uint8_t* buffer, long long hz) {
    //CODE IS UNTESTED, LIKELY DOES NOT WORK, DO NOT ASSUME OK TO KEEP!
    //Two counters: Length and frequency.
    static long long frequencyRollover = 0;
    static long long lengthRollover = 0;
    static uint8_t nextDutyIndex = 0;
    //return;
    //m_square2LengthCounter -= hz;

    //When length is decremented to 0, channel is disabled. 
    //On length counter hitting 0, set volume to 0 but do not cease generation of data!
    //Length counter decrements each 256 Hz.
    //Only decremented if length counter enabled.
    //Check if length counter is enabled
    bool bChannelEnabled = true;
    if (getNR24() & CHANNEL_LENGTH_ENABLE > 0) {
        if (hz + lengthRollover >= 256) {
            m_square2LengthCounter--;
            if (m_square2LengthCounter == 0) {
                bChannelEnabled = false;
            }
            lengthRollover = hz + lengthRollover - 256;
        } else {
            lengthRollover += hz;
        }
    }


    if (bChannelEnabled && !(getNR52() >> 7)) {
        bChannelEnabled = false;
    }

    //Stutters and low sounds may be due to inner hz >= m_square2FrequencyTimer check being wrong, ignoring good samples
    //Only getting a small portion of what we want?
    uint8_t note = 0;
    if (bChannelEnabled) {
        //TODO - see if making this a loop and continually adding notes, 
        //and then making notes for the remainder of the hz fixes stutter.

        if (hz >= m_square2FrequencyTimer) {
            //hz -= m_square2FrequencyTimer;
           // m_square2FrequencyTimer = (2048 - m_square2Frequency) * 4;
            //nextDutyIndex = (nextDutyIndex + 1) % 8;
            //Might be overcomplicating things with time! Try with just notes, and stretch according to sample buffer size!
            //This might work since we KNOW that in the SDL side, a second of data should fill a 44Khz buffer!

            //According to reddit post here, this is actually a sample a hz!
            //https://www.reddit.com/r/EmuDev/comments/5gkwi5/gb_apu_sound_emulation/
            //Recommends to cut down to only gathering a sample every 95 hz, from CPU_CLOCK/Playback Frequency
            while (hz >= m_square2FrequencyTimer && m_square2FrequencyTimer >= 0) {
                nextDutyIndex = (nextDutyIndex + 1) % 8;
                note = SQUARE_DUTY_WAVEFORM_TABLE[(getNR21() >> 6) & 0x03][nextDutyIndex];
                note *= m_square2Volume;
                //m_player->addNote(note, m_square2FrequencyTimer);
                memset(buffer, note, m_square2FrequencyTimer);
                buffer += m_square2FrequencyTimer;

                hz -= m_square2FrequencyTimer;
                m_square2FrequencyTimer = (2048 - m_square2Frequency) * 4;
                //std::cout << "Duty Waveform: " << ((getNR21() >> 6) & 0x03) << std::endl;
                //std::cout << "Duty height : " << (uint16_t)(SQUARE_DUTY_WAVEFORM_TABLE[(getNR21() >> 6) & 0x03][nextDutyIndex]) << std::endl;
            }

        } else {
            m_square2FrequencyTimer -= hz;
        }
        

        note = SQUARE_DUTY_WAVEFORM_TABLE[(getNR21() >> 6) & 0x03][nextDutyIndex];
        note *= m_square2Volume;

    }
    
    if (true || hz < 100000) {
        //m_player->addNote(note, hz);
        memset(buffer, note, hz);
    }

    return note;
}

uint16_t GBAudio::tickWave(uint8_t* buffer, long long hz){
    //Edited from tick square1, which was copied from tick square 2.
    //Two counters: Length and frequency.
    static long long frequencyRollover = 0;
    static long long lengthRollover = 0;
    static uint8_t sampleByteIndex = 0;
    static bool bFirstByteSample = true;

    //return;
    //m_square2LengthCounter -= hz;

    //When length is decremented to 0, channel is disabled. 
    //On length counter hitting 0, set volume to 0 but do not cease generation of data!
    //Length counter decrements each 256 Hz.
    //Only decremented if length counter enabled.
    //Check if length counter is enabled
    bool bChannelEnabled = true;
    //Implementation is broken!
    /*
    if ((getNR34() & CHANNEL_LENGTH_ENABLE) > 0) {
        if (hz + lengthRollover >= 256) {
            if (m_waveLengthCounter > 0) {
                m_waveLengthCounter--;
            }
            
            lengthRollover = hz + lengthRollover - 256;
        } else {
            lengthRollover += hz;
        }

        if (m_waveLengthCounter <= 0) {
            bChannelEnabled = false;
        }
    }*/

    if (bChannelEnabled && !(getNR52() >> 7)) {
        bChannelEnabled = false;
    }

    //Stutters and low sounds may be due to inner hz >= m_square2FrequencyTimer check being wrong, ignoring good samples
    //Only getting a small portion of what we want?
    uint8_t note = 0;
    //Need to check NR30 for if DAC is enabled
    if (bChannelEnabled && getNR30()) {
        //TODO - see if making this a loop and continually adding notes, 
        //and then making notes for the remainder of the hz fixes stutter.

        if (hz >= m_waveFrequencyTimer) {
            //hz -= m_square2FrequencyTimer;
            // m_square2FrequencyTimer = (2048 - m_square2Frequency) * 4;
            //nextDutyIndex = (nextDutyIndex + 1) % 8;
            //Might be overcomplicating things with time! Try with just notes, and stretch according to sample buffer size!
            //This might work since we KNOW that in the SDL side, a second of data should fill a 44Khz buffer!

            //According to reddit post here, this is actually a sample a hz!
            //https://www.reddit.com/r/EmuDev/comments/5gkwi5/gb_apu_sound_emulation/
            //Recommends to cut down to only gathering a sample every 95 hz, from CPU_CLOCK/Playback Frequency
            while (hz >= m_waveFrequencyTimer && m_waveFrequencyTimer >= 0) {
                /*
                nextDutyIndex = (nextDutyIndex + 1) % 8;
                note = SQUARE_DUTY_WAVEFORM_TABLE[(getNR11() >> 6) & 0x03][nextDutyIndex];
                */
                if (bFirstByteSample) {
                    bFirstByteSample = false;
                } else {
                    sampleByteIndex = (sampleByteIndex + 1) % 16;
                }

                note = 0x0F & m_gbmemory->direct_read(ADDRESS_WAVE_TABLE_DATA_START + sampleByteIndex + (bFirstByteSample ? 0 : 1));

                //Adjust note volume based on NR32
                switch ((getNR32() >> 5) & 0x11) {
                    case 0x00: //Note is silent
                        note = note >> 4;
                        break;
                    case 0x01: //Note is full volume
                        break;
                    case 0x10: //Note is half volme
                        note = note >> 1;
                        break;
                    case 0x11: //Note is 25% volume
                        note = note >> 2;
                        break;
                    default:
                        std::cout << "Invalid wave channel volume!" << std::endl;
                }

                //Not sure why volume is so loud...
                //note /= 20;
                memset(buffer, note, m_waveFrequencyTimer);
                buffer += m_waveFrequencyTimer;

                hz -= m_waveFrequencyTimer;
                m_waveFrequencyTimer = (2048 - m_waveFrequency) * 2;
            }

        } else {
            m_waveFrequencyTimer -= hz;
        }

        note = 0x0F & m_gbmemory->direct_read(ADDRESS_WAVE_TABLE_DATA_START + sampleByteIndex + (bFirstByteSample ? 0 : 1));

        //Adjust note volume based on NR32
        switch ((getNR32() >> 5) & 0x11) {
        case 0x00: //Note is silent
            note = note >> 4;
            break;
        case 0x01: //Note is full volume
            break;
        case 0x10: //Note is half volme
            note = note >> 1;
            break;
        case 0x11: //Note is 25% volume
            note = note >> 2;
            break;
        default:
            std::cout << "Invalid wave channel volume!" << std::endl;
        }
        
        //Not sure why note is so loud...
        //note /= 20;

    }

    if (true || hz < 100000) {
        //m_player->addNote(note, hz);
        memset(buffer, note, hz);
    }

    return note;
}

uint16_t GBAudio::tickNoise(uint8_t* buffer, long long hz){
    //Copied and edited form tickSquare2
    //Two counters: Length and frequency.
    static long long frequencyRollover = 0;
    static long long lengthRollover = 0;
    static long long volumeEnvelopeRollover = 0;
    static long long volumeEnvelopeCounterRollover = 0;
    static uint8_t nextDutyIndex = 0;

    //return;
    //m_square2LengthCounter -= hz;

    //When length is decremented to 0, channel is disabled. 
    //On length counter hitting 0, set volume to 0 but do not cease generation of data!
    //Length counter decrements each 256 Hz.
    //Only decremented if length counter enabled.
    //Check if length counter is enabled
    bool bChannelEnabled = true;
    if (m_noiseLengthEnable) {
        if ((hz + lengthRollover >= 256)) {
            if (m_noiseLengthCounter > 0) {
                m_noiseLengthCounter--;
            }
            lengthRollover = hz + lengthRollover - 256;
        } else {
            lengthRollover += hz;
        }

        if (m_noiseLengthCounter == 0) {
            bChannelEnabled = false;
            lengthRollover = 0;
        }
    }

    //Apparently period drives another timer?
    //http://www.retroisle.com/others/nintendogameboy/Technical/Firmware/GBsound.php

    //Check for volume sweep
    if (m_noiseEnvelopePeriod > 0) {
        if ((hz + volumeEnvelopeRollover) >= 64) {
            //signed so we can check for -1.
            m_noiseVolumeEnvelopeCounter = CLOCK_GB / m_noiseEnvelopePeriod;

            volumeEnvelopeRollover = hz + volumeEnvelopeRollover - 64;

        } else {
            volumeEnvelopeRollover += hz;
        }
    } else {
        volumeEnvelopeRollover = 0;
    }

    //Execute volume sweep
    if (volumeEnvelopeCounterRollover + hz >= m_noiseVolumeEnvelopeCounter) {
        int8_t newVolume = m_noiseVolume;
        if (getNR42() & 0x08) {
            newVolume++;
        } else {
            newVolume--;
        }

        if (newVolume >= 0 && newVolume <= 15) {
            m_noiseVolume = newVolume;
        } else {
            m_noiseEnvelopePeriod = 0;
        }

        //m_noiseVolumeEnvelopeCounter = CLOCK_GB / m_noiseEnvelopePeriod;
    }

    if (bChannelEnabled && !(getNR52() >> 7)) {
        bChannelEnabled = false;
    }

    //Stutters and low sounds may be due to inner hz >= m_square2FrequencyTimer check being wrong, ignoring good samples
    //Only getting a small portion of what we want?
    uint8_t note = 0;
    if (bChannelEnabled && m_noiseTriggered) {
        //TODO - see if making this a loop and continually adding notes, 
        //and then making notes for the remainder of the hz fixes stutter.
        
        //Is noise frequency even teh right way to describe this?
        if (hz >= m_noiseFrequencyTimer) {

            //According to reddit post here, this is actually a sample a hz!
            //https://www.reddit.com/r/EmuDev/comments/5gkwi5/gb_apu_sound_emulation/
            //Recommends to cut down to only gathering a sample every 95 hz, from CPU_CLOCK/Playback Frequency
            while (hz >= m_noiseFrequencyTimer && m_noiseFrequencyTimer >= 0) {
                uint16_t xorResult = ((m_lfsr & 0x01) ^ ((m_lfsr & 0x02) >> 1));
                m_lfsr = (m_lfsr >> 1) | (xorResult << 14);

                if (getNR43() & 0x08) {
                    //Set sixth bit to xorResult as well
                    m_lfsr &= ~(1 << 5) | 0x70;
                    m_lfsr |= (xorResult << 5);
                }

                note = (~m_lfsr & 0x1) * m_noiseVolume;

                //m_player->addNote(note, m_square2FrequencyTimer);
                memset(buffer, note, nextDutyIndex);
                buffer += m_noiseFrequencyTimer;

                hz -= m_noiseFrequencyTimer;
                m_noiseFrequencyTimer = getNoiseDivisor() << ((getNR43() & 0xF0) >> 4);
                //std::cout << "Duty Waveform: " << ((getNR21() >> 6) & 0x03) << std::endl;
                //std::cout << "Duty height : " << (uint16_t)(SQUARE_DUTY_WAVEFORM_TABLE[(getNR21() >> 6) & 0x03][nextDutyIndex]) << std::endl;
            }

        } else {
            m_noiseFrequencyTimer -= hz;
        }

        //Only output if bit 6 is set?
        note = (~m_lfsr & 0x1) * m_noiseVolume;

    }

    if (true || hz < 100000) {
        //m_player->addNote(note, hz);
        memset(buffer, note, hz);
    }

    return note;
}

uint8_t GBAudio::getNoiseDivisor() {
    uint8_t toReturn = 8;
    switch (getNR43() & 0x07) {
        case 0:
            toReturn = 8;
            break;
        case 1:
            toReturn = 16;
            break;
        case 2:
            toReturn = 32;
            break;
        case 3:
            toReturn = 48;
            break;
        case 4:
            toReturn = 64;
            break;
        case 5:
            toReturn = 80;
            break;
        case 6:
            toReturn = 96;
            break;
        case 7:
            toReturn = 112;
            break;
        default:
            toReturn = 8;
            break;
    }

    return toReturn;
}
        
void GBAudio::setPlayer(IAudioPlayer* player){
    m_player = player;
}

//Square Wave 1 Channel

//Sweep period, negate and shift
void GBAudio::setNR10(uint8_t val){
    //Leftmost bit is unused
    m_gbmemory->direct_write(ADDRESS_NR10, val & 0x7F);
}

uint8_t GBAudio::getNR10(){
    return m_gbmemory->direct_read(ADDRESS_NR10);
}

//Duty, length load
void GBAudio::setNR11(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR11, val);
}

uint8_t GBAudio::getNR11(){
    return m_gbmemory->direct_read(ADDRESS_NR11);
}

//Starting volume, envelope mode, period
void GBAudio::setNR12(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR12, val);
}

uint8_t GBAudio::getNR12(){
    return m_gbmemory->direct_read(ADDRESS_NR12);
}

//Frequency low byte
void GBAudio::setNR13(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR13, val);
    //Copied, edited from setNR23
    m_square1Frequency = (0x0700 & m_square1Frequency) | (val & 0xFF);
    m_square1FrequencyTimer = (2048 - m_square1Frequency) * 4;
}

uint8_t GBAudio::getNR13(){
    return m_gbmemory->direct_read(ADDRESS_NR13);
}

//trigger, has length, frequency high byte
void GBAudio::setNR14(uint8_t val){
    //Coppied, edited from setNR24
    //Check if trigger is being set
    if (val & CHANNEL_TRIGGER) {
        m_square1Triggered = true;
        //Enable channel, see length counter documentation

        //Check if length counter is 0, set to length load if so.
        if (m_square1LengthCounter == 0) {
            m_square1LengthCounter = 64;
        }

        //Load frequency timer with period (from NR22?)
        //m_square2FrequencyTimer = getNR22() & CHANNEL_PERIOD;
        m_square1Frequency = (((uint16_t)val & 0x0007) << 8) | (m_square1Frequency & 0x00FF);
        m_square1FrequencyTimer = (2048 - m_square1Frequency) * 4;

        //Load volume envelope timer with period (from NR22?)
        m_square1VolumeEnvelopeTimer = getNR12() & CHANNEL_PERIOD;//m_square2FrequencyTimer;

                                                                  //Load channel volume from NR22
        m_square1Volume = (getNR12() & CHANNEL_VOLUME_START) >> 4;

        //Other channel specific stuff... none to worry about with Square 2.

        //Check if channel DAC is off and disable self again if so.
        //DAC is checked using upper 5 bits of NR22
        if (!(getNR12() & (CHANNEL_VOLUME_START | CHANNEL_ENVELOPE))) {
            m_square1Triggered = false;
            val &= ~CHANNEL_TRIGGER;
            m_square1LengthCounter = 0;
            m_square1Triggered = false;
        }
    }


    //TL-- -FFF
    m_gbmemory->direct_write(ADDRESS_NR14, val & 0xC7);
}

uint8_t GBAudio::getNR14(){
    return m_gbmemory->direct_read(ADDRESS_NR14);
}

//Square Wave 2 Channel

//Duty, length load
void GBAudio::setNR21(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR21, val);
}

uint8_t GBAudio::getNR21(){
    return m_gbmemory->direct_read(ADDRESS_NR21);
}

//Start volume, envelope mode, period
void GBAudio::setNR22(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR22, val);
}

uint8_t GBAudio::getNR22(){
    return m_gbmemory->direct_read(ADDRESS_NR22);
}

//Frequency low byte
void GBAudio::setNR23(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR23, val);
    m_square2Frequency = (0x0700 & m_square2Frequency) | (val & 0xFF);
    m_square2FrequencyTimer = (2048 - m_square2Frequency) * 4;
}

uint8_t GBAudio::getNR23(){
    return m_gbmemory->direct_read(ADDRESS_NR23);
}

//Trigger, has length, frequency high byte
void GBAudio::setNR24(uint8_t val){
    //Check if trigger is being set
    if(val & CHANNEL_TRIGGER){
        m_square2Triggered = true;
        //Enable channel, see length counter documentation
        
        //Check if length counter is 0, set to length load if so.
        if(m_square2LengthCounter == 0){
            m_square2LengthCounter = 64;
        }
        
        //Load frequency timer with period (from NR22?)
        //m_square2FrequencyTimer = getNR22() & CHANNEL_PERIOD;
        m_square2Frequency = (((uint16_t)val & 0x0007) << 8) | (m_square2Frequency & 0x00FF);
        m_square2FrequencyTimer = (2048 - m_square2Frequency) * 4;

        //Load volume envelope timer with period (from NR22?)
        m_square2VolumeEnvelopeTimer = getNR22() & CHANNEL_PERIOD;//m_square2FrequencyTimer;
        
        //Load channel volume from NR22
        m_square2Volume = (getNR22() & CHANNEL_VOLUME_START) >> 4;
        
        //Other channel specific stuff... none to worry about with Square 2.
        
        //Check if channel DAC is off and disable self again if so.
        //DAC is checked using upper 5 bits of NR22
        if(!(getNR22() & (CHANNEL_VOLUME_START | CHANNEL_ENVELOPE))){
            m_square2Triggered = false;
            val &= ~CHANNEL_TRIGGER;
            m_square2LengthCounter = 0;
            m_square2Triggered = false;
        }
    }
    
    //TL-- -FFF
    m_gbmemory->direct_write(ADDRESS_NR24, val & 0xC7);
    
}

uint8_t GBAudio::getNR24(){
    return m_gbmemory->direct_read(ADDRESS_NR24);
}

//Wave Table Channel

//DAC power
void GBAudio::setNR30(uint8_t val){
    //Only upper most bit is used
    m_gbmemory->direct_write(ADDRESS_NR30, val & 0x80);
}

uint8_t GBAudio::getNR30(){
    return m_gbmemory->direct_read(ADDRESS_NR30);
}

//Length load
void GBAudio::setNR31(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR31, val);
}

uint8_t GBAudio::getNR31(){
    return m_gbmemory->direct_read(ADDRESS_NR31);
}

// Volume code
void GBAudio::setNR32(uint8_t val){
    //-VV- ----
    m_gbmemory->direct_write(ADDRESS_NR32, val & 0x60);
}

uint8_t GBAudio::getNR32(){
    return m_gbmemory->direct_read(ADDRESS_NR32);
}

//Frequency low byte
void GBAudio::setNR33(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR33, val);
    //Copied, adapted from Square 2
    m_waveFrequency = (0x0700 & m_waveFrequency) | (val & 0xFF);
    m_waveFrequencyTimer = (2048 - m_waveFrequency) * 2;
}

uint8_t GBAudio::getNR33(){
    return m_gbmemory->direct_read(ADDRESS_NR33);
}

//Trigger, has length, frequency high byte
void GBAudio::setNR34(uint8_t val){
    //Code adapted from setNR24
    //Check if trigger is being set
    if (val & CHANNEL_TRIGGER) {
        m_waveTriggered = true;
        //Enable channel, see length counter documentation

        //Check if length counter is 0, set to length load if so.
        if (m_waveLengthCounter == 0) {
            m_square2LengthCounter = 256;
        }

        //Load frequency timer with period (from NR22?)
        //m_square2FrequencyTimer = getNR22() & CHANNEL_PERIOD;
        m_waveFrequency = (((uint16_t)val & 0x0007) << 8) | (m_waveFrequency & 0x00FF);
        m_waveFrequencyTimer = (2048 - m_square2Frequency) * 2;

        //Not applicable to wave channel
        //Load volume envelope timer with period (from NR22?)
        //m_square2VolumeEnvelopeTimer = getNR22() & CHANNEL_PERIOD;//m_square2FrequencyTimer;

        //Not applicable to wave channel
        //Load channel volume from NR22
        //m_square2Volume = (getNR22() & CHANNEL_VOLUME_START) >> 4;

        //Other channel specific stuff... none to worry about with Square 2.

        //Check if channel DAC is off and disable self again if so.
        //DAC is checked using upper 5 bits of NR22
        if (!getNR30()) {
            m_waveTriggered = false;
            val &= ~CHANNEL_TRIGGER;
            m_waveLengthCounter = 0;
        }
    }

    //TL-- -FFF
    m_gbmemory->direct_write(ADDRESS_NR34, 0xC7);
}

uint8_t GBAudio::getNR34(){
    return m_gbmemory->direct_read(ADDRESS_NR34);
}

//Noise Channel

//Length load
void GBAudio::setNR41(uint8_t val){
    //--LL LLLL
    m_gbmemory->direct_write(ADDRESS_NR41, val & 0x3F);
}

uint8_t GBAudio::getNR41(){
    return m_gbmemory->direct_read(ADDRESS_NR41);
}

//Starting volume, envelope mode, period
void GBAudio::setNR42(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR42, val);
}

uint8_t GBAudio::getNR42(){
    return m_gbmemory->direct_read(ADDRESS_NR42);
}

//Clock shift, LFSR width, divisor code
void GBAudio::setNR43(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR43, val);
}

uint8_t GBAudio::getNR43(){
    return m_gbmemory->direct_read(ADDRESS_NR43);
}

//Trigger, has length
void GBAudio::setNR44(uint8_t val){
    //Adapted from setNR24
    //Check if trigger is being set
    if (val & CHANNEL_TRIGGER) {
        m_noiseTriggered = true;
        //Enable channel, see length counter documentation

        //Check if length counter is 0, set to length load if so.
        
        if (m_noiseLengthCounter == 0) {
            m_noiseLengthCounter = 64;
        }

        //Load frequency timer with period (from NR22?)
        //m_square2FrequencyTimer = getNR22() & CHANNEL_PERIOD;
        m_noiseFrequencyTimer = getNoiseDivisor() << ((getNR43() & 0xF0) >> 4);

        //Load volume envelope timer with period (from NR22?)
        //m_square2VolumeEnvelopeTimer = getNR22() & CHANNEL_PERIOD;//m_square2FrequencyTimer;

                                                                  //Load channel volume from NR22
        m_noiseVolume = (getNR42() & CHANNEL_VOLUME_START) >> 4;

        m_noiseVolumeEnvelopeCounter = 64;
        m_noiseEnvelopePeriod = getNR42() & 0x07;
        //Other channel specific stuff... none to worry about with Square 2.
        m_lfsr = 0x7FFF;

        m_noiseLengthEnable = (val & CHANNEL_LENGTH_ENABLE) > 0;

        //Check if channel DAC is off and disable self again if so.
        //DAC is checked using upper 5 bits of NR22
        if (!(getNR42() & (CHANNEL_VOLUME_START | CHANNEL_ENVELOPE))) {
            //m_noiseTrigg = false;
            val &= ~CHANNEL_TRIGGER;
            m_noiseFrequencyTimer = 0;
            m_noiseVolume = 0;
           // m_square2Triggered = false;
        }
    }

    //TL-- ----
    m_gbmemory->direct_write(ADDRESS_NR44, 0xC0);
}

uint8_t GBAudio::getNR44(){
    return m_gbmemory->direct_read(ADDRESS_NR44);
}

//Audio control

//Vin left/right enable, volume left/right.
void GBAudio::setNR50(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR50, val);
}

uint8_t GBAudio::getNR50(){
    return m_gbmemory->direct_read(ADDRESS_NR50);
}

//Left/right enables
void GBAudio::setNR51(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR51, val);
}

uint8_t GBAudio::getNR51(){
    return m_gbmemory->direct_read(ADDRESS_NR51);
}

//Power status, channel length status
void GBAudio::setNR52(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR52, val & 0x8F);

    //If audio is disabled, reset everything
    if (!(val >> 7)) {
        m_square1Triggered = false;
        m_square2Triggered = false;
        m_waveTriggered = false;
        m_noiseTriggered = false;
        m_square1Duty = 0;
        m_square2Duty = 0;
        m_square1Volume = 0;
        m_square2Volume = 0;
        m_noiseVolume = 0;
        m_square1Frequency = 0;
        m_square2Frequency = 0;
        m_waveFrequency = 0;
        m_noiseFrequency = 0;
        m_square1FrequencyTimer = 0;
        m_square2FrequencyTimer = 0;
        m_waveFrequencyTimer = 0;
        m_noiseFrequencyTimer = 0;
        m_square1LengthCounter = 0;
        m_square2LengthCounter = 0;
        m_waveLengthCounter = 0;
        m_noiseLengthCounter = 0;
        m_lfsr = 0;
    }
}

uint8_t GBAudio::getNR52(){
    return m_gbmemory->direct_read(ADDRESS_NR52);
}
