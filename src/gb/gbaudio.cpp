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
    uint8_t* tempBuffer = new uint8_t[hz];
    uint8_t* mixedBuffer = new uint8_t[hz];
    uint16_t currentNote = 0;
    uint16_t mixedNote = 0;
	
    uint32_t skip = (CLOCK_GB * MHZ_TO_HZ) / m_player->getSampleRate() / 2;
    if ((rollover + hz) >= skip) {
		
		currentNote = tickSquare1(tempBuffer, hz);
		m_player->mixNotes(&currentNote, &mixedNote, 1);

		currentNote = tickSquare2(tempBuffer, hz);
		m_player->mixNotes(&currentNote, &mixedNote, 1);

		currentNote = tickWave(tempBuffer, hz);
		m_player->mixNotes(&currentNote, &mixedNote, 1);

		currentNote = tickNoise(tempBuffer, hz);
		m_player->mixNotes(&currentNote, &mixedNote, 1);
		
        m_player->addNote(mixedNote * 500, 1);
        rollover = rollover + hz - skip;
    } else {
		
		tickSquare1(tempBuffer, hz);
		tickSquare2(tempBuffer, hz);
		tickWave(tempBuffer, hz);
		tickNoise(tempBuffer, hz);
		
        rollover += hz;
    }

    delete tempBuffer;
    delete mixedBuffer;
}

uint16_t GBAudio::tickSquare1(uint8_t* buffer, long long hz){
    static uint8_t nextDutyIndex = 0;
    bool bChannelEnabled = true;
    
	//Execute volume envelope
	static long long volumeEnvelopeCounter = 0;
	if ((getNR12() & 0x7)) {
		m_square1VolumeEnvelopeTimer += hz;
		
		while (m_square1VolumeEnvelopeTimer > (64 * 512 * (getNR12() & 0x7))) {
			if (getNR12() & 0x7) {
				if ((getNR12() & 0x08) && (m_square1Volume < 15)) {
					m_square1Volume++;
				}
				else if (!(getNR12() & 0x08) && (m_square1Volume > 0)) {
					m_square1Volume--;
				}
			}
			
			m_square1VolumeEnvelopeTimer -= 64 * 512 * (getNR12() & 0x7) * 2; //Test * 2 to see if this fixes short note problem...
		}
	}
	else {
		volumeEnvelopeCounter = 0;
	}

    //Execute length counter
	if ((getNR14() & 0x40)) {
		if (m_square1LengthCounter > 0) {
			m_square1LengthCounter -= hz;
		}

		if (m_square1LengthCounter <= 0) {
			m_square1Volume = 0;
		}
	}

    if (bChannelEnabled && !(getNR52() >> 7)) {
        bChannelEnabled = false;
		m_square1Volume = 0;
    }
    
	//Square 1 Frequency Sweep
	m_square1FrequencySweepTimer -= hz;
	//Only perform sweep if sweep period and shift aren't 0
	if (m_square1FrequencySweepTimer <= 0 && (getNR10() & 0x70) && (getNR10() & 0x07)) {
		//Documentation says adjustment is ran twice, but only first result is stored.
		for (int checkCount = 0; checkCount < 2; checkCount++) {			
			//New frequency is trigger frequency shifted right., optionally negated, and then added with added to the original frequency.
			uint16_t newFrequency = m_square1Frequency >> (getNR10() & 0x7);
			
			//Check for negation
			if (getNR10() & 0x8) {
				newFrequency *= -1;
			}

			//Sum frequencies
			newFrequency += m_square1Frequency;
            
			//Only keep new frequency if under 2048
			if (newFrequency < 2048) {
				//Only keep frequency if doing inital check.
				if (checkCount == 0) {
					m_square1TriggerFrequency = newFrequency;
					m_square1Frequency = newFrequency;
				}
			}
			else {
				//Both checks can disable the channel.
				m_square1Volume = 0;
			}
			
		}

		//Reload timer
		//Add current value because current value is negative, contains unused hz.
		m_square1FrequencySweepTimer += (128 * 512);
	}
    
    //Stutters and low sounds may be due to inner hz >= m_square2FrequencyTimer check being wrong, ignoring good samples
    //Only getting a small portion of what we want?
    uint8_t note = 0;
    if (bChannelEnabled) {

        if (hz >= m_square1FrequencyTimer) {
           
            while (hz >= m_square1FrequencyTimer && m_square1FrequencyTimer >= 0) {
                nextDutyIndex = (nextDutyIndex + 1) % 8;
                note = SQUARE_DUTY_WAVEFORM_TABLE[(getNR11() >> 6) & 0x03][nextDutyIndex];
                note *= m_square1Volume;
                memset(buffer, note, m_square1FrequencyTimer);
                buffer += m_square1FrequencyTimer;

                hz -= m_square1FrequencyTimer;
                m_square1FrequencyTimer = (2048 - m_square1Frequency) * 4;
            }

        } else {
            m_square1FrequencyTimer -= hz;
        }

        note = SQUARE_DUTY_WAVEFORM_TABLE[(getNR11() >> 6) & 0x03][nextDutyIndex];
        note *= m_square1Volume;

    } 

    return note;
}

