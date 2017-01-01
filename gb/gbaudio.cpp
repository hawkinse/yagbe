#include "gbaudio.h"
#include <stdlib.h>
#include <iostream>
#include "gbmem.h"

GBAudio::GBAudio(GBMem* mem){
    m_gbmemory = mem;
}

GBAudio::~GBAudio(){
    
}

void GBAudio::tick(long long hz){
    //std::cout << "Unimplemented audio tick " << +hz << " cycles" << std::endl;
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
}

uint8_t GBAudio::getNR13(){
    return m_gbmemory->direct_read(ADDRESS_NR13);
}

//trigger, has length, frequency high byte
void GBAudio::setNR14(uint8_t val){
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
}

uint8_t GBAudio::getNR23(){
    return m_gbmemory->direct_read(ADDRESS_NR23);
}

//Trigger, has length, frequency high byte
void GBAudio::setNR24(uint8_t val){
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
}

uint8_t GBAudio::getNR33(){
    return m_gbmemory->direct_read(ADDRESS_NR33);
}

//Trigger, has length, frequency high byte
void GBAudio::setNR34(uint8_t val){
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
}

uint8_t GBAudio::getNR52(){
    return m_gbmemory->direct_read(ADDRESS_NR52);
}