uint16_t GBAudio::tickSquare2(uint8_t* buffer, long long hz) {
    static uint8_t nextDutyIndex = 0;
    bool bChannelEnabled = true;

	//Execute volume envelope
	static long long volumeEnvelopeCounter = 0;
	if (getNR22() & 0x07) {
		m_square2VolumeEnvelopeTimer += hz;
		while (m_square2VolumeEnvelopeTimer > (64 * 512 * (getNR22() & 0x07))) {
			if ((getNR22() & 0x07)) {
				if ((getNR22() & 0x08) && (m_square2Volume < 15)) {
					m_square2Volume++;
				}
				else if (!(getNR22() & 0x08) && (m_square2Volume > 0)) {
					m_square2Volume--;
				}
			}
			m_square2VolumeEnvelopeTimer -= 64 * 512 * (getNR22() & 0x07) * 2;
		}
	}
	else {
		volumeEnvelopeCounter = 0;
	}

    //Execute length counter
	if ((getNR24() & 0x40)) {
		if (m_square2LengthCounter > 0) {
			m_square2LengthCounter -= hz;
		}

		if (m_square2LengthCounter <= 0) {
			m_square2Volume = 0;
		}
	}

    if (bChannelEnabled && !(getNR52() >> 7)) {
        bChannelEnabled = false;
    }

    uint8_t note = 0;
    if (bChannelEnabled) {
        if (hz >= m_square2FrequencyTimer) {
           
            while (hz >= m_square2FrequencyTimer && m_square2FrequencyTimer >= 0) {
                nextDutyIndex = (nextDutyIndex + 1) % 8;
                note = SQUARE_DUTY_WAVEFORM_TABLE[(getNR21() >> 6) & 0x03][nextDutyIndex];
                note *= m_square2Volume;
                memset(buffer, note, m_square2FrequencyTimer);
                buffer += m_square2FrequencyTimer;

                hz -= m_square2FrequencyTimer;
                m_square2FrequencyTimer = (2048 - m_square2Frequency) * 4;
            }

        } else {
            m_square2FrequencyTimer -= hz;
        }

        note = SQUARE_DUTY_WAVEFORM_TABLE[(getNR21() >> 6) & 0x03][nextDutyIndex];
        note *= m_square2Volume;
    }
    
    if (true || hz < 100000) {
        memset(buffer, note, hz);
    }

    return note;
}

uint16_t GBAudio::tickWave(uint8_t* buffer, long long hz){
    //Samples are stored as 4-bit values, two per byte.
    static bool bFirstByteSample = true;
    bool bChannelEnabled = true;

    if (bChannelEnabled && !(getNR52() >> 7)) {
        bChannelEnabled = false;
    }

    uint8_t note = 0;
    //Need to check NR30 for if DAC is enabled
    if (bChannelEnabled && getNR30()) {

        if (hz >= m_waveFrequencyTimer) {
            while (hz >= m_waveFrequencyTimer && m_waveFrequencyTimer >= 0) {
                if (!bFirstByteSample) {
					m_waveSampleByteIndex = (m_waveSampleByteIndex + 1) % 16;
                }
                
				bFirstByteSample = !bFirstByteSample;

                hz -= m_waveFrequencyTimer;
				m_waveFrequencyTimer = (2048 - m_waveFrequency) * 2;
            }

        } else {
            m_waveFrequencyTimer -= hz;
        }

        note = m_gbmemory->direct_read(ADDRESS_WAVE_TABLE_DATA_START + m_waveSampleByteIndex);
				
        if(bFirstByteSample){
            note = note >> 4;
        }
                
        note &= 0x0F;

        //Adjust note volume based on NR32
        switch ((getNR32() >> 5) & 0x3) {
        case 0x00: //Note is silent
			note =  note >> 4;
            break;
        case 0x01: //Note is full volume
            break;
        case 0x02: //Note is half volme
            note = note >> 1;
            break;
        case 0x03: //Note is 25% volume
            note = note >> 2;
            break;
        default:
            std::cout << "Invalid wave channel volume " << +((getNR32() >> 5) & 0x3) << std::endl;
        }
    }

    return note;
}

uint16_t GBAudio::tickNoise(uint8_t* buffer, long long hz){
    static uint8_t nextDutyIndex = 0;
    bool bChannelEnabled = true;

	//Execute volume envelope
	if ((getNR42() & 0x07)) {
		m_noiseVolumeEnvelopePeriod += hz;
		while (m_noiseVolumeEnvelopePeriod > (64 * 512 * (getNR42() & 0x07))) {
			if ((getNR42() & 0x07)) {
				if ((getNR42() & 0x08) && (m_noiseVolume < 15)) {
					m_noiseVolume++;
				}
				else if (!(getNR42() & 0x08) && (m_noiseVolume > 0)) {
					m_noiseVolume--;
				}
			}
			m_noiseVolumeEnvelopePeriod -= 64 * 512 * (getNR42() & 0x07) * 2;
		}
	}

	//Execute Length Counter
	if (getNR44() & 0x40) {
		m_noiseLengthCounter -= hz;
		if (m_noiseLengthCounter <= 0) {
			m_noiseVolume = 0;
		}
	}
	
    if (bChannelEnabled && !(getNR52() >> 7)) {
        bChannelEnabled = false;
    }

    uint8_t note = 0;
    if (bChannelEnabled && m_noiseTriggered) {
        if (hz >= m_noiseFrequencyTimer) {
            while (hz >= m_noiseFrequencyTimer && m_noiseFrequencyTimer >= 0) {
                uint16_t xorResult = ((m_lfsr & 0x01) ^ ((m_lfsr & 0x02) >> 1));
                m_lfsr = ((m_lfsr >> 1) & 0x3FFF) | (xorResult << 14);

                if (getNR43() & 0x08) {
                    //Set sixth bit to xorResult as well
                    m_lfsr &= ~(1 << 5);
                    m_lfsr |= (xorResult << 5);
                }

                note = (~m_lfsr & 0x1) * m_noiseVolume;

                memset(buffer, note, nextDutyIndex);
                buffer += m_noiseFrequencyTimer;

                hz -= m_noiseFrequencyTimer;
                m_noiseFrequencyTimer = getNoiseDivisor() << ((getNR43() & 0xF0) >> 4);
            }

        } else {
            m_noiseFrequencyTimer -= hz;
        }

        note = (~m_lfsr & 0x1) * m_noiseVolume;

    }

    if (true || hz < 100000) {
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
	m_square1FrequencySweepTimer = 128 * 512 * ((val >> 4) & 0x07);
}

uint8_t GBAudio::getNR10(){
    return m_gbmemory->direct_read(ADDRESS_NR10);
}

//Duty, length load
void GBAudio::setNR11(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR11, val);
	m_square1LengthCounter = (val & 0x3F) * 256 * 512;
}

uint8_t GBAudio::getNR11(){
    return m_gbmemory->direct_read(ADDRESS_NR11);
}

//Starting volume, envelope mode, period
void GBAudio::setNR12(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR12, val);
	m_square1VolumeEnvelopeTimer = 0;
	m_square1Volume = (val & 0xF0) >> 4;
}

uint8_t GBAudio::getNR12(){
    return m_gbmemory->direct_read(ADDRESS_NR12);
}

//Frequency low byte
void GBAudio::setNR13(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR13, val);
    m_square1Frequency = (0x0700 & m_square1Frequency) | (val & 0xFF);
    m_square1FrequencyTimer = (2048 - m_square1Frequency) * 4;
}

uint8_t GBAudio::getNR13(){
    return m_gbmemory->direct_read(ADDRESS_NR13);
}

//trigger, has length, frequency high byte
void GBAudio::setNR14(uint8_t val){
	//TL-- -FFF
	m_gbmemory->direct_write(ADDRESS_NR14, val & 0xC7);

	//Check if length counter is 0, set to length load if so.
	if (m_square1LengthCounter == 0) {
		m_square1LengthCounter = (getNR11() & 0x3F) * 256 * 512;//64;
	}

	m_square1Frequency = (((uint16_t)val & 0x0007) << 8) | (m_square1Frequency & 0x00FF);
	m_square1FrequencyTimer = (2048 - m_square1Frequency) * 4;

	//Load channel volume from NR22
	m_square1Volume = (getNR12() & CHANNEL_VOLUME_START) >> 4;

	//Set up frequency sweep variables
	m_square1TriggerFrequency = m_square1Frequency;
    m_square1FrequencySweepTimer = 128 * 512;


	//Check if channel DAC is off and disable self again if so.
	//DAC is checked using upper 5 bits of NR22
	if (!(getNR12() & (CHANNEL_VOLUME_START | CHANNEL_ENVELOPE))) {
		m_square1Triggered = false;
		val &= ~CHANNEL_TRIGGER;
		m_square1LengthCounter = 0;
		m_square1Triggered = false;
	}

    //Coppied, edited from setNR24
    //Check if trigger is being set
    if (val & CHANNEL_TRIGGER) {
        m_square1Triggered = true;

		//Set up frequency sweep variables
		m_square1TriggerFrequency = m_square1Frequency;
    }
}

uint8_t GBAudio::getNR14(){
    return m_gbmemory->direct_read(ADDRESS_NR14);
}

//Square Wave 2 Channel

//Duty, length load
void GBAudio::setNR21(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR21, val);
	m_square2LengthCounter = (val & 0x3F) * 256 * 512;
}

uint8_t GBAudio::getNR21(){
    return m_gbmemory->direct_read(ADDRESS_NR21);
}

//Start volume, envelope mode, period
void GBAudio::setNR22(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR22, val);
	m_square2VolumeEnvelopeTimer = 0;
	m_square2Volume = (val & 0xF0) >> 4;
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
	//TL-- -FFF
	m_gbmemory->direct_write(ADDRESS_NR24, val & 0xC7);

	//Check if length counter is 0, set to length load if so.
	if (m_square2LengthCounter == 0) {
		m_square2LengthCounter = (getNR21() & 0x3F) * 256 * 512;
	}

	//Load frequency timer with period
	m_square2Frequency = (((uint16_t)val & 0x0007) << 8) | (m_square2Frequency & 0x00FF);
	m_square2FrequencyTimer = (2048 - m_square2Frequency) * 4;
    
	//Load channel volume from NR22
	m_square2Volume = (getNR22() & CHANNEL_VOLUME_START) >> 4;

	//Check if channel DAC is off and disable self again if so.
	//DAC is checked using upper 5 bits of NR22
	if (!(getNR22() & (CHANNEL_VOLUME_START | CHANNEL_ENVELOPE))) {
		m_square2Triggered = false;
		val &= ~CHANNEL_TRIGGER;
		m_square2LengthCounter = 0;
		m_square2Triggered = false;
	}

    //Check if trigger is being set
    if(val & CHANNEL_TRIGGER){
        m_square2Triggered = true;
        //TODO - See if commented out sutff can be deleted before committing to branch!
        /*
        //Enable channel, see length counter documentation
        
        //Load frequency timer with period (from NR22?)
        //m_square2FrequencyTimer = getNR22() & CHANNEL_PERIOD;
        m_square2Frequency = (((uint16_t)val & 0x0007) << 8) | (m_square2Frequency & 0x00FF);
        m_square2FrequencyTimer = (2048 - m_square2Frequency) * 4;

        //Load volume envelope timer with period (from NR22?)
        //m_square2VolumeEnvelopeTimer = getNR22() & CHANNEL_PERIOD;//m_square2FrequencyTimer;
        
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
         */
    }    
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
	m_waveLengthCounter = 1 * 512 * 256;
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

uint8_t m_waveFrequencyNextLow;
//Frequency low byte
void GBAudio::setNR33(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR33, val);
    m_waveFrequency = (0x0700 & m_waveFrequency) | ((uint16_t)val & 0x00FF);
	m_waveFrequencyTimer = (2048 - m_waveFrequency) * 2;
}

uint8_t GBAudio::getNR33(){
    return m_gbmemory->direct_read(ADDRESS_NR33);
}

//Trigger, has length, frequency high byte
void GBAudio::setNR34(uint8_t val){
	//TL-- -FFF
	m_gbmemory->direct_write(ADDRESS_NR34, val & 0xC7);

	m_waveFrequency = 0x7FF & (((uint16_t)val) << 8) | ((uint16_t)getNR33() & 0x00FF);
	m_waveFrequencyTimer = (2048 - m_waveFrequency) * 2;
	
	//Check if length counter is 0, set to length load if so.
	if (m_waveLengthCounter == 0) {
		m_waveLengthCounter = 1 * 512 * 256;
	}

    //Check if trigger is being set
    if (val & CHANNEL_TRIGGER) {
        m_waveTriggered = true;
        
        //TODO - See if things still work when this is commented out before commit
        /*
        //Enable channel, see length counter documentation

        //Check if length counter is 0, set to length load if so.
        if (m_waveLengthCounter == 0) {
            m_waveLengthCounter = 256;
        }

        //Load frequency timer with period
        //m_waveFrequency = (((uint16_t)val & 0x0007) << 9) | (m_waveFrequency & 0x00FF);
		m_waveFrequency = (((uint16_t)val & 0x0007) << 8) | ((uint16_t)getNR33() & 0x00FF);
        m_waveFrequencyTimer = (2048 - m_waveFrequency) * 2;


        //Check if channel DAC is off and disable self again if so.
        if (!getNR30()) {
            m_waveTriggered = false;
            val &= ~CHANNEL_TRIGGER;
            m_waveLengthCounter = 0;
        }
        */
        
        m_waveSampleByteIndex = 0;
    }
}

uint8_t GBAudio::getNR34(){
    return m_gbmemory->direct_read(ADDRESS_NR34);
}

//Noise Channel

//Length load
void GBAudio::setNR41(uint8_t val){
    //--LL LLLL
    m_gbmemory->direct_write(ADDRESS_NR41, val & 0x3F);
	m_noiseLengthCounter = 1 * 512 * 256;
}

uint8_t GBAudio::getNR41(){
    return m_gbmemory->direct_read(ADDRESS_NR41);
}

//Starting volume, envelope mode, period
void GBAudio::setNR42(uint8_t val){
    m_gbmemory->direct_write(ADDRESS_NR42, val);
	m_noiseVolumeEnvelopePeriod = 0;
	m_noiseVolume = (val & CHANNEL_VOLUME_START) >> 4;
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
	//TL-- ----
	m_gbmemory->direct_write(ADDRESS_NR44, val & 0xC0);

	m_noiseFrequencyTimer = getNoiseDivisor() << ((getNR43() & 0xF0) >> 4);

	if (m_noiseLengthCounter == 0) {
		m_noiseLengthCounter = 64 * 512 * 256;
	}

	//Reset noise LFSR
	m_lfsr = 0x7FFF;

	m_noiseLengthEnable = (val & CHANNEL_LENGTH_ENABLE) > 0;

    //Adapted from setNR24
    //Check if trigger is being set
    if (val & CHANNEL_TRIGGER) {
        m_noiseTriggered = true;
        
        //Check if channel DAC is off and disable self again if so.
        //DAC is checked using upper 5 bits of NR22
        if (!(getNR42() & (CHANNEL_VOLUME_START | CHANNEL_ENVELOPE))) {
            //m_noiseTrigg = false;
            val &= ~CHANNEL_TRIGGER;
            m_noiseFrequencyTimer = 0;
            m_noiseVolume = 0;
        }
    }
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
    m_gbmemory->direct_write(ADDRESS_NR52, val & 0x80);

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
