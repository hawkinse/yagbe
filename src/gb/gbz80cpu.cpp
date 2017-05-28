#include <iostream>
#include "gbz80cpu.h"
#include "opcodes.h"
#include "opcycles.h"
#include "gbmem.h"
#include "gblcd.h"
#include "bytehelpers.h"

GBZ80::GBZ80(GBMem* memory, GBLCD* lcd, GBAudio* audio, SGBHandler* sgbhandler){
    m_gbmemory = memory;
    m_gblcd = lcd;
    m_gbaudio = audio;
    m_bSingleStep = SINGLE_STEP;
    m_sgbhandler = sgbhandler;
    init();
}

GBZ80::~GBZ80(){
}

void GBZ80::init(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "GBZ80cpu init!" << "\n";
    
    //Set clock speed
    m_Clock = CLOCK_GB;
    
    
    m_bInterruptsEnabled = false;
    m_bInterruptsEnabledNext = false;
    m_bHalt = false;
    m_bStop = false;
    
    //If we have a boot rom loaded and enabled, don't worry about manually setting memory
    if(m_gbmemory->getBootRomEnabled()){
        std::cout << "Boot rom is enabled." << std::endl;
        AF = 0x0000;
        BC = 0x0000;
        DE = 0x0000;
        HL = 0x0000;
        SP = 0x0000;
        PC = 0x00;
        return;
    } 
    
    std::cout << "No boot rom. Using predetermined init values" << std::endl;
    
    //Startup values from pandoc
    AF = 0x01B0;
    BC = 0x0013;
    DE = 0x00D8;
    HL = 0x014D;
    SP = 0xFFFE;
    PC = 0x100;
    
    m_gbmemory->write(0xFF05, 0x00);
    m_gbmemory->write(0xFF06, 0x00);
    m_gbmemory->write(0xFF07, 0x00);
    m_gbmemory->write(0xFF10, 0x80);
    m_gbmemory->write(0xFF11, 0xBF);
    m_gbmemory->write(0xFF12, 0xF3);
    m_gbmemory->write(0xFF14, 0xBF);
    m_gbmemory->write(0xFF16, 0x3F);
    m_gbmemory->write(0xFF17, 0x00);
    m_gbmemory->write(0xFF1A, 0x7F);
    m_gbmemory->write(0xFF1B, 0xFF);
    m_gbmemory->write(0xFF1C, 0x9F);
    m_gbmemory->write(0xFF1E, 0xFB);
    m_gbmemory->write(0xFF20, 0xFF);
    m_gbmemory->write(0xFF21, 0x00);
    m_gbmemory->write(0xFF22, 0x00);
    m_gbmemory->write(0xFF23, 0xBF);
    m_gbmemory->write(0xFF24, 0x77);
    m_gbmemory->write(0xFF25, 0xF3);
    
    if(m_sgbhandler == NULL){
        m_gbmemory->write(0xFF26, 0xF0); //0xF1 for DMG, 0xF0 for super gameboy
    } else {
        m_gbmemory->write(0xFF26, 0xF1);
    }
    
    m_gbmemory->write(0xFFFF, 0x00);
    
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "CPU init complete\n\n";
}

void GBZ80::tick(float deltaTime){    
    //Get the next instruction and its cycle length
    uint8_t nextInstruction = m_gbmemory->read(PC);
    uint8_t nextCycleLength = get_cycle_length(nextInstruction);
    
    //Determine the amount of cycles to run this tick.
    static long long timeRollover = 0;
    long long cycles = 0;
    if(m_bSingleStep){
        //Set cycles based on the next instruction
        cycles = nextCycleLength;
    } else {
        //Set cycles based on the given deltaTime and existing rollover
        cycles = (deltaTime * m_Clock * MHZ_TO_HZ) + timeRollover;
    }
    
    //Hacky workaround to broken SDL when not rendering due to halted CPU
    if(deltaTime == 0){
        cycles = 4; 
    }
    
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "GBZ80 - Clock cycles this tick: " << cycles << std::endl;
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "GBZ80 - input delta time: " << deltaTime << std::endl;
    
    //Run until we hit an instruction requiring more cycles than we have time for
    while(cycles >= nextCycleLength){
        
        //Run next instruction if CPU is active
        if(!(m_bStop || (m_bHalt && !IGNORE_HALT))){
            //Run the next instruction
            uint8_t directRead = m_gbmemory->read(PC);
            uint8_t nextInstr = read_next_byte();
            process_opcode(nextInstr);
        }
        
        //Update LCD even in stop state. Suspect that suspending LCD doesn't actually entierly disable it.
        m_gblcd->tick(nextCycleLength);
        
        //Memory tick is for system timer functionality.
        //TODO - move timer stuff into its own class!
        m_gbmemory->tick(nextCycleLength);
        
        
        m_gbaudio->tick(nextCycleLength);
        
        if(m_sgbhandler != NULL){
            m_sgbhandler->tick(nextCycleLength);
        }
        
        if(m_bStop){
            if(CONSOLE_OUTPUT_ENABLED) std::cout << "Processor is stopped" << std::endl;
            
            //Stop typically indicates we've hit an unimplemented opcode
            if(STOP_ON_STOP){
                exit(0);
            }
        }
        
        if(m_bHalt){
            if(CONSOLE_OUTPUT_ENABLED) std::cout << "Processor is halted" << std::endl;
            
            if(STOP_ON_HALT){
                exit(0);
            }
        }
        
        if(CONSOLE_OUTPUT_REGISTERS){
            showRegisters();
        }
        
        //Handle any interrupts
        process_interrupts();
        
        //Turn on interrupts from EI instruction
        if(m_bInterruptsEnabledNext){
            m_bInterruptsEnabled = true;
            m_bInterruptsEnabledNext = false;
        }
        
        //Decrement cycles
        cycles -= nextCycleLength;
        
        //Set the cycle length for the next instruction
        nextInstruction = m_gbmemory->read(PC);
        nextCycleLength = get_cycle_length(nextInstruction);
    }
    
    //Store any unused cycles for next tick
    timeRollover = (cycles > 0) ? cycles : 0;
}

void GBZ80::setInputChecker(IInputChecker* checker){
    m_InputChecker = checker;
}

//Debug
void GBZ80::showDebugPrompt(){
    bool bShowPrompt = true;
    char* input = new char[80];
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Next instruction: " << +m_gbmemory->read(PC);
    
    do{
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "\n>";
        std::cin >> input;
        switch(input[0]){
            case 'b':
                if(CONSOLE_OUTPUT_ENABLED) std::cout << "UNIMPLEMENTED\n";
                break;
            case 'c':
                if(CONSOLE_OUTPUT_ENABLED) std::cout << "UNIMPLEMENTED\n";
                break;
            case 'm':
                if(CONSOLE_OUTPUT_ENABLED) std::cout << "UNIMPLEMENTED\n";
                break;
            case 'g':
                showRegisters();
                break;
            case 's':
                if(CONSOLE_OUTPUT_ENABLED) std::cout << "UNIMPLEMENTED\n";
                m_bSingleStep = true;
                bShowPrompt = false;
                break;
            case 'r':
                m_bSingleStep = false;
                bShowPrompt = false;
                break;
            default:
                showHelp();
        }
    } while(bShowPrompt);
    
    delete input;
}

void GBZ80::showHelp(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Debugger help:\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Set breakpoint: >b hex_address\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Clear breakpoint: >c hex_address\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Show memory: >m hex_address (count)\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Show registers: >g\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Single step: s\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "run: r\n";
    
}

void GBZ80::setBreakpoint(int line){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Breakpoint set at line " << line << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Breakpoint is not yet hex!!\n";
}

void GBZ80::clearBreakpoint(int line){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Breakpoint cleared\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Breakpoint is not yet hex!\n";
}

void GBZ80::showMem(uint16_t addr){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << std::hex << addr << ": " << m_gbmemory->direct_read(addr);
}

void GBZ80::showMem(uint16_t addr, uint16_t count){
    for(uint16_t i = 0; i < count; i++){
        showMem(addr + i);
    }
}

void GBZ80::showRegisters(){
    //Display registers and flags
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Registers:\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "AF: " << AF << " A: " << +getRegisterA() << " F: " << +getRegisterF() << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "BC: " << BC << " B: " << +getRegisterB() << " C: " << +getRegisterC() << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "DE: " << DE << " D: " << +getRegisterD() << " E: " << +getRegisterE() << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "HL: " << HL << " H: " << +getRegisterH() << " L: " << +getRegisterL() << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "SP: " << SP << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "PC: " << PC << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Flags:\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z: " << getFlag_Z() << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "N: " << getFlag_N() << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "H: " << getFlag_H() << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "C: " << getFlag_C() << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Interrupts enabled: " << m_bInterruptsEnabled << ", next: " << m_bInterruptsEnabledNext << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "IE: " << +m_gbmemory->direct_read(INTERRUPT_ENABLE) << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "IF: " << +m_gbmemory->direct_read(ADDRESS_IF) << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Halt: " << m_bHalt << ", Stop: " << m_bStop << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "LCDC: " << +m_gblcd->getLCDC() << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "STAT: " << +m_gblcd->getSTAT() << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "LY:   " << +m_gblcd->getLY() << "\n";
    if(CONSOLE_OUTPUT_ENABLED) std::cout << std::endl;
}

void GBZ80::step(){
    m_bSingleStep = true;
}

//Register access funcs
uint16_t GBZ80::getRegisterAF(){
    return AF;
}

void GBZ80::setRegisterAF(uint16_t val){
    //Bottom four bits of F are always 0
    AF = val & 0xFFF0;
}

uint8_t GBZ80::getRegisterA(){
    //return (AF >> 8);
    return getMSB(AF);
}

void GBZ80::setRegisterA(uint8_t val){
    AF = setMSB(AF, val);
}

uint8_t GBZ80::getRegisterF(){
    return getLSB(AF);
}

void GBZ80::setRegisterF(uint8_t val){
    //Bottom four bits of F are always 0
    AF = setLSB(AF, val & 0xF0);
}

uint16_t GBZ80::getRegisterBC(){
    return BC;
}

void GBZ80::setRegisterBC(uint16_t val){
    BC = val;
}

uint8_t GBZ80::getRegisterB(){
    return getMSB(BC);
}

void GBZ80::setRegisterB(uint8_t val){
    BC = setMSB(BC, val);
}

uint8_t GBZ80::getRegisterC(){
    return getLSB(BC);
}

void GBZ80::setRegisterC(uint8_t val){
    BC = setLSB(BC, val);
}


uint16_t GBZ80::getRegisterDE(){
    return DE;
}

void GBZ80::setRegisterDE(uint16_t val){
    DE = val;
}

uint8_t GBZ80::getRegisterD(){
    return getMSB(DE);
}

void GBZ80::setRegisterD(uint8_t val){
    DE = setMSB(DE, val);
}

uint8_t GBZ80::getRegisterE(){
    return getLSB(DE);
}

void GBZ80::setRegisterE(uint8_t val){
    DE = setLSB(DE, val);
}


uint16_t GBZ80::getRegisterHL(){
    return HL;
}

 void GBZ80::setRegisterHL(uint16_t val){
    HL = val;
}


uint8_t GBZ80::getRegisterH(){
    return getMSB(HL);
}

void GBZ80::setRegisterH(uint8_t val){;
    HL = setMSB(HL, val);
}

uint8_t GBZ80::getRegisterL(){
    return getLSB(HL);
}

void GBZ80::setRegisterL(uint8_t val){
    HL = setLSB(HL, val);
}



uint16_t GBZ80::getRegisterSP(){
    return SP;
}

void GBZ80::setRegisterSP(uint16_t val){
    SP = val;
}


uint16_t GBZ80::getRegisterPC(){
    return PC;
}

void GBZ80::setRegisterPC(uint16_t val){
    PC = val;
}


//Flag funcs (lower 8 bits of AF register)
bool GBZ80::getFlag_Z(){
    return (getRegisterF() >> 7) & 1; 
}

void GBZ80::setFlag_Z(bool val){
    if(val){
        setRegisterF(instruction_set_generic(7, getRegisterF()));
    }else {
        setRegisterF(instruction_res_generic(7, getRegisterF()));
    }    
}

bool GBZ80::getFlag_N(){
    return (getRegisterF() >> 6) & 1;
}

void GBZ80::setFlag_N(bool val){
    if(val){
        setRegisterF(instruction_set_generic(6, getRegisterF()));
    }else {
        setRegisterF(instruction_res_generic(6, getRegisterF()));
    }
}

bool GBZ80::getFlag_H(){
    return (getRegisterF() >> 5) & 1;
}

void GBZ80::setFlag_H(bool val){
    if(val){
        setRegisterF(instruction_set_generic(5, getRegisterF()));
    }else {
        setRegisterF(instruction_res_generic(5, getRegisterF()));
    }
}

bool GBZ80::getFlag_C(){
    return (getRegisterF() >> 4) & 1;
}

void GBZ80::setFlag_C(bool val){    
    if(val){
        setRegisterF(instruction_set_generic(4, getRegisterF()));
    }else {
        setRegisterF(instruction_res_generic(4, getRegisterF()));
    }
}

//8-Bit load instructions

//ld r,n

void GBZ80::instruction_ld_B(uint8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld B," << std::hex << +n << "\n";
    setRegisterB(n);
}

void GBZ80::instruction_ld_C(uint8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld C," << std::hex << +n << "\n";
    setRegisterC(n);
}

void GBZ80::instruction_ld_D(uint8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld D," << std::hex << +n << "\n";
    setRegisterD(n);
}

void GBZ80::instruction_ld_E(uint8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld E," << std::hex << +n << "\n";
    setRegisterE(n);
}

void GBZ80::instruction_ld_H(uint8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld H," << std::hex << +n << "\n";
    setRegisterH(n);
}

void GBZ80::instruction_ld_L(uint8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld L," << std::hex << +n << "\n";
    setRegisterL(n);
}

//ld r,r
void GBZ80::instruction_ld_A_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,A" << std::hex << "\n";
    setRegisterA(getRegisterA());
}

void GBZ80::instruction_ld_A_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,B" << "\n";
    setRegisterA(getRegisterB());
}

void GBZ80::instruction_ld_A_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,C" << "\n";
    setRegisterA(getRegisterC());
}

void GBZ80::instruction_ld_A_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,D" << "\n";
    setRegisterA(getRegisterD());
}

void GBZ80::instruction_ld_A_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,E" << "\n";
    setRegisterA(getRegisterE());
}

void GBZ80::instruction_ld_A_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,H" << "\n";
    setRegisterA(getRegisterH());
}

void GBZ80::instruction_ld_A_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,L" << "\n";
    setRegisterA(getRegisterL());
}

void GBZ80::instruction_ld_A_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,(HL)" << "\n";
    setRegisterA(m_gbmemory->read(getRegisterHL()));
}

void GBZ80::instruction_ld_B_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld B,A" << "\n";
    setRegisterB(getRegisterA());
}

void GBZ80::instruction_ld_B_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld B,B" << "\n";
    setRegisterB(getRegisterB());
}

void GBZ80::instruction_ld_B_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld B,C" << "\n";
    setRegisterB(getRegisterC());
}

void GBZ80::instruction_ld_B_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld B,D" << "\n";
    setRegisterB(getRegisterD());
}

void GBZ80::instruction_ld_B_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld B,E" << "\n";
    setRegisterB(getRegisterE());
}

void GBZ80::instruction_ld_B_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld B,H" << "\n";
    setRegisterB(getRegisterH());
}

void GBZ80::instruction_ld_B_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld B,L" << "\n";
    setRegisterB(getRegisterL());
}
    
void GBZ80::instruction_ld_B_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld B,(HL)" << "\n";
    setRegisterB(m_gbmemory->read(getRegisterHL()));
}

void GBZ80::instruction_ld_C_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld C,A" << "\n";
    setRegisterC(getRegisterA());
}

void GBZ80::instruction_ld_C_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld C,B" << "\n";
    setRegisterC(getRegisterB());
}

void GBZ80::instruction_ld_C_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld C,C" << "\n";
    setRegisterC(getRegisterC());
}

void GBZ80::instruction_ld_C_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld C,D" << "\n";
    setRegisterC(getRegisterD());
}

void GBZ80::instruction_ld_C_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld C,E" << "\n";
    setRegisterC(getRegisterE());
}

void GBZ80::instruction_ld_C_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld C,H" << "\n";
    setRegisterC(getRegisterH());
}

void GBZ80::instruction_ld_C_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld C,L" << "\n";
    setRegisterC(getRegisterL());
}

void GBZ80::instruction_ld_C_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld C,(HL)" << "\n";
    setRegisterC(m_gbmemory->read(getRegisterHL()));
}

void GBZ80::instruction_ld_D_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld D,A" << "\n";
    setRegisterD(getRegisterA());
}

void GBZ80::instruction_ld_D_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld D,B" << "\n";
    setRegisterD(getRegisterB());
}

void GBZ80::instruction_ld_D_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld D,C" << "\n";
    setRegisterD(getRegisterC());
}

void GBZ80::instruction_ld_D_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld D,D" << "\n";
    setRegisterD(getRegisterD());
}

void GBZ80::instruction_ld_D_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld D,E" << "\n";
    setRegisterD(getRegisterE());
}

void GBZ80::instruction_ld_D_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld D,H" << "\n";
    setRegisterD(getRegisterH());
}

void GBZ80::instruction_ld_D_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld D,L" << "\n";
    setRegisterD(getRegisterL());
}

void GBZ80::instruction_ld_D_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld D,(HL)" << "\n";
    setRegisterD(m_gbmemory->read(getRegisterHL()));
}

void GBZ80::instruction_ld_E_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld E,A" << "\n";
    setRegisterE(getRegisterA());
}

void GBZ80::instruction_ld_E_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld E,B" << "\n";
    setRegisterE(getRegisterB());
}

void GBZ80::instruction_ld_E_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld E,C" << "\n";
    setRegisterE(getRegisterC());
}

void GBZ80::instruction_ld_E_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld E,D" << "\n";
    setRegisterE(getRegisterD());
}

void GBZ80::instruction_ld_E_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld E,E" << "\n";
    setRegisterE(getRegisterE());
}

void GBZ80::instruction_ld_E_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld E,H" << "\n";
    setRegisterE(getRegisterH());
}

void GBZ80::instruction_ld_E_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld E,L" << "\n";
    setRegisterE(getRegisterL());
}

void GBZ80::instruction_ld_E_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld E,(HL)" << "\n";
    setRegisterE(m_gbmemory->read(getRegisterHL()));
}

void GBZ80::instruction_ld_H_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld H,A" << "\n";
    setRegisterH(getRegisterA());
}

void GBZ80::instruction_ld_H_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld H,B" << "\n";
    setRegisterH(getRegisterB());
}

void GBZ80::instruction_ld_H_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld H,C" << "\n";
    setRegisterH(getRegisterC());
}

void GBZ80::instruction_ld_H_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld H,D" << "\n";
    setRegisterH(getRegisterD());
}

void GBZ80::instruction_ld_H_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld H,E" << "\n";
    setRegisterH(getRegisterE());
}

void GBZ80::instruction_ld_H_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld H,H" << "\n";
    setRegisterH(getRegisterH());
}

void GBZ80::instruction_ld_H_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld H,L" << "\n";
    setRegisterH(getRegisterL());
}

void GBZ80::instruction_ld_H_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld H,(HL)" << "\n";
    setRegisterH(m_gbmemory->read(getRegisterHL()));
}

void GBZ80::instruction_ld_L_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld L,A" << "\n";
    setRegisterL(getRegisterA());
}

void GBZ80::instruction_ld_L_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld L,B" << "\n";
    setRegisterL(getRegisterB());
}

void GBZ80::instruction_ld_L_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld L,C" << "\n";
    setRegisterL(getRegisterC());
}

void GBZ80::instruction_ld_L_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld L,D" << "\n";
    setRegisterL(getRegisterD());
}

void GBZ80::instruction_ld_L_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld L,E" << "\n";
    setRegisterL(getRegisterE());
}

void GBZ80::instruction_ld_L_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld L,H" << "\n";
    setRegisterL(getRegisterH());
}

void GBZ80::instruction_ld_L_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld L,L" << "\n";
    setRegisterL(getRegisterL());
}

void GBZ80::instruction_ld_L_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld H,(HL)" << "\n";
    setRegisterL(m_gbmemory->read(getRegisterHL()));
}

void GBZ80::instruction_ld_HLI_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (HL),A" << "\n";
    m_gbmemory->write(getRegisterHL(), getRegisterA());
}

void GBZ80::instruction_ld_HLI_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (HL),B" << "\n";
    m_gbmemory->write(getRegisterHL(), getRegisterB());
}

void GBZ80::instruction_ld_HLI_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (HL),C" << "\n";
    m_gbmemory->write(getRegisterHL(), getRegisterC());
}

void GBZ80::instruction_ld_HLI_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (HL),D" << "\n";
    m_gbmemory->write(getRegisterHL(), getRegisterD());
}

void GBZ80::instruction_ld_HLI_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (HL),E" << "\n";
    m_gbmemory->write(getRegisterHL(), getRegisterE());
}

void GBZ80::instruction_ld_HLI_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (HL),H" << "\n";
    m_gbmemory->write(getRegisterHL(), getRegisterH());
}

void GBZ80::instruction_ld_HLI_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (HL),L" << "\n";
    m_gbmemory->write(getRegisterHL(), getRegisterL());
}

void GBZ80::instruction_ld_HLI_N(uint8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (HL)," << +n << "\n";
    m_gbmemory->write(getRegisterHL(), n);
}

void GBZ80::instruction_ld_A_BCI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,(BC)" << "\n";
    setRegisterA(m_gbmemory->read(getRegisterBC()));
}

void GBZ80::instruction_ld_A_DEI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,(DE)" << "\n";
    setRegisterA(m_gbmemory->read(getRegisterDE()));
}

void GBZ80::instruction_ld_A_NNI(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,(" << nn << ")" << "\n";
    setRegisterA(m_gbmemory->read(nn));
}

void GBZ80::instruction_ld_A_CONST(uint8_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A," << val << "\n";
    setRegisterA(val);
}

void GBZ80::instruction_ld_BCI_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (BC),A" << "\n";
    m_gbmemory->write(getRegisterBC(), getRegisterA());
}

void GBZ80::instruction_ld_DEI_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (DE),A" << "\n";
    m_gbmemory->write(getRegisterDE(), getRegisterA());
}

void GBZ80::instruction_ld_NNI_A(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (" << nn << "),A" << "\n";
    m_gbmemory->write(nn, getRegisterA());
}

//LD with memory address $FF00 + register C into A
void GBZ80::instruction_ld_A_FF00CI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld A,(0xFF00+" << getRegisterC() << ")" << "\n";
    setRegisterA(m_gbmemory->read(0xFF00 + getRegisterC()));
}

//LD with A into memory address $FF00 + register C
void GBZ80::instruction_ld_FF00CI_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (0xFF00+" << getRegisterC() << "),A" << "\n";
    m_gbmemory->write(0xFF00 + getRegisterC(), getRegisterA());
}

//Put val at address in HL into A, decrement HL
void GBZ80::instruction_ldd_A_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ldd A,(HL)" << "\n";
    setRegisterA(m_gbmemory->read(getRegisterHL()));
    HL--;
}

//Put val in A into memory address of AL, decrement HL
void GBZ80::instruction_ldd_HLI_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ldd (HL),A" << "\n";
    m_gbmemory->write(getRegisterHL(), getRegisterA());
    HL--;
}

//Put val at address HL into A, increment HL
void GBZ80::instruction_ldi_A_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ldi A,(HL)" << "\n";
    setRegisterA(m_gbmemory->read(getRegisterHL()));
    HL++;
}

//Put val in A into memory address of AL, increment HL
void GBZ80::instruction_ldi_HLI_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ldi (HL),A" << "\n";
    m_gbmemory->write(getRegisterHL(), getRegisterA());
    HL++;
}


//Put A into memory address $FF00+n
void GBZ80::instruction_ldh_FF00NI_A(uint8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ldh (0xFF00+" << +n << "),A" << "\n";
    m_gbmemory->write(0xFF00 + n, getRegisterA());
}

//Put memory address $FF00+n into A
void GBZ80::instruction_ldh_A_FF00NI(uint8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ldh A,(0xFF00+" << +n << ")" << "\n";
    setRegisterA(m_gbmemory->read(0xFF00 + n));
}

//16-bit load ops

void GBZ80::instruction_ld_BC_NN(uint16_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld BC," << val << "\n";
    setRegisterBC(val);    
}

void GBZ80::instruction_ld_DE_NN(uint16_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld DE," << val << "\n";
    setRegisterDE(val);  
}

void GBZ80::instruction_ld_HL_NN(uint16_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld HL," << val << "\n";
    setRegisterHL(val);  
}

void GBZ80::instruction_ld_SP_NN(uint16_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld SP," << val << "\n";
    setRegisterSP(val);  
}

void GBZ80::instruction_ld_SP_HL(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld SP,HL" << "\n";
    setRegisterSP(getRegisterHL());
}

//Put address of SP+n into HL
void GBZ80::instruction_ldhl(int8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ldhl SP," << +n << "\n";
    uint16_t result = getRegisterSP() + n;
    setFlag_Z(false);
    setFlag_N(false);
    //H and C flags are relative to 8-bit input, not 16-bit HL.
    setFlag_H((SP & 0x0F) + (n & 0x0F) > 0x0F);
    setFlag_C((SP & 0xFF) + (n & 0xFF) > 0xFF);
    
    setRegisterHL(result);
}

//Put SP into address n
void GBZ80::instruction_ld_NNI_SP(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ld (" << nn << "),SP" << "\n";
    m_gbmemory->write(nn, getRegisterSP() & 0xFF);
    m_gbmemory->write(nn+1, (getRegisterSP() >> 8) & 0xFF);
}

//Push the given two bytes onto the stack and decrement SP accordingly
void GBZ80::instruction_push_generic(uint16_t val){
    SP--;
    m_gbmemory->write(getRegisterSP(), getMSB(val));
    SP--;
    m_gbmemory->write(getRegisterSP(), getLSB(val));
    
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "\nPush instruction performed! Value pushed: " << +val << "SP: " << +SP << std::endl;
}

//Push specified register (two bytes) onto stack.
void GBZ80::instruction_push_AF(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "push AF" << "\n";
    instruction_push_generic(getRegisterAF());
}

void GBZ80::instruction_push_BC(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "push BC" << "\n";
    instruction_push_generic(getRegisterBC());
}

void GBZ80::instruction_push_DE(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "push DE" << "\n";
    instruction_push_generic(getRegisterDE());
}

void GBZ80::instruction_push_HL(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "push HL" << "\n";
    instruction_push_generic(getRegisterHL());
}

//Pop the given two bytes from the stack. 
uint16_t GBZ80::instruction_pop_generic(){
    uint16_t val = 0;
    
    val += m_gbmemory->read(getRegisterSP());
    SP++;
    val = setMSB(val, m_gbmemory->read(getRegisterSP()));
    SP++;
    
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "\nPop instruction called! Value popped: " << val << "\nSP: " << SP << std::endl;
    return val;
}

//Pop two bytes off stack and store in specified register.
void GBZ80::instruction_pop_AF(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "pop AF" << "\n";
    setRegisterAF(instruction_pop_generic());
}

void GBZ80::instruction_pop_BC(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "pop BC" << "\n";
    setRegisterBC(instruction_pop_generic());
}

void GBZ80::instruction_pop_DE(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "pop DE" << "\n";
    setRegisterDE(instruction_pop_generic());
}

void GBZ80::instruction_pop_HL(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "pop HL" << "\n";
    setRegisterHL(instruction_pop_generic());
}

//8 Bit ALU

//Adds the value to register A
void GBZ80::instruction_add_generic(uint8_t val){
    uint8_t regA = getRegisterA();
    uint8_t result = val + regA;
    
    setFlag_Z(result == 0);
    setFlag_N(false);
    setFlag_H(((regA & 0x0F) + (val & 0x0F)) > 0x0F);
    setFlag_C((regA + val) > 0xFF);
    
    setRegisterA(result); 
}

//Add the specified register to A
void GBZ80::instruction_add_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add A,A" << "\n";
    instruction_add_generic(getRegisterA());   
}

void GBZ80::instruction_add_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add A,B" << "\n";
    instruction_add_generic(getRegisterB());
}

void GBZ80::instruction_add_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add A,C" << "\n";
    instruction_add_generic(getRegisterC());
}

void GBZ80::instruction_add_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add A,D" << "\n";
    instruction_add_generic(getRegisterD());
}

void GBZ80::instruction_add_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add A,E" << "\n";
    instruction_add_generic(getRegisterE());
}

void GBZ80::instruction_add_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add A,H" << "\n";
    instruction_add_generic(getRegisterH());
}

void GBZ80::instruction_add_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add A,L" << "\n";
    instruction_add_generic(getRegisterL());
}

void GBZ80::instruction_add_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add A,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_add_generic(val);
}

void GBZ80::instruction_add_CONST(uint8_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add A," << val << "\n";
    instruction_add_generic(val);
}

//Adds the value + carry flag to A
void GBZ80::instruction_adc_generic(uint8_t val){
    uint8_t regA = getRegisterA();
    uint8_t result = regA + val + getFlag_C();
    uint16_t cTest = regA + val + getFlag_C();
    
    setFlag_Z(result == 0);
    setFlag_H((regA & 0x0F) + (val & 0x0F) + getFlag_C() > 0x0F);
    setFlag_C(cTest > 0xFF);
    setFlag_N(false);
    
    setRegisterA(result);
}

//Add register value + carry flag to A
void GBZ80::instruction_adc_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "adc A,A" << "\n";
    instruction_adc_generic(getRegisterA());
}

void GBZ80::instruction_adc_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "adc A,B" << "\n";
    instruction_adc_generic(getRegisterB());
}

void GBZ80::instruction_adc_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "adc A,C" << "\n";
    instruction_adc_generic(getRegisterC());
}

void GBZ80::instruction_adc_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "adc A,D" << "\n";
    instruction_adc_generic(getRegisterD());
}

void GBZ80::instruction_adc_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "adc A,E" << "\n";
    instruction_adc_generic(getRegisterE());
}

void GBZ80::instruction_adc_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "adc A,H" << "\n";
    instruction_adc_generic(getRegisterH());
}

void GBZ80::instruction_adc_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "adc A,L" << "\n";
    instruction_adc_generic(getRegisterL());
}

void GBZ80::instruction_adc_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "adc A,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_adc_generic(val);
}

void GBZ80::instruction_adc_CONST(uint8_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "adc A," << val << "\n";
    instruction_adc_generic(val);
}

//Sub the specified register from A
void GBZ80::instruction_sub_generic(uint8_t val){
    uint8_t regA = getRegisterA();
    uint8_t result = regA - val;
    
    setFlag_Z(result == 0);
    setFlag_N(true);
    setFlag_H((val & 0x0F) > (regA & 0x0F));
    setFlag_C(val > regA);
    
    setRegisterA(result);
}

void GBZ80::instruction_sub_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sub A" << "\n";
    instruction_sub_generic(getRegisterA());
}

void GBZ80::instruction_sub_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sub B" << "\n";
    instruction_sub_generic(getRegisterB());
}

void GBZ80::instruction_sub_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sub C" << "\n";
    instruction_sub_generic(getRegisterC());
}

void GBZ80::instruction_sub_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sub D" << "\n";
    instruction_sub_generic(getRegisterD());
}

void GBZ80::instruction_sub_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sub E" << "\n";
    instruction_sub_generic(getRegisterE());
}

void GBZ80::instruction_sub_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sub H" << "\n";
    instruction_sub_generic(getRegisterH());
}

void GBZ80::instruction_sub_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sub L" << "\n";
    instruction_sub_generic(getRegisterL());
}

void GBZ80::instruction_sub_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sub (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_sub_generic(val);
}

void GBZ80::instruction_sub_CONST(uint8_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sub " << val << "\n";
    instruction_sub_generic(val);
}

//Sub register + carry flag from A
void GBZ80::instruction_sbc_generic(uint8_t val){
    uint8_t regA = getRegisterA();
    uint8_t result = regA - val - getFlag_C();
    uint16_t cTest = regA - val - getFlag_C();
    
    setFlag_Z(result == 0);
    setFlag_H((regA & 0x0F) < (val & 0x0F) + getFlag_C());
    setFlag_C(cTest > 0xFF);
    setFlag_N(true);
    
    setRegisterA(result);
}

void GBZ80::instruction_sbc_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sbc A" << "\n";
    instruction_sbc_generic(getRegisterA());
}

void GBZ80::instruction_sbc_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sbc B" << "\n";
    instruction_sbc_generic(getRegisterB());
}

void GBZ80::instruction_sbc_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sbc C" << "\n";
    instruction_sbc_generic(getRegisterC());
}

void GBZ80::instruction_sbc_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sbc D" << "\n";
    instruction_sbc_generic(getRegisterD());
}

void GBZ80::instruction_sbc_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sbc E" << "\n";
    instruction_sbc_generic(getRegisterE());
}

void GBZ80::instruction_sbc_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sbc H" << "\n";
    instruction_sbc_generic(getRegisterH());
}

void GBZ80::instruction_sbc_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sbc L" << "\n";
    instruction_sbc_generic(getRegisterL());
}

void GBZ80::instruction_sbc_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sbc (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_sbc_generic(val);
}

void GBZ80::instruction_sbc_CONST(uint8_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sbc " << val << "\n";
    instruction_sbc_generic(val);
}

uint8_t GBZ80::instruction_and_generic(uint8_t r1, uint8_t r2){
    setFlag_Z((r1 & r2) == 0);
    setFlag_N(false);
    setFlag_H(true);
    setFlag_C(false);
    
    return (r1 & r2);
}

//Logically AND register with A, store in A
void GBZ80::instruction_and_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "and A,A" << "\n";
    setRegisterA(instruction_and_generic(getRegisterA(), getRegisterA()));
}

void GBZ80::instruction_and_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "and A,B" << "\n";
    setRegisterA(instruction_and_generic(getRegisterA(), getRegisterB()));
}

void GBZ80::instruction_and_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "and A,C" << "\n";
    setRegisterA(instruction_and_generic(getRegisterA(), getRegisterC()));
}

void GBZ80::instruction_and_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "and A,D" << "\n";
    setRegisterA(instruction_and_generic(getRegisterA(), getRegisterD()));
}

void GBZ80::instruction_and_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "and A,E" << "\n";
    setRegisterA(instruction_and_generic(getRegisterA(), getRegisterE()));
}

void GBZ80::instruction_and_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "and A,H" << "\n";
    setRegisterA(instruction_and_generic(getRegisterA(), getRegisterH()));
}

void GBZ80::instruction_and_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "and A,L" << "\n";
    setRegisterA(instruction_and_generic(getRegisterA(), getRegisterL()));
}

void GBZ80::instruction_and_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "and A,(HL)" << "\n";
    setRegisterA(instruction_and_generic(getRegisterA(), m_gbmemory->read(getRegisterHL())));
}

void GBZ80::instruction_and_CONST(uint8_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "and A," << val << "\n";
    setRegisterA(instruction_and_generic(getRegisterA(), val));
}

//Logically OR registers, store in register A
uint8_t GBZ80::instruction_or_generic(uint8_t r1, uint8_t r2){
    setFlag_Z((r1 | r2) == 0);
    setFlag_N(false);
    setFlag_H(false);
    setFlag_C(false);
    
    return (r1 | r2);
}

void GBZ80::instruction_or_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "or A,A" << "\n";
    setRegisterA(instruction_or_generic(getRegisterA(), getRegisterA()));
}

void GBZ80::instruction_or_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "or A,B" << "\n";
    setRegisterA(instruction_or_generic(getRegisterA(), getRegisterB()));
}

void GBZ80::instruction_or_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "or A,C" << "\n";
    setRegisterA(instruction_or_generic(getRegisterA(), getRegisterC()));
}

void GBZ80::instruction_or_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "or A,D" << "\n";
    setRegisterA(instruction_or_generic(getRegisterA(), getRegisterD()));
}

void GBZ80::instruction_or_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "or A,E" << "\n";
    setRegisterA(instruction_or_generic(getRegisterA(), getRegisterE()));
}

void GBZ80::instruction_or_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "or A,H" << "\n";
    setRegisterA(instruction_or_generic(getRegisterA(), getRegisterH()));
}

void GBZ80::instruction_or_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "or A,L" << "\n";
    setRegisterA(instruction_or_generic(getRegisterA(), getRegisterL()));
}

void GBZ80::instruction_or_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "or A,(HL)" << "\n";
    setRegisterA(instruction_or_generic(getRegisterA(), m_gbmemory->read(getRegisterHL())));
}

void GBZ80::instruction_or_CONST(uint8_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "or A," << val << "\n";
    setRegisterA(instruction_or_generic(getRegisterA(), val));
}

//Logically XOR registers, store in A
uint8_t GBZ80::instruction_xor_generic(uint8_t r1, uint8_t r2){
    setFlag_Z((r1 ^ r2) == 0);
    setFlag_N(false);
    setFlag_H(false);
    setFlag_C(false);
    
    return (r1 ^ r2);
}

void GBZ80::instruction_xor_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "xor A,A" << "\n";
    setRegisterA(instruction_xor_generic(getRegisterA(), getRegisterA()));
}

void GBZ80::instruction_xor_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "xor A,B" << "\n";
    setRegisterA(instruction_xor_generic(getRegisterA(), getRegisterB()));
}

void GBZ80::instruction_xor_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "xor A,C" << "\n";
    setRegisterA(instruction_xor_generic(getRegisterA(), getRegisterC()));
}

void GBZ80::instruction_xor_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "xor A,D" << "\n";
    setRegisterA(instruction_xor_generic(getRegisterA(), getRegisterD()));
}

void GBZ80::instruction_xor_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "xor A,E" << "\n";
    setRegisterA(instruction_xor_generic(getRegisterA(), getRegisterE()));
}

void GBZ80::instruction_xor_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "xor A,H" << "\n";
    setRegisterA(instruction_xor_generic(getRegisterA(), getRegisterH()));
}

void GBZ80::instruction_xor_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "xor A,L" << "\n";
    setRegisterA(instruction_xor_generic(getRegisterA(), getRegisterL()));
}

void GBZ80::instruction_xor_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "xor A,(HL)" << "\n";
    setRegisterA(instruction_xor_generic(getRegisterA(), m_gbmemory->read(getRegisterHL())));
}

void GBZ80::instruction_xor_CONST(uint8_t val){ 
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "xor A," << val << "\n";
    setRegisterA(instruction_xor_generic(getRegisterA(),val));
}

//Compare A with register. Basically A - register without storing result
void GBZ80::instruction_cp_generic(uint8_t val){
    uint8_t regA = getRegisterA();
    uint8_t result = regA - val;
    
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "Comparison: " << +regA << " - " << +val << std::endl;
    
    setFlag_Z(regA == val);
    setFlag_N(true);
    setFlag_H((val & 0x0f) > (regA & 0x0f));
    setFlag_C(regA < val);
}

void GBZ80::instruction_cp_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "cp A" << "\n";
    instruction_cp_generic(getRegisterA());
}

void GBZ80::instruction_cp_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "cp B" << "\n";
    instruction_cp_generic(getRegisterB());
}

void GBZ80::instruction_cp_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "cp C" << "\n";
    instruction_cp_generic(getRegisterC());
}

void GBZ80::instruction_cp_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "cp D" << "\n";
    instruction_cp_generic(getRegisterD());
}

void GBZ80::instruction_cp_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "cp E" << "\n";
    instruction_cp_generic(getRegisterE());
}

void GBZ80::instruction_cp_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "cp H" << "\n";
    instruction_cp_generic(getRegisterH());
}

void GBZ80::instruction_cp_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "cp L" << "\n";
    instruction_cp_generic(getRegisterL());
}

void GBZ80::instruction_cp_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "cp (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_cp_generic(val);
}

void GBZ80::instruction_cp_CONST(uint8_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "cp " << +val << "\n";
    instruction_cp_generic(val);
}

//Increment register
uint8_t GBZ80::instruction_inc_generic(uint8_t val){
    uint8_t result = val + 1;
    
    setFlag_Z(result == 0);
    setFlag_N(false);
    setFlag_H((((val & 0x0F) + 1)) > 0x0F);
    
    return result;
}

void GBZ80::instruction_inc_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc A" << "\n";
    setRegisterA(instruction_inc_generic(getRegisterA()));
}

void GBZ80::instruction_inc_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc B" << "\n";
    setRegisterB(instruction_inc_generic(getRegisterB()));
}

void GBZ80::instruction_inc_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc C" << "\n";
    setRegisterC(instruction_inc_generic(getRegisterC()));
}

void GBZ80::instruction_inc_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc D" << "\n";
    setRegisterD(instruction_inc_generic(getRegisterD()));
}

void GBZ80::instruction_inc_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc E" << "\n";
    setRegisterE(instruction_inc_generic(getRegisterE()));
}

void GBZ80::instruction_inc_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc H" << "\n";
    setRegisterH(instruction_inc_generic(getRegisterH()));
}

void GBZ80::instruction_inc_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc L" << "\n";
    setRegisterL(instruction_inc_generic(getRegisterL()));
}

void GBZ80::instruction_inc_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_inc_generic(val);
    m_gbmemory->write(getRegisterHL(), val);
}

//Decrement register
uint8_t GBZ80::instruction_dec_generic(uint8_t val){
    uint8_t result = val - 1;
    uint8_t hFlagTest = val & 0x0F;
    hFlagTest -= 1;
    
    setFlag_Z(result == 0);
    setFlag_N(true);
    setFlag_H(hFlagTest > 0x0F);
    
    return result;
}

void GBZ80::instruction_dec_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec A" << "\n";
    setRegisterA(instruction_dec_generic(getRegisterA()));
}

void GBZ80::instruction_dec_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec B" << "\n";
    setRegisterB(instruction_dec_generic(getRegisterB()));
}

void GBZ80::instruction_dec_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec C" << "\n";
    setRegisterC(instruction_dec_generic(getRegisterC()));
}

void GBZ80::instruction_dec_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec D" << "\n";
    setRegisterD(instruction_dec_generic(getRegisterD()));
}

void GBZ80::instruction_dec_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec E" << "\n";
    setRegisterE(instruction_dec_generic(getRegisterE()));
}

void GBZ80::instruction_dec_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec H" << "\n";
    setRegisterH(instruction_dec_generic(getRegisterH()));
}

void GBZ80::instruction_dec_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec L" << "\n";
    setRegisterL(instruction_dec_generic(getRegisterL()));
}

void GBZ80::instruction_dec_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_dec_generic(val);
    m_gbmemory->write(getRegisterHL(), val);
}

//16 Bit arithmetic

//Add register to HL directly
void GBZ80::instruction_add_generic(uint16_t val){
    uint16_t hFlag = (HL & 0x0FFF) + (val & 0x0FFF) ;
    uint16_t cFlag = HL + val;
    
    setFlag_N(false);
    setFlag_H(hFlag & 0x1000);
    setFlag_C(cFlag < HL);
    
    HL += val;
}

void GBZ80::instruction_add_BC(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add BC" << "\n";
    instruction_add_generic(getRegisterBC());
}

void GBZ80::instruction_add_DE(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add DE" << "\n";
    instruction_add_generic(getRegisterDE());
}

void GBZ80::instruction_add_HL(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add HL" << "\n";
    instruction_add_generic(getRegisterHL());
}

void GBZ80::instruction_add_SP(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add SP" << "\n";
    instruction_add_generic(getRegisterSP());
}


//Add value to SP
void GBZ80::instruction_add_SP_CONST(int8_t val){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "add SP," << val << "\n";
    uint16_t cTest = SP + val;
    
    setFlag_Z(false);
    setFlag_N(false);
    //H and C flags are relative to 8-bit input, not 16-bit SP.
    setFlag_H((SP & 0x0F) + (val & 0x0F) > 0x0F);
    setFlag_C((SP & 0xFF) + (val & 0xFF) > 0xFF);
    
    SP += val;
}


//Increment register
void GBZ80::instruction_inc_BC(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc BC" << "\n";
    BC++;
}

void GBZ80::instruction_inc_DE(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc DE" << "\n";
    DE++;
}

void GBZ80::instruction_inc_HL(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc HL" << "\n";
    HL++;
}

void GBZ80::instruction_inc_SP(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "inc SP" << "\n";
    SP++;
}


//Decrement register
void GBZ80::instruction_dec_BC(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec BC" << "\n";
    BC--;
}

void GBZ80::instruction_dec_DE(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec DE" << "\n";
    DE--;
}

void GBZ80::instruction_dec_HL(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec HL" << "\n";
    HL--;
}

void GBZ80::instruction_dec_SP(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "dec SP" << "\n";
    SP--;
}

//Swap upper and lower nibbles of register
uint8_t GBZ80::instruction_swap_generic(uint8_t val){
    uint8_t result = ((val << 4) & 0xF0) | ((val >> 4) & 0x0F);
    
    setFlag_Z(result == 0);
    setFlag_N(false);
    setFlag_H(false);
    setFlag_C(false);
    
    return result;
}

void GBZ80::instruction_swap_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "swap A" << "\n";
    setRegisterA(instruction_swap_generic(getRegisterA()));
}
    
void GBZ80::instruction_swap_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "swap B" << "\n";
    setRegisterB(instruction_swap_generic(getRegisterB()));
}

void GBZ80::instruction_swap_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "swap C" << "\n";
    setRegisterC(instruction_swap_generic(getRegisterC()));
}

void GBZ80::instruction_swap_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "swap D" << "\n";
    setRegisterD(instruction_swap_generic(getRegisterD()));
}

void GBZ80::instruction_swap_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "swap E" << "\n";
    setRegisterE(instruction_swap_generic(getRegisterE()));
}

void GBZ80::instruction_swap_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "swap H" << "\n";
    setRegisterH(instruction_swap_generic(getRegisterH()));
}

void GBZ80::instruction_swap_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "swap L" << "\n";
    setRegisterL(instruction_swap_generic(getRegisterL()));
}

void GBZ80::instruction_swap_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "swap (HL)" << "\n";
    uint8_t toSwap = m_gbmemory->read(getRegisterHL());
    m_gbmemory->write(getRegisterHL(), instruction_swap_generic(toSwap));
}

//Adjusts register A to contain correct representation of Binary Coded Decimal
void GBZ80::instruction_daa(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "daa" << "\n";
    
    //Logic from /u/mudanhonnyaku's post on Reddit in /r/emudev
    uint8_t regA = getRegisterA();
    uint8_t result = regA;
    bool flagC = getFlag_C();
    if(flagC || (!getFlag_N() && (regA > 0x99))){
        if(getFlag_N()){
            result -= 0x60;
        } else {
            result += 0x60;
        }
        
        setFlag_C(true);
    }
    
    uint8_t flagH = getFlag_H();
    if(flagH || (!getFlag_N() && ((regA & 0x0F) > 9))){
        if(getFlag_N()){
            result -= 6;
        } else {
            result += 6;
        }
    }
    
    setFlag_H(false);
    setFlag_Z(result == 0);
    setRegisterA(result);
}

//Compliments register A (flips all bits)
void GBZ80::instruction_cpl(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "cpl" << "\n";
    setFlag_N(true);
    setFlag_H(true);
    setRegisterA(~getRegisterA());
}
  
//Compliments/Flips C(arry) flag
void GBZ80::instruction_ccf(){
    setFlag_N(false);
    setFlag_H(false);
    setFlag_C(!getFlag_C());
}
  
//Sets the Cary flag to true.
void GBZ80::instruction_scf(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "scf" << "\n";
    setFlag_N(false);
    setFlag_H(false);
    setFlag_C(true);
}

//No op
void GBZ80::instruction_nop(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "nop" << "\n";
}
  
//Powers down CPU until an interupt occures
void GBZ80::instruction_halt(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "halt" << "\n";
    m_bHalt = true;
}
  
//Halt CPU and LCD
void GBZ80::instruction_stop(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "stop" << "\n";
    m_bStop = true;
}
  
//Disable interrupts after next instruction
void GBZ80::instruction_di(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "di" << "\n";
    m_bInterruptsEnabledNext = false;
    m_bInterruptsEnabled = false;
}

//Enable interrupts after next instruction
void GBZ80::instruction_ei(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ei" << "\n";
    //TODO - bring back enabled next in tick and set to false here once CPU is more accurate elsewhere
    m_bInterruptsEnabledNext = true;
    //m_bInterruptsEnabled = true;
}

//Rotates and shifts

//Generic rotate left. Old bit 7 is stored in Carry flag AND as bit 0
uint8_t GBZ80::instruction_rlc_generic(uint8_t val){
    uint8_t bit7 = ((val & 128) >> 7) & 0x01;
    setFlag_C(bit7 ? true : false);
    setFlag_N(false);
    setFlag_H(false);
    val = ((val << 1) & 0xFE)| bit7;
    setFlag_Z(val == 0);
    
    return val;
}

//Generic rotate left through carry flag. Old bit 7 is stored in Carry flag. Bit 0 set to old Carry flag value
uint8_t GBZ80::instruction_rl_generic(uint8_t val){
    uint8_t bit7 = (val >> 7) & 0x01;//(val & 128) >> 7;
    uint8_t oldCarry = getFlag_C();
    setFlag_C(bit7 ? true : false);
    setFlag_N(false);
    setFlag_H(false);
    val = ((val << 1) & 0xFE) | oldCarry;
    setFlag_Z(val == 0);
    return val;
}

//Generic rotate right. Old bit 0 is stored in Carry flag AND as bit 7
uint8_t GBZ80::instruction_rrc_generic(uint8_t val){
    uint8_t bit0 = val & 1;
    setFlag_C(bit0 ? true : false);
    setFlag_N(false);
    setFlag_H(false);
    val = ((val >> 1) & 0x7F) | ((bit0 << 7) & 0x80);
    setFlag_Z(val == 0);
    
    return val;
}

//Generic rotate right through carry flag. Old bit 0 is stored in Carry. Bit 7 set to old Carry value
uint8_t GBZ80::instruction_rr_generic(uint8_t val){
    uint8_t bit0 = val & 1;
    uint8_t oldCarry = getFlag_C();
    setFlag_C(bit0 ? true : false);
    setFlag_N(false);
    setFlag_H(false);
    val = ((val >> 1) & 0x7F) | ((oldCarry << 7) & 0x80);
    setFlag_Z(val == 0);
    
    return val;
}

//Rotate A left. Old bit 7 is stored in Carry flag AND as bit 0
void GBZ80::instruction_rlca(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rlca" << "\n";
    
    uint8_t regA = getRegisterA();
    setFlag_C((regA & 0x80) > 0);
    uint8_t result = ((regA << 1) & 0xFE) | getFlag_C();
    setFlag_N(false);
    setFlag_H(false);
    setFlag_Z(false);
    setRegisterA(result);
}
  
//Rotate A left through carry flag. Old bit 7 is stored in Carry flag. Bit 0 set to old Carry flag value
void GBZ80::instruction_rla(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rla" << "\n";
    
    uint8_t regA = getRegisterA();
    uint8_t result = ((regA << 1) & 0xFE) | getFlag_C();
    setFlag_C(regA & 0x80);
    setFlag_H(false);
    setFlag_N(false);
    setFlag_Z(false);
    setRegisterA(result);
}
  
//Rotate A right. Old bit 0 is stored in Carry flag AND as bit 7
void GBZ80::instruction_rrca(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rrca" << "\n";
    
    uint8_t regA = getRegisterA();
    setFlag_C((regA & 0x01) > 0);
    uint8_t result = ((regA >> 1) & 0x7F) | ((getFlag_C() << 7) & 0x80);
    setFlag_N(false);
    setFlag_H(false);
    setFlag_Z(false);
    setRegisterA(result);
}
  
//Rotate A right through carry flag. Old bit 0 is stored in Carry. Bit 7 set to old Carry value
void GBZ80::instruction_rra(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rra" << "\n";
    
    uint8_t regA = getRegisterA();
    uint8_t result = ((regA >> 1) & 0x7F) | ((getFlag_C() << 7) & 0x80);
    setFlag_C((regA & 0x01) > 0);
    setFlag_N(false);
    setFlag_H(false);
    setFlag_Z(false);
    setRegisterA(result);
    
}

//Rotate register left. Old bit 7 stored in Carry flag
void GBZ80::instruction_rlc_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rlc A" << "\n";
    setRegisterA(instruction_rlc_generic(getRegisterA()));
}

void GBZ80::instruction_rlc_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rlc B" << "\n";
    setRegisterB(instruction_rlc_generic(getRegisterB()));
}

void GBZ80::instruction_rlc_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rlc C" << "\n";
    setRegisterC(instruction_rlc_generic(getRegisterC()));
}

void GBZ80::instruction_rlc_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rlc D" << "\n";
    setRegisterD(instruction_rlc_generic(getRegisterD()));
}

void GBZ80::instruction_rlc_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rlc E" << "\n";
    setRegisterE(instruction_rlc_generic(getRegisterE()));
}

void GBZ80::instruction_rlc_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rlc H" << "\n";
    setRegisterH(instruction_rlc_generic(getRegisterH()));
}

void GBZ80::instruction_rlc_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rlc L" << "\n";
    setRegisterL(instruction_rlc_generic(getRegisterL()));
}

void GBZ80::instruction_rlc_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rlc (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_rlc_generic(val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_rl_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rl A" << "\n";
    setRegisterA(instruction_rl_generic(getRegisterA()));
}

void GBZ80::instruction_rl_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rl B" << "\n";
    setRegisterB(instruction_rl_generic(getRegisterB()));
}

void GBZ80::instruction_rl_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rl C" << "\n";
    setRegisterC(instruction_rl_generic(getRegisterC()));
}

void GBZ80::instruction_rl_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rl D" << "\n";
    setRegisterD(instruction_rl_generic(getRegisterD()));
}

void GBZ80::instruction_rl_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rl E" << "\n";
    setRegisterE(instruction_rl_generic(getRegisterE()));
}

void GBZ80::instruction_rl_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rl H" << "\n";
    setRegisterH(instruction_rl_generic(getRegisterH()));
}

void GBZ80::instruction_rl_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rl L" << "\n";
    setRegisterL(instruction_rl_generic(getRegisterL()));
}

void GBZ80::instruction_rl_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rl (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_rl_generic(val);
    m_gbmemory->write(getRegisterHL(), val);
}

//Rotate register right. Old bit 0 stored in Carry flag
void GBZ80::instruction_rrc_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rrc A" << "\n";
    setRegisterA(instruction_rrc_generic(getRegisterA()));
}

void GBZ80::instruction_rrc_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rrc B" << "\n";
    setRegisterB(instruction_rrc_generic(getRegisterB()));
}

void GBZ80::instruction_rrc_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rrc C" << "\n";
    setRegisterC(instruction_rrc_generic(getRegisterC()));
}

void GBZ80::instruction_rrc_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rrc D" << "\n";
    setRegisterD(instruction_rrc_generic(getRegisterD()));
}

void GBZ80::instruction_rrc_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rrc E" << "\n";
    setRegisterE(instruction_rrc_generic(getRegisterE()));
}

void GBZ80::instruction_rrc_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rrc H" << "\n";
    setRegisterH(instruction_rrc_generic(getRegisterH()));
}

void GBZ80::instruction_rrc_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rrc L" << "\n";
    setRegisterL(instruction_rrc_generic(getRegisterL()));
}

void GBZ80::instruction_rrc_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rrc (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_rrc_generic(val);
    m_gbmemory->write(getRegisterHL(), val);
}

//Rotate register right through Carry flag (How is this different form rrc?)
void GBZ80::instruction_rr_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rr A" << "\n";
    setRegisterA(instruction_rr_generic(getRegisterA()));
}

void GBZ80::instruction_rr_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rr B" << "\n";
    setRegisterB(instruction_rr_generic(getRegisterB()));
}

void GBZ80::instruction_rr_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rr C" << "\n";
    setRegisterC(instruction_rr_generic(getRegisterC()));
}

void GBZ80::instruction_rr_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rr D" << "\n";
    setRegisterD(instruction_rr_generic(getRegisterD()));
}

void GBZ80::instruction_rr_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rr E" << "\n";
    setRegisterE(instruction_rr_generic(getRegisterE()));
}

void GBZ80::instruction_rr_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rr H" << "\n";
    setRegisterH(instruction_rr_generic(getRegisterH()));
}

void GBZ80::instruction_rr_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rr L" << "\n";
    setRegisterL(instruction_rr_generic(getRegisterL()));
}

void GBZ80::instruction_rr_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rr (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_rr_generic(val);
    m_gbmemory->write(getRegisterHL(), val);
}


//Shift register left into Carry. LSB of register set to 0
uint8_t GBZ80::instruction_sla_generic(uint8_t val){
    setFlag_N(false);
    setFlag_H(false);
    setFlag_C(((val & 128) >> 7) & 0xFF);
    val = (val << 1) & 0xFE;
    setFlag_Z(val == 0);
    return val;
}

void GBZ80::instruction_sla_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sla A" << "\n";
    setRegisterA(instruction_sla_generic(getRegisterA()));
}

void GBZ80::instruction_sla_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sla B" << "\n";
    setRegisterB(instruction_sla_generic(getRegisterB()));
}

void GBZ80::instruction_sla_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sla C" << "\n";
    setRegisterC(instruction_sla_generic(getRegisterC()));
}

void GBZ80::instruction_sla_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sla D" << "\n";
    setRegisterD(instruction_sla_generic(getRegisterD()));
}

void GBZ80::instruction_sla_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sla E" << "\n";
    setRegisterE(instruction_sla_generic(getRegisterE()));
}

void GBZ80::instruction_sla_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sla H" << "\n";
    setRegisterH(instruction_sla_generic(getRegisterH()));
}

void GBZ80::instruction_sla_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sla L" << "\n";
    setRegisterL(instruction_sla_generic(getRegisterL()));
}

void GBZ80::instruction_sla_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sla (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_sla_generic(val);
    m_gbmemory->write(getRegisterHL(), val);
}

//Shift register right into carry. MSB doesn't change
uint8_t GBZ80::instruction_sra_generic(uint8_t val){
    setFlag_N(false);
    setFlag_H(false);
    setFlag_C((val & 1) ? true : false);
    uint8_t msb = val & 0x80;
    val = ((val >> 1) & 0x7F) | msb;
    setFlag_Z(val == 0);
    
    return val;
}

void GBZ80::instruction_sra_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sra A" << "\n";
    setRegisterA(instruction_sra_generic(getRegisterA()));
}

void GBZ80::instruction_sra_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sra B" << "\n";
    setRegisterB(instruction_sra_generic(getRegisterB()));
}

void GBZ80::instruction_sra_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sra C" << "\n";
    setRegisterC(instruction_sra_generic(getRegisterC()));
}

void GBZ80::instruction_sra_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sra D" << "\n";
    setRegisterD(instruction_sra_generic(getRegisterD()));
}

void GBZ80::instruction_sra_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sra E" << "\n";
    setRegisterE(instruction_sra_generic(getRegisterE()));
}

void GBZ80::instruction_sra_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sra H" << "\n";
    setRegisterH(instruction_sra_generic(getRegisterH()));
}

void GBZ80::instruction_sra_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sra L" << "\n";
    setRegisterL(instruction_sra_generic(getRegisterL()));
}

void GBZ80::instruction_sra_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "sra (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_sra_generic(val);
    m_gbmemory->write(getRegisterHL(), val);
}

  
//Shift register left into Carry. MSB of register set to 0
uint8_t GBZ80::instruction_srl_generic(uint8_t val){
    setFlag_N(false);
    setFlag_H(false);
    setFlag_C((val & 1) ? true : false);
    val = (val >> 1) & 0x7F;
    setFlag_Z(val == 0);
    
    return val;
}

void GBZ80::instruction_srl_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "srl A" << "\n";
    setRegisterA(instruction_srl_generic(getRegisterA()));
}

void GBZ80::instruction_srl_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "srl B" << "\n";
    setRegisterB(instruction_srl_generic(getRegisterB()));
}

void GBZ80::instruction_srl_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "srl C" << "\n";
    setRegisterC(instruction_srl_generic(getRegisterC()));
}

void GBZ80::instruction_srl_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "srl D" << "\n";
    setRegisterD(instruction_srl_generic(getRegisterD()));
}

void GBZ80::instruction_srl_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "srl E" << "\n";
    setRegisterE(instruction_srl_generic(getRegisterE()));
}

void GBZ80::instruction_srl_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "srl H" << "\n";
    setRegisterH(instruction_srl_generic(getRegisterH()));
}

void GBZ80::instruction_srl_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "srl L" << "\n";
    setRegisterL(instruction_srl_generic(getRegisterL()));
}

void GBZ80::instruction_srl_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "srl (HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_srl_generic(val);
    m_gbmemory->write(getRegisterHL(), val);
}


//Bit instructions

//Test the given bit of register
bool GBZ80::instruction_bit_generic(uint8_t bit, uint8_t val){
    uint8_t set = (val >> bit) & 1;
    
    setFlag_N(false);
    setFlag_H(true);
    setFlag_Z(set == 0);
    
    return set;
}

void GBZ80::instruction_bit_0_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "0,A" << "\n";
    instruction_bit_generic(0, getRegisterA());
}

void GBZ80::instruction_bit_0_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "0,B" << "\n";
    instruction_bit_generic(0, getRegisterB());
}

void GBZ80::instruction_bit_0_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "0,C" << "\n";
    instruction_bit_generic(0, getRegisterC());
}

void GBZ80::instruction_bit_0_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "0,D" << "\n";
    instruction_bit_generic(0, getRegisterD());
}

void GBZ80::instruction_bit_0_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "0,D" << "\n";
    instruction_bit_generic(0, getRegisterE());
}

void GBZ80::instruction_bit_0_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "0,H" << "\n";
    instruction_bit_generic(0, getRegisterH());
}

void GBZ80::instruction_bit_0_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "0,L" << "\n";
    instruction_bit_generic(0, getRegisterL());
}

void GBZ80::instruction_bit_0_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "0,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_bit_generic(0, val);
}

void GBZ80::instruction_bit_1_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "1,A" << "\n";
    instruction_bit_generic(1, getRegisterA());
}

void GBZ80::instruction_bit_1_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "1,B" << "\n";
    instruction_bit_generic(1, getRegisterB());
}

void GBZ80::instruction_bit_1_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "1,C" << "\n";
    instruction_bit_generic(1, getRegisterC());
}

void GBZ80::instruction_bit_1_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "1,D" << "\n";
    instruction_bit_generic(1, getRegisterD());
}

void GBZ80::instruction_bit_1_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "1,E" << "\n";
    instruction_bit_generic(1, getRegisterE());
}

void GBZ80::instruction_bit_1_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "1,H" << "\n";
    instruction_bit_generic(1, getRegisterH());
}

void GBZ80::instruction_bit_1_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "1,L" << "\n";
    instruction_bit_generic(1, getRegisterL());
}

void GBZ80::instruction_bit_1_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "1,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_bit_generic(1, val);
}

void GBZ80::instruction_bit_2_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "2,A" << "\n";
    instruction_bit_generic(2, getRegisterA());
}

void GBZ80::instruction_bit_2_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "2,B" << "\n";
    instruction_bit_generic(2, getRegisterB());
}

void GBZ80::instruction_bit_2_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "2,C" << "\n";
    instruction_bit_generic(2, getRegisterC());
}

void GBZ80::instruction_bit_2_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "2,D" << "\n";
    instruction_bit_generic(2, getRegisterD());
}

void GBZ80::instruction_bit_2_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "2,E" << "\n";
    instruction_bit_generic(2, getRegisterE());
}

void GBZ80::instruction_bit_2_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "2,H" << "\n";
    instruction_bit_generic(2, getRegisterH());
}

void GBZ80::instruction_bit_2_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "2,L" << "\n";
    instruction_bit_generic(2, getRegisterL());
}

void GBZ80::instruction_bit_2_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "2,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_bit_generic(2, val);
}

void GBZ80::instruction_bit_3_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "3,B" << "\n";
    instruction_bit_generic(3, getRegisterA());
}

void GBZ80::instruction_bit_3_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "3,B" << "\n";
    instruction_bit_generic(3, getRegisterB());
}

void GBZ80::instruction_bit_3_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "3,C" << "\n";
    instruction_bit_generic(3, getRegisterC());
}

void GBZ80::instruction_bit_3_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "3,D" << "\n";
    instruction_bit_generic(3, getRegisterD());
}

void GBZ80::instruction_bit_3_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "3,E" << "\n";
    instruction_bit_generic(3, getRegisterE());
}

void GBZ80::instruction_bit_3_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "3,H" << "\n";
    instruction_bit_generic(3, getRegisterH());
}

void GBZ80::instruction_bit_3_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "3,L" << "\n";
    instruction_bit_generic(3, getRegisterL());
}

void GBZ80::instruction_bit_3_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "3,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_bit_generic(3, val);
}

void GBZ80::instruction_bit_4_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "4,A" << "\n";
    instruction_bit_generic(4, getRegisterA());
}

void GBZ80::instruction_bit_4_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "4,B" << "\n";
    instruction_bit_generic(4, getRegisterB());
}

void GBZ80::instruction_bit_4_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "4,C" << "\n";
    instruction_bit_generic(4, getRegisterC());
}

void GBZ80::instruction_bit_4_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "4,D" << "\n";
    instruction_bit_generic(4, getRegisterD());
}

void GBZ80::instruction_bit_4_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "4,E" << "\n";
    instruction_bit_generic(4, getRegisterE());
}

void GBZ80::instruction_bit_4_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "4,H" << "\n";
    instruction_bit_generic(4, getRegisterH());
}

void GBZ80::instruction_bit_4_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "4,L" << "\n";
    instruction_bit_generic(4, getRegisterL());
}

void GBZ80::instruction_bit_4_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "4,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_bit_generic(4, val);
}

void GBZ80::instruction_bit_5_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "5,A" << "\n";
    instruction_bit_generic(5, getRegisterA());
}

void GBZ80::instruction_bit_5_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "5,B" << "\n";
    instruction_bit_generic(5, getRegisterB());
}

void GBZ80::instruction_bit_5_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "5,C" << "\n";
    instruction_bit_generic(5, getRegisterC());
}

void GBZ80::instruction_bit_5_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "5,D" << "\n";
    instruction_bit_generic(5, getRegisterD());
}

void GBZ80::instruction_bit_5_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "5,E" << "\n";
    instruction_bit_generic(5, getRegisterE());
}

void GBZ80::instruction_bit_5_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "5,H" << "\n";
    instruction_bit_generic(5, getRegisterH());
}

void GBZ80::instruction_bit_5_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "5,L" << "\n";
    instruction_bit_generic(5, getRegisterL());
}

void GBZ80::instruction_bit_5_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "5,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_bit_generic(5, val);
}

void GBZ80::instruction_bit_6_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "6,A" << "\n";
    instruction_bit_generic(6, getRegisterA());
}

void GBZ80::instruction_bit_6_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "6,B" << "\n";
    instruction_bit_generic(6, getRegisterB());
}

void GBZ80::instruction_bit_6_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "6,C" << "\n";
    instruction_bit_generic(6, getRegisterC());
}

void GBZ80::instruction_bit_6_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "6,D" << "\n";
    instruction_bit_generic(6, getRegisterD());
}

void GBZ80::instruction_bit_6_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "6,E" << "\n";
    instruction_bit_generic(6, getRegisterE());
}

void GBZ80::instruction_bit_6_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "6,H" << "\n";
    instruction_bit_generic(6, getRegisterH());
}

void GBZ80::instruction_bit_6_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "6,L" << "\n";
    instruction_bit_generic(6, getRegisterL());
}

void GBZ80::instruction_bit_6_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "6,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_bit_generic(6, val);
}

void GBZ80::instruction_bit_7_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "7,A" << "\n";
    instruction_bit_generic(7, getRegisterA());
}

void GBZ80::instruction_bit_7_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "7,B" << "\n";
    instruction_bit_generic(7, getRegisterB());
}

void GBZ80::instruction_bit_7_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "7,C" << "\n";
    instruction_bit_generic(7, getRegisterC());
}

void GBZ80::instruction_bit_7_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "7,D" << "\n";
    instruction_bit_generic(7, getRegisterD());
}

void GBZ80::instruction_bit_7_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "7,E" << "\n";
    instruction_bit_generic(7, getRegisterE());
}

void GBZ80::instruction_bit_7_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "7,H" << "\n";
    instruction_bit_generic(7, getRegisterH());
}

void GBZ80::instruction_bit_7_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "7,L" << "\n";
    instruction_bit_generic(7, getRegisterL());
}

void GBZ80::instruction_bit_7_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "bit " << "7,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    instruction_bit_generic(7, val);
}

//Set the given bit of register to 1
uint8_t GBZ80::instruction_set_generic(uint8_t bit, uint8_t val){
    val |= (1 << bit);
    return val;
}
    
void GBZ80::instruction_set_0_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "0,A" << "\n";
    setRegisterA(instruction_set_generic(0, getRegisterA()));
}

void GBZ80::instruction_set_0_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "0,B" << "\n";
    setRegisterB(instruction_set_generic(0, getRegisterB()));
}

void GBZ80::instruction_set_0_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "0,C" << "\n";
    setRegisterC(instruction_set_generic(0, getRegisterC()));
}

void GBZ80::instruction_set_0_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "0,D" << "\n";
    setRegisterD(instruction_set_generic(0, getRegisterD()));
}

void GBZ80::instruction_set_0_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << ",E" << "\n";
    setRegisterE(instruction_set_generic(0, getRegisterE()));
}

void GBZ80::instruction_set_0_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "0,H" << "\n";
    setRegisterH(instruction_set_generic(0, getRegisterH()));
}

void GBZ80::instruction_set_0_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "0,L" << "\n";
    setRegisterL(instruction_set_generic(0, getRegisterL()));
}

void GBZ80::instruction_set_0_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "0,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_set_generic(0, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_set_1_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "1,A" << "\n";
    setRegisterA(instruction_set_generic(1, getRegisterA()));
}

void GBZ80::instruction_set_1_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "1,B" << "\n";
    setRegisterB(instruction_set_generic(1, getRegisterB()));
}

void GBZ80::instruction_set_1_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "1,C" << "\n";
    setRegisterC(instruction_set_generic(1, getRegisterC()));
}

void GBZ80::instruction_set_1_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "1,D" << "\n";
    setRegisterD(instruction_set_generic(1, getRegisterD()));
}

void GBZ80::instruction_set_1_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << ",E" << "\n";
    setRegisterE(instruction_set_generic(1, getRegisterE()));
}

void GBZ80::instruction_set_1_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "1,H" << "\n";
    setRegisterH(instruction_set_generic(1, getRegisterH()));
}

void GBZ80::instruction_set_1_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "1,L" << "\n";
    setRegisterL(instruction_set_generic(1, getRegisterL()));
}

void GBZ80::instruction_set_1_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "1,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_set_generic(1, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_set_2_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "2,A" << "\n";
    setRegisterA(instruction_set_generic(2, getRegisterA()));
}

void GBZ80::instruction_set_2_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "2,B" << "\n";
    setRegisterB(instruction_set_generic(2, getRegisterB()));
}

void GBZ80::instruction_set_2_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "2,C" << "\n";
    setRegisterC(instruction_set_generic(2, getRegisterC()));
}

void GBZ80::instruction_set_2_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "2,D" << "\n";
    setRegisterD(instruction_set_generic(2, getRegisterD()));
}

void GBZ80::instruction_set_2_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << ",E" << "\n";
    setRegisterE(instruction_set_generic(2, getRegisterE()));
}

void GBZ80::instruction_set_2_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "2,H" << "\n";
    setRegisterH(instruction_set_generic(2, getRegisterH()));
}

void GBZ80::instruction_set_2_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "2,L" << "\n";
    setRegisterL(instruction_set_generic(2, getRegisterL()));
}

void GBZ80::instruction_set_2_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "2,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_set_generic(2, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_set_3_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "3,A" << "\n";
    setRegisterA(instruction_set_generic(3, getRegisterA()));
}

void GBZ80::instruction_set_3_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "3,B" << "\n";
    setRegisterB(instruction_set_generic(3, getRegisterB()));
}

void GBZ80::instruction_set_3_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "3,C" << "\n";
    setRegisterC(instruction_set_generic(3, getRegisterC()));
}

void GBZ80::instruction_set_3_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "3,D" << "\n";
    setRegisterD(instruction_set_generic(3, getRegisterD()));
}

void GBZ80::instruction_set_3_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << ",E" << "\n";
    setRegisterE(instruction_set_generic(3, getRegisterE()));
}

void GBZ80::instruction_set_3_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "3,H" << "\n";
    setRegisterH(instruction_set_generic(3, getRegisterH()));
}

void GBZ80::instruction_set_3_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "3,L" << "\n";
    setRegisterL(instruction_set_generic(3, getRegisterL()));
}

void GBZ80::instruction_set_3_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "3,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_set_generic(3, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_set_4_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "4,A" << "\n";
    setRegisterA(instruction_set_generic(4, getRegisterA()));
}

void GBZ80::instruction_set_4_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "4,B" << "\n";
    setRegisterB(instruction_set_generic(4, getRegisterB()));
}

void GBZ80::instruction_set_4_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "4,C" << "\n";
    setRegisterC(instruction_set_generic(4, getRegisterC()));
}

void GBZ80::instruction_set_4_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "4,D" << "\n";
    setRegisterD(instruction_set_generic(4, getRegisterD()));
}

void GBZ80::instruction_set_4_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << ",E" << "\n";
    setRegisterE(instruction_set_generic(4, getRegisterE()));
}

void GBZ80::instruction_set_4_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "4,H" << "\n";
    setRegisterH(instruction_set_generic(4, getRegisterH()));
}

void GBZ80::instruction_set_4_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "4,L" << "\n";
    setRegisterL(instruction_set_generic(4, getRegisterL()));
}

void GBZ80::instruction_set_4_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "4,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_set_generic(4, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_set_5_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "5,A" << "\n";
    setRegisterA(instruction_set_generic(5, getRegisterA()));
}

void GBZ80::instruction_set_5_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "5,B" << "\n";
    setRegisterB(instruction_set_generic(5, getRegisterB()));
}

void GBZ80::instruction_set_5_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "5,C" << "\n";
    setRegisterC(instruction_set_generic(5, getRegisterC()));
}

void GBZ80::instruction_set_5_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "5,D" << "\n";
    setRegisterD(instruction_set_generic(5, getRegisterD()));
}

void GBZ80::instruction_set_5_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << ",E" << "\n";
    setRegisterE(instruction_set_generic(5, getRegisterE()));
}

void GBZ80::instruction_set_5_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "5,H" << "\n";
    setRegisterH(instruction_set_generic(5, getRegisterH()));
}

void GBZ80::instruction_set_5_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "5,L" << "\n";
    setRegisterL(instruction_set_generic(5, getRegisterL()));
}

void GBZ80::instruction_set_5_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "5,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_set_generic(5, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_set_6_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "6,A" << "\n";
    setRegisterA(instruction_set_generic(6, getRegisterA()));
}

void GBZ80::instruction_set_6_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "6,B" << "\n";
    setRegisterB(instruction_set_generic(6, getRegisterB()));
}

void GBZ80::instruction_set_6_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "6,C" << "\n";
    setRegisterC(instruction_set_generic(6, getRegisterC()));
}

void GBZ80::instruction_set_6_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "6,D" << "\n";
    setRegisterD(instruction_set_generic(6, getRegisterD()));
}

void GBZ80::instruction_set_6_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << ",E" << "\n";
    setRegisterE(instruction_set_generic(6, getRegisterE()));
}

void GBZ80::instruction_set_6_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "6,H" << "\n";
    setRegisterH(instruction_set_generic(6, getRegisterH()));
}

void GBZ80::instruction_set_6_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "6,L" << "\n";
    setRegisterL(instruction_set_generic(6, getRegisterL()));
}

void GBZ80::instruction_set_6_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "6,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_set_generic(6, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_set_7_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "7,A" << "\n";
    setRegisterA(instruction_set_generic(7, getRegisterA()));
}

void GBZ80::instruction_set_7_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "7,B" << "\n";
    setRegisterB(instruction_set_generic(7, getRegisterB()));
}

void GBZ80::instruction_set_7_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "7,C" << "\n";
    setRegisterC(instruction_set_generic(7, getRegisterC()));
}

void GBZ80::instruction_set_7_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "7,D" << "\n";
    setRegisterD(instruction_set_generic(7, getRegisterD()));
}

void GBZ80::instruction_set_7_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << ",E" << "\n";
    setRegisterE(instruction_set_generic(7, getRegisterE()));
}

void GBZ80::instruction_set_7_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "7,H" << "\n";
    setRegisterH(instruction_set_generic(7, getRegisterH()));
}

void GBZ80::instruction_set_7_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "7,L" << "\n";
    setRegisterL(instruction_set_generic(7, getRegisterL()));
}

void GBZ80::instruction_set_7_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "set " << "7,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_set_generic(7, val);
    m_gbmemory->write(getRegisterHL(), val);
}

uint8_t GBZ80::instruction_res_generic(uint8_t bit, uint8_t val){
    val &= ~(1 << bit);
    return val;
}

//Reset the given bit of register to 0
void GBZ80::instruction_res_0_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "0,A" << "\n";
    setRegisterA(instruction_res_generic(0, getRegisterA()));
}

void GBZ80::instruction_res_0_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "0,B" << "\n";
    setRegisterB(instruction_res_generic(0, getRegisterB()));
}

void GBZ80::instruction_res_0_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "0,C" << "\n";
    setRegisterC(instruction_res_generic(0, getRegisterC()));
}

void GBZ80::instruction_res_0_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "0,D" << "\n";
    setRegisterD(instruction_res_generic(0, getRegisterD()));
}

void GBZ80::instruction_res_0_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "0,E" << "\n";
    setRegisterE(instruction_res_generic(0, getRegisterE()));
}

void GBZ80::instruction_res_0_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "0,H" << "\n";
    setRegisterH(instruction_res_generic(0, getRegisterH()));
}

void GBZ80::instruction_res_0_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "0,L" << "\n";
    setRegisterL(instruction_res_generic(0, getRegisterL()));
}

void GBZ80::instruction_res_0_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "0,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_res_generic(0, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_res_1_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "1,A" << "\n";
    setRegisterA(instruction_res_generic(1, getRegisterA()));
}

void GBZ80::instruction_res_1_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "1,B" << "\n";
    setRegisterB(instruction_res_generic(1, getRegisterB()));
}

void GBZ80::instruction_res_1_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "1,C" << "\n";
    setRegisterC(instruction_res_generic(1, getRegisterC()));
}

void GBZ80::instruction_res_1_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "1,D" << "\n";
    setRegisterD(instruction_res_generic(1, getRegisterD()));
}

void GBZ80::instruction_res_1_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "1,E" << "\n";
    setRegisterE(instruction_res_generic(1, getRegisterE()));
}

void GBZ80::instruction_res_1_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "1,H" << "\n";
    setRegisterH(instruction_res_generic(1, getRegisterH()));
}

void GBZ80::instruction_res_1_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "1,L" << "\n";
    setRegisterL(instruction_res_generic(1, getRegisterL()));
}

void GBZ80::instruction_res_1_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "1,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_res_generic(1, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_res_2_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "2,A" << "\n";
    setRegisterA(instruction_res_generic(2, getRegisterA()));
}

void GBZ80::instruction_res_2_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "2,B" << "\n";
    setRegisterB(instruction_res_generic(2, getRegisterB()));
}

void GBZ80::instruction_res_2_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "2,C" << "\n";
    setRegisterC(instruction_res_generic(2, getRegisterC()));
}

void GBZ80::instruction_res_2_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "2,D" << "\n";
    setRegisterD(instruction_res_generic(2, getRegisterD()));
}

void GBZ80::instruction_res_2_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "2,E" << "\n";
    setRegisterE(instruction_res_generic(2, getRegisterE()));
}

void GBZ80::instruction_res_2_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "2,H" << "\n";
    setRegisterH(instruction_res_generic(2, getRegisterH()));
}

void GBZ80::instruction_res_2_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "2,L" << "\n";
    setRegisterL(instruction_res_generic(2, getRegisterL()));
}

void GBZ80::instruction_res_2_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "2,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_res_generic(2, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_res_3_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "3,A" << "\n";
    setRegisterA(instruction_res_generic(3, getRegisterA()));
}

void GBZ80::instruction_res_3_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "3,B" << "\n";
    setRegisterB(instruction_res_generic(3, getRegisterB()));
}

void GBZ80::instruction_res_3_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "3,C" << "\n";
    setRegisterC(instruction_res_generic(3, getRegisterC()));
}

void GBZ80::instruction_res_3_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "3,D" << "\n";
    setRegisterD(instruction_res_generic(3, getRegisterD()));
}

void GBZ80::instruction_res_3_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "3,E" << "\n";
    setRegisterE(instruction_res_generic(3, getRegisterE()));
}

void GBZ80::instruction_res_3_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "3,H" << "\n";
    setRegisterH(instruction_res_generic(3, getRegisterH()));
}

void GBZ80::instruction_res_3_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "3,L" << "\n";
    setRegisterL(instruction_res_generic(3, getRegisterL()));
}

void GBZ80::instruction_res_3_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "3,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_res_generic(3, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_res_4_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "4,A" << "\n";
    setRegisterA(instruction_res_generic(4, getRegisterA()));
}

void GBZ80::instruction_res_4_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "4,B" << "\n";
    setRegisterB(instruction_res_generic(4, getRegisterB()));
}

void GBZ80::instruction_res_4_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "4,C" << "\n";
    setRegisterC(instruction_res_generic(4, getRegisterC()));
}

void GBZ80::instruction_res_4_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "4,D" << "\n";
    setRegisterD(instruction_res_generic(4, getRegisterD()));
}

void GBZ80::instruction_res_4_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "4,E" << "\n";
    setRegisterE(instruction_res_generic(4, getRegisterE()));
}

void GBZ80::instruction_res_4_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "4,H" << "\n";
    setRegisterH(instruction_res_generic(4, getRegisterH()));
}

void GBZ80::instruction_res_4_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "4,L" << "\n";
    setRegisterL(instruction_res_generic(4, getRegisterL()));
}

void GBZ80::instruction_res_4_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "4,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_res_generic(4, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_res_5_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "5,A" << "\n";
    setRegisterA(instruction_res_generic(5, getRegisterA()));
}

void GBZ80::instruction_res_5_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "5,B" << "\n";
    setRegisterB(instruction_res_generic(5, getRegisterB()));
}

void GBZ80::instruction_res_5_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "5,C" << "\n";
    setRegisterC(instruction_res_generic(5, getRegisterC()));
}

void GBZ80::instruction_res_5_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "5,D" << "\n";
    setRegisterD(instruction_res_generic(5, getRegisterD()));
}

void GBZ80::instruction_res_5_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "5,E" << "\n";
    setRegisterE(instruction_res_generic(5, getRegisterE()));
}

void GBZ80::instruction_res_5_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "5,H" << "\n";
    setRegisterH(instruction_res_generic(5, getRegisterH()));
}

void GBZ80::instruction_res_5_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "5,L" << "\n";
    setRegisterL(instruction_res_generic(5, getRegisterL()));
}

void GBZ80::instruction_res_5_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "5,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_res_generic(5, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_res_6_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "6,A" << "\n";
    setRegisterA(instruction_res_generic(6, getRegisterA()));
}

void GBZ80::instruction_res_6_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "6,B" << "\n";
    setRegisterB(instruction_res_generic(6, getRegisterB()));
}

void GBZ80::instruction_res_6_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "6,C" << "\n";
    setRegisterC(instruction_res_generic(6, getRegisterC()));
}

void GBZ80::instruction_res_6_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "6,D" << "\n";
    setRegisterD(instruction_res_generic(6, getRegisterD()));
}

void GBZ80::instruction_res_6_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "6,E" << "\n";
    setRegisterE(instruction_res_generic(6, getRegisterE()));
}

void GBZ80::instruction_res_6_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "6,H" << "\n";
    setRegisterH(instruction_res_generic(6, getRegisterH()));
}

void GBZ80::instruction_res_6_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "6,L" << "\n";
    setRegisterL(instruction_res_generic(6, getRegisterL()));
}

void GBZ80::instruction_res_6_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "6,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_res_generic(6, val);
    m_gbmemory->write(getRegisterHL(), val);
}

void GBZ80::instruction_res_7_A(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "7,A" << "\n";
    setRegisterA(instruction_res_generic(7, getRegisterA()));
}

void GBZ80::instruction_res_7_B(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "7,B" << "\n";
    setRegisterB(instruction_res_generic(7, getRegisterB()));
}

void GBZ80::instruction_res_7_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "7,C" << "\n";
    setRegisterC(instruction_res_generic(7, getRegisterC()));
}

void GBZ80::instruction_res_7_D(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "7,D" << "\n";
    setRegisterD(instruction_res_generic(7, getRegisterD()));
}

void GBZ80::instruction_res_7_E(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "7,E" << "\n";
    setRegisterE(instruction_res_generic(7, getRegisterE()));
}

void GBZ80::instruction_res_7_H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "7,H" << "\n";
    setRegisterH(instruction_res_generic(7, getRegisterH()));
}

void GBZ80::instruction_res_7_L(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "7,L" << "\n";
    setRegisterL(instruction_res_generic(7, getRegisterL()));
}

void GBZ80::instruction_res_7_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "res " << "7,(HL)" << "\n";
    uint8_t val = m_gbmemory->read(getRegisterHL());
    val = instruction_res_generic(7, val);
    m_gbmemory->write(getRegisterHL(), val);
}

//Jump instructions

//Jumps to the given address
void GBZ80::instruction_jp(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "jp " << nn << "\n";
    PC = nn;
}

//Jumps to the given address if NZ
void GBZ80::instruction_jp_NZ(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "jp nz," << nn << "\n";
    if(!getFlag_Z()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is not zero. Jumping" << "\n";
        PC = nn;
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is zero. Not jumping" << "\n";
    }
}
  
//Jumps to the given address if Z
void GBZ80::instruction_jp_Z(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "jp z," << nn << "\n";
    if(getFlag_Z()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is zero. Jumping" << "\n";
        PC = nn;
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is not zero. Not jumping." << "\n";
    }
}

  
//Jumps to the given address if NC
void GBZ80::instruction_jp_NC(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "jp nc," << nn << "\n";
    if(!getFlag_C()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is not set. Jumping" << "\n";
        PC = nn;
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is set. Not jumping" << "\n";
    }
}
  
//Jumps to the given address if C
void GBZ80::instruction_jp_C(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "jp c," << nn << "\n";
    if(getFlag_C()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is set. Jumping" << "\n";
        PC = nn;
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is not set. Not jumping" << "\n";
    }
}
  
//Jumps to address contained in HL
//TODO - RENAME TO jp HL SINCE THIS IS NOT ACTUALLY AN INDIRECT INSTRUCTION ACCORDING TO BGB!!!
void GBZ80::instruction_jp_HLI(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "jp HL" << "\n";
    PC = HL;
}


//Adds n to current address and jumps to it
void GBZ80::instruction_jr(int8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "jr " << +(PC + n) << " (PC + " << +n  << ")" << "\n";
    PC += n;
}

//Adds n to current address and jumps to it if NZ
void GBZ80::instruction_jr_NZ(int8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "jr nz," << +(PC + n)  << " (PC + " << +n  << ")" << "\n";
    
    if(!getFlag_Z()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is not zero. Jumping" << "\n";
        PC += n;
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is zero. Not jumping" << "\n";
    }
}
  
//Adds n to current address and jumps to it if Z
void GBZ80::instruction_jr_Z(int8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "jr z," << +(PC + n) << " (PC + " << +n  << ")" << "\n";
    if(getFlag_Z()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is zero. Jumping" << "\n";
        PC += n;
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is not zero. Not jumping" << "\n";
    }
}
  
//Adds n to current address and jumps to it if NC
void GBZ80::instruction_jr_NC(int8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "jr nc," << +(PC + n) << " (PC + " << +n  << ")" << "\n";
    if(!getFlag_C()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is not set. Jumping" << "\n";
        PC += n;
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is set. Not jumping" << "\n";
    }
}
  
//Adds n to current address and jumps to it if C
void GBZ80::instruction_jr_C(int8_t n){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "jr c," << +(PC + n) << " (PC + " << +n  << ")" << "\n";
    if(getFlag_C()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is set. Jumping" << "\n";
        PC += n;
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is not set. Not jumping" << "\n";
    }
}

//Call instructions

//Push address of next instruction onto stack and jump to address nn
void GBZ80::instruction_call(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "call " << nn;
    instruction_push_generic(PC);
    PC = nn;
}

//Calls address nn if NZ
void GBZ80::instruction_call_NZ(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "call nz," << nn << " via call " << nn << "\n";
    if(!getFlag_Z()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is not zero. Calling" << "\n";
        instruction_call(nn);
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is zero. Not calling" << "\n";
    }
}
  
//Calls address nn if Z
void GBZ80::instruction_call_Z(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "call z," << nn << " via call " << nn << "\n";
    if(getFlag_Z()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is zero. Calling" << "\n";
        instruction_call(nn);
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is not zero. Not calling" << "\n";
    }
}
  
//Calls address nn if NC
void GBZ80::instruction_call_NC(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "call nc," << nn << " via call " << nn << "\n";
    if(!getFlag_C()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is not set. Calling" << "\n";
        instruction_call(nn);
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is set. Not calling" << "\n";
    }
}
  
//Calls address nn if C
void GBZ80::instruction_call_C(uint16_t nn){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "call c," << nn << " via call " << nn << "\n";
    if(getFlag_C()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is set. Calling" << "\n";
        instruction_call(nn);
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is not set. Not calling" << "\n";
    }
}

//Restart instructions

//Push present address onto stack and jump to $0000 + hex constant
void GBZ80::instruction_rst_00H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rst 00H" << "\n";
    instruction_push_generic(PC);
    PC = 0x00;
}

void GBZ80::instruction_rst_08H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rst 08H" << "\n";
    instruction_push_generic(PC);
    PC = 0x08;
}

void GBZ80::instruction_rst_10H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rst 10H" << "\n";
    instruction_push_generic(PC);
    PC = 0x10;
}

void GBZ80::instruction_rst_18H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rst 18H" << "\n";
    instruction_push_generic(PC);
    PC = 0x18;
}

void GBZ80::instruction_rst_20H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rst 20H" << "\n";
    instruction_push_generic(PC);
    PC = 0x20;
}

void GBZ80::instruction_rst_28H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rst 28H" << "\n";
    instruction_push_generic(PC);
    PC = 0x28;
}

void GBZ80::instruction_rst_30H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rst 30H" << "\n";
    instruction_push_generic(PC);
    PC = 0x30;
}

void GBZ80::instruction_rst_38H(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "rst 38H" << "\n";
    instruction_push_generic(PC);
    PC = 0x38;
}

//Return instructions

//Pop two bytes from stack and jump to that address
void GBZ80::instruction_ret(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ret" << "\n";
    PC = instruction_pop_generic();
}

//Pop two bytes from stack and jump to that address if NZ
void GBZ80::instruction_ret_NZ(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ret nz" << "\n";
    if(!getFlag_Z()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is not zero. Returning" << "\n";
        PC = instruction_pop_generic();
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is zero. Not returning" << "\n";
    }
}
  
//Pop two bytes from stack and jump to that address if Z
void GBZ80::instruction_ret_Z(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ret z" << "\n";
    if(getFlag_Z()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is zero. Returning" << "\n";
        PC = instruction_pop_generic();
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "Z is not zero. Not returning" << "\n";
    }
}
  
//Pop two bytes from stack and jump to that address if NC
void GBZ80::instruction_ret_NC(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ret nc" << "\n";
    if(!getFlag_C()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is not set. Returning" << "\n";
        PC = instruction_pop_generic();
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is set. Not returning" << "\n";
    }
}
  
//Pop two bytes from stack and jump to that address if C
void GBZ80::instruction_ret_C(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "ret c" << "\n";
    if(getFlag_C()){
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is set. Returning" << "\n";
        PC = instruction_pop_generic();
    } else {
        if(CONSOLE_OUTPUT_ENABLED) std::cout << "C is not set. Not returning" << "\n";
    }
}
  
//Pop two bytes from stack, jump to that address, then enable interrupts (Immediately? Or after one instruction?)
void GBZ80::instruction_reti(){
    if(CONSOLE_OUTPUT_ENABLED) std::cout << "reti" << "\n";
    PC = instruction_pop_generic();
    //This enables interrupts on the next instruction, not after next. Unsure if correct
    
    //Set both interrupt flags at once so that the next flag won't undo the current flag. 
    m_bInterruptsEnabled = true;
    m_bInterruptsEnabledNext = false;
}

//Execution functions

//Grabs the next byte and increments PC
uint8_t GBZ80::read_next_byte(){
    uint8_t next = m_gbmemory->read(getRegisterPC());
    PC++;
    return next;
}

//Grabs the next two bytes and increments PC twice
uint16_t GBZ80::read_next_word(){
    uint16_t next = 0;
    next = read_next_byte();
    next = setMSB(next, read_next_byte());
    return next;
}

//Returns the cycle length for the passed in opcode
int GBZ80::get_cycle_length(uint8_t opcode){
    //Default to 4 since this is the minimum cycle length for an instruction
    int toReturn = 4;
    
    //Coppied and adapted from process_opcode
    uint16_t cbop = 0;
    
    switch(opcode){
        case OP_LD_A_CONST:
        case OP_LD_B:
        case OP_LD_C:
        case OP_LD_D:
        case OP_LD_E:
        case OP_LD_H:
        case OP_LD_L:
            toReturn = CYCLES_OP_LD_R_N;
            break;
        case OP_LD_A_A:
        case OP_LD_A_B:
        case OP_LD_A_C:
        case OP_LD_A_D:
        case OP_LD_A_E:
        case OP_LD_A_H:
        case OP_LD_A_L:
        case OP_LD_B_A:
        case OP_LD_B_B:
        case OP_LD_B_C:
        case OP_LD_B_D:
        case OP_LD_B_E:
        case OP_LD_B_H:
        case OP_LD_B_L:
        case OP_LD_C_A:
        case OP_LD_C_B:
        case OP_LD_C_C:
        case OP_LD_C_D:
        case OP_LD_C_E:
        case OP_LD_C_H:
        case OP_LD_C_L:
        case OP_LD_D_A:
        case OP_LD_D_B:
        case OP_LD_D_C:
        case OP_LD_D_D:
        case OP_LD_D_E:
        case OP_LD_D_H:
        case OP_LD_D_L:
        case OP_LD_E_A:
        case OP_LD_E_B:
        case OP_LD_E_C:
        case OP_LD_E_D:
        case OP_LD_E_E:
        case OP_LD_E_H:
        case OP_LD_E_L:
        case OP_LD_H_A:
        case OP_LD_H_B:
        case OP_LD_H_C:
        case OP_LD_H_D:
        case OP_LD_H_E:
        case OP_LD_H_H:
        case OP_LD_H_L:
        case OP_LD_L_A:
        case OP_LD_L_B:
        case OP_LD_L_C:
        case OP_LD_L_D:
        case OP_LD_L_E:
        case OP_LD_L_H:
        case OP_LD_L_L:
            toReturn = CYCLES_OP_LD_R_R;
            break;
        case OP_LD_A_HLI:
        case OP_LD_B_HLI:
        case OP_LD_C_HLI:
        case OP_LD_D_HLI:
        case OP_LD_E_HLI:
        case OP_LD_H_HLI:
        case OP_LD_L_HLI:
        case OP_LD_HLI_A:
        case OP_LD_HLI_B:
        case OP_LD_HLI_C:
        case OP_LD_HLI_D:
        case OP_LD_HLI_E:
        case OP_LD_HLI_H:
        case OP_LD_HLI_L:
        case OP_LD_A_BCI:
        case OP_LD_A_DEI:
        case OP_LD_BCI_A:
        case OP_LD_DEI_A:
            toReturn = CYCLES_OP_LD_R_RRI;
            break;        
        case OP_LD_HLI_N:
            toReturn = CYCLES_OP_LD_HLI_N;
            break;
        case OP_LD_A_NNI:
        case OP_LD_NNI_A:
            toReturn = CYCLES_OP_LD_NNI_R;
            break;
        case OP_LD_A_FF00CI:
        case OP_LD_FF00CI_A:
            toReturn = CYCLES_OP_LD_FF00CI;
            break;
        case OP_LDD_A_HLI:
        case OP_LDD_HLI_A:
            toReturn = CYCLES_OP_LDD;
            break;
        case OP_LDI_A_HLI:
        case OP_LDI_HLI_A:
            toReturn = CYCLES_OP_LDI;
            break;
        case OP_LDH_FF00NI_A:
        case OP_LDH_A_FF00NI:
            toReturn = CYCLES_OP_LDH;
            break;
        case OP_LD_BC_NN:
        case OP_LD_DE_NN:
        case OP_LD_HL_NN:
        case OP_LD_SP_NN:
            toReturn = CYCLES_OP_LD_RR_NN;
            break;
        case OP_LD_SP_HL:
            toReturn = CYCLES_OP_LD_SP_HL;
            break;
        case OP_LDHL_SP_N:
            toReturn = CYCLES_OP_LDHL;
            break;
        case OP_LD_NNI_SP:
            toReturn = CYCLES_OP_LD_NNI_SP;
            break;
        case OP_PUSH_AF:
        case OP_PUSH_BC:
        case OP_PUSH_DE:
        case OP_PUSH_HL:
            toReturn = CYCLES_OP_PUSH;
            break;
        case OP_POP_AF:
        case OP_POP_BC:
        case OP_POP_DE:
        case OP_POP_HL:
            toReturn = CYCLES_OP_POP;
            break;
        case OP_ADD_A:
        case OP_ADD_B:
        case OP_ADD_C:
        case OP_ADD_D:
        case OP_ADD_E:
        case OP_ADD_H:
        case OP_ADD_L:
            toReturn = CYCLES_OP_ADD_R;
            break;
        case OP_ADD_HLI:
            toReturn = CYCLES_OP_ADD_HLI;
            break;
        case OP_ADD_CONST:
            toReturn = CYCLES_OP_ADD_N;
            break;
        case OP_ADC_A:
        case OP_ADC_B:
        case OP_ADC_C:
        case OP_ADC_D:
        case OP_ADC_E:
        case OP_ADC_H:
        case OP_ADC_L:
            toReturn = CYCLES_OP_ADC_R;
            break;
        case OP_ADC_HLI:
            toReturn = CYCLES_OP_ADC_HLI;
            break;
        case OP_ADC_CONST:
            toReturn = CYCLES_OP_ADC_N;
            break;
        case OP_SUB_A:
        case OP_SUB_B:
        case OP_SUB_C:
        case OP_SUB_D:
        case OP_SUB_E:
        case OP_SUB_H:
        case OP_SUB_L:
            toReturn = CYCLES_OP_SUB_R;
            break;
        case OP_SUB_HLI:
            toReturn = CYCLES_OP_SUB_HLI;
            break;
        case OP_SUB_CONST:
            toReturn = CYCLES_OP_SUB_N;
            break;
        case OP_SBC_A:
        case OP_SBC_B:
        case OP_SBC_C:
        case OP_SBC_D:
        case OP_SBC_E:
        case OP_SBC_H:
        case OP_SBC_L:
            toReturn = CYCLES_OP_SBC_R;
            break;
        case OP_SBC_HLI:
            toReturn = CYCLES_OP_SBC_HLI;
            break;
        case OP_SBC_CONST:
            toReturn = CYCLES_OP_SBC_N;
            break;
        case OP_AND_A:
        case OP_AND_B:
        case OP_AND_C:
        case OP_AND_D:
        case OP_AND_E:
        case OP_AND_H:
        case OP_AND_L:
            toReturn = CYCLES_OP_AND_R;
            break;
        case OP_AND_HLI:
            toReturn = CYCLES_OP_AND_HLI;
            break;
        case OP_AND_CONST:
            toReturn = CYCLES_OP_AND_N;
            break;
        case OP_OR_A:
        case OP_OR_B:
        case OP_OR_C:
        case OP_OR_D:
        case OP_OR_E:
        case OP_OR_H:
        case OP_OR_L:
            toReturn = CYCLES_OP_OR_R;
            break;
        case OP_OR_HLI:
            toReturn = CYCLES_OP_OR_HLI;
            break;
        case OP_OR_CONST:
            toReturn = CYCLES_OP_OR_N;
            break;
        case OP_XOR_A:
        case OP_XOR_B:
        case OP_XOR_C:
        case OP_XOR_D:
        case OP_XOR_E:
        case OP_XOR_H:
        case OP_XOR_L:
            toReturn = CYCLES_XOR_R;
            break;
        case OP_XOR_HLI:
            toReturn = CYCLES_XOR_HLI;
            break;
        case OP_XOR_CONST:
            toReturn = CYCLES_XOR_N;
            break;
        case OP_CP_A:
        case OP_CP_B:
        case OP_CP_C:
        case OP_CP_D:
        case OP_CP_E:
        case OP_CP_H:
        case OP_CP_L:
            toReturn = CYCLES_CP_R;
            break;
        case OP_CP_HLI:
            toReturn = CYCLES_CP_HLI;
            break;
        case OP_CP_CONST:
            toReturn = CYCLES_CP_N;
            break;
        case OP_INC_A:
        case OP_INC_B:
        case OP_INC_C:
        case OP_INC_D:
        case OP_INC_E:
        case OP_INC_H:
        case OP_INC_L:
            toReturn = CYCLES_INC_R;
            break;
        case OP_INC_HLI:
            toReturn = CYCLES_INC_HLI;
            break;
        case OP_DEC_A:
        case OP_DEC_B:
        case OP_DEC_C:
        case OP_DEC_D:
        case OP_DEC_E:
        case OP_DEC_H:
        case OP_DEC_L:
            toReturn = CYCLES_DEC_R;
            break;
        case OP_DEC_HLI:
            toReturn = CYCLES_DEC_HLI;
            break;
        case OP_ADD_BC:
        case OP_ADD_DE:
        case OP_ADD_HL:
        case OP_ADD_SP:
            toReturn = CYCLES_ADD_HL_RR;
            break;
        case OP_ADD_SP_CONST:
            toReturn = CYCLES_ADD_SP_N;
            break;
        case OP_INC_BC:
        case OP_INC_DE:
        case OP_INC_HL:
        case OP_INC_SP:
            toReturn = CYCLES_INC_RR;
            break;
        case OP_DEC_BC:
        case OP_DEC_DE:
        case OP_DEC_HL:
        case OP_DEC_SP:
            toReturn = CYCLES_DEC_RR;
            break;
        //SWAP instructions appear later with other CB prefixes
        case OP_DAA:
            toReturn = CYCLES_DAA;
            break;
        case OP_CPL:
            toReturn = CYCLES_CPL;
            break;
        case OP_CCF:
            toReturn = CYCLES_CCF;
            break;
        case OP_SCF:
            toReturn = CYCLES_SCF;
            break;
        case OP_NOP:
            toReturn = CYCLES_NOP;
            break;
        case OP_HALT:
            toReturn = CYCLES_HALT;
            break;
        //Stop is at end since it is a two byte opcode
        case OP_DI:
            toReturn = CYCLES_DI;
            break;
        case OP_EI:
            toReturn = CYCLES_EI;
            break;
        case OP_RLCA:
            toReturn = CYCLES_RLCA;
            break;
        case OP_RLA:
            toReturn = CYCLES_RLA;
            break;
        case OP_RRCA:
            toReturn = CYCLES_RRCA;
            break;
        case OP_RRA:
            toReturn = CYCLES_RRA;
            break;
        //CB instructions appear after all single byte opcodes
        case OP_JP:
        case OP_JP_NZ:
        case OP_JP_Z:
        case OP_JP_NC:
        case OP_JP_C:
            toReturn = CYCLES_JP_NN;
            break;
        case OP_JP_HLI:
            toReturn = CYCLES_JP_HLI;
            break;
        case OP_JR:
        case OP_JR_NZ:
        case OP_JR_Z:
        case OP_JR_NC:
        case OP_JR_C:
            toReturn = CYCLES_JR;
            break;
        case OP_CALL:
        case OP_CALL_NZ:
        case OP_CALL_Z:
        case OP_CALL_NC:
        case OP_CALL_C:
            toReturn = CYCLES_CALL;
            break;
        case OP_RST_00H:
        case OP_RST_08H:
        case OP_RST_10H:
        case OP_RST_18H:
        case OP_RST_20H:
        case OP_RST_28H:
        case OP_RST_30H:
        case OP_RST_38H:
            toReturn = CYCLES_RST;
            break;
        case OP_RET:
        case OP_RET_NZ:
        case OP_RET_Z:
        case OP_RET_NC:
        case OP_RET_C:
        case OP_RETI:
            toReturn = CYCLES_RET;
            break;
        case OP_STOP8:
            toReturn = CYCLES_STOP;
            break;
        case OP_IS_CB_PREFIXED:
            cbop = setMSB(m_gbmemory->read(PC + 1), opcode);
            if(CONSOLE_OUTPUT_ENABLED) std::cout << "cbopcode: " << +cbop << "\n";
            switch(cbop){
                case OP_SWAP_A:
                case OP_SWAP_B:
                case OP_SWAP_C:
                case OP_SWAP_D:
                case OP_SWAP_E:
                case OP_SWAP_H:
                case OP_SWAP_L:
                    toReturn = CYCLES_SWAP_R;
                    break;
                case OP_SWAP_HLI:
                    toReturn = CYCLES_SWAP_HLI;
                    break;
                case OP_RLC_A:
                case OP_RLC_B:
                case OP_RLC_C:
                case OP_RLC_D:
                case OP_RLC_E:
                case OP_RLC_H:
                case OP_RLC_L:
                    toReturn = CYCLES_RLC_R;
                    break;
                case OP_RLC_HLI:
                    toReturn = CYCLES_RLC_HLI;
                    break;
                case OP_RL_A:
                case OP_RL_B:
                case OP_RL_C:
                case OP_RL_D:
                case OP_RL_E:
                case OP_RL_H:
                case OP_RL_L:
                    toReturn = CYCLES_RL_R;
                    break;
                case OP_RL_HLI:
                    toReturn = CYCLES_RL_HLI;
                    break;
                case OP_RRC_A:
                case OP_RRC_B:
                case OP_RRC_C:
                case OP_RRC_D:
                case OP_RRC_E:
                case OP_RRC_H:
                case OP_RRC_L:
                    toReturn = CYCLES_RRC_R;
                    break;
                case OP_RRC_HLI:
                    toReturn = CYCLES_RRC_HLI;
                    break;
                case OP_RR_A:
                case OP_RR_B:
                case OP_RR_C:
                case OP_RR_D:
                case OP_RR_E:
                case OP_RR_H:
                case OP_RR_L:
                    toReturn = CYCLES_RR_R;
                    break;
                case OP_RR_HLI:
                    toReturn = CYCLES_RR_HLI;
                    break;
                case OP_SLA_A:
                case OP_SLA_B:
                case OP_SLA_C:
                case OP_SLA_D:
                case OP_SLA_E:
                case OP_SLA_H:
                case OP_SLA_L:
                    toReturn = CYCLES_SLA_R;
                    break;
                case OP_SLA_HLI:
                    toReturn = CYCLES_SLA_HLI;
                    break;
                case OP_SRA_A:
                case OP_SRA_B:
                case OP_SRA_C:
                case OP_SRA_D:
                case OP_SRA_E:
                case OP_SRA_H:
                case OP_SRA_L:
                    toReturn = CYCLES_SRA_R;
                    break;
                case OP_SRA_HLI:
                    toReturn = CYCLES_SRA_HLI;
                    break;
                case OP_SRL_A:
                case OP_SRL_B:
                case OP_SRL_C:
                case OP_SRL_D:
                case OP_SRL_E:
                case OP_SRL_H:
                case OP_SRL_L:
                    toReturn = CYCLES_SRL_R;
                    break;
                case OP_SRL_HLI:
                    toReturn = CYCLES_SRL_HLI;
                    break;
                case OP_BIT_0_A:
                case OP_BIT_0_B:
                case OP_BIT_0_C:
                case OP_BIT_0_D:
                case OP_BIT_0_E:
                case OP_BIT_0_H:
                case OP_BIT_0_L:
                case OP_BIT_1_A:
                case OP_BIT_1_B:
                case OP_BIT_1_C:
                case OP_BIT_1_D:
                case OP_BIT_1_E:
                case OP_BIT_1_H:
                case OP_BIT_1_L:
                case OP_BIT_2_A:
                case OP_BIT_2_B:
                case OP_BIT_2_C:
                case OP_BIT_2_D:
                case OP_BIT_2_E:
                case OP_BIT_2_H:
                case OP_BIT_2_L:
                case OP_BIT_3_A:
                case OP_BIT_3_B:
                case OP_BIT_3_C:
                case OP_BIT_3_D:
                case OP_BIT_3_E:
                case OP_BIT_3_H:
                case OP_BIT_3_L:
                case OP_BIT_4_A:
                case OP_BIT_4_B:
                case OP_BIT_4_C:
                case OP_BIT_4_D:
                case OP_BIT_4_E:
                case OP_BIT_4_H:
                case OP_BIT_4_L:
                case OP_BIT_5_A:
                case OP_BIT_5_B:
                case OP_BIT_5_C:
                case OP_BIT_5_D:
                case OP_BIT_5_E:
                case OP_BIT_5_H:
                case OP_BIT_5_L:
                case OP_BIT_6_A:
                case OP_BIT_6_B:
                case OP_BIT_6_C:
                case OP_BIT_6_D:
                case OP_BIT_6_E:
                case OP_BIT_6_H:
                case OP_BIT_6_L:
                case OP_BIT_7_A:
                case OP_BIT_7_B:
                case OP_BIT_7_C:
                case OP_BIT_7_D:
                case OP_BIT_7_E:
                case OP_BIT_7_H:
                case OP_BIT_7_L:
                    toReturn = CYCLES_BIT_B_R;
                    break;
                case OP_BIT_0_HLI:
                case OP_BIT_1_HLI:
                case OP_BIT_2_HLI:
                case OP_BIT_3_HLI:
                case OP_BIT_4_HLI:
                case OP_BIT_5_HLI:
                case OP_BIT_6_HLI:
                case OP_BIT_7_HLI:
                    toReturn = CYCLES_BIT_B_HLI;
                    break;
                case OP_SET_0_A:
                case OP_SET_0_B:
                case OP_SET_0_C:
                case OP_SET_0_D:
                case OP_SET_0_E:
                case OP_SET_0_H:
                case OP_SET_0_L:
                case OP_SET_1_A:
                case OP_SET_1_B:
                case OP_SET_1_C:
                case OP_SET_1_D:
                case OP_SET_1_E:
                case OP_SET_1_H:
                case OP_SET_1_L:
                case OP_SET_2_A:
                case OP_SET_2_B:
                case OP_SET_2_C:
                case OP_SET_2_D:
                case OP_SET_2_E:
                case OP_SET_2_H:
                case OP_SET_2_L:
                case OP_SET_3_A:
                case OP_SET_3_B:
                case OP_SET_3_C:
                case OP_SET_3_D:
                case OP_SET_3_E:
                case OP_SET_3_H:
                case OP_SET_3_L:
                case OP_SET_4_A:
                case OP_SET_4_B:
                case OP_SET_4_C:
                case OP_SET_4_D:
                case OP_SET_4_E:
                case OP_SET_4_H:
                case OP_SET_4_L:
                case OP_SET_5_A:
                case OP_SET_5_B:
                case OP_SET_5_C:
                case OP_SET_5_D:
                case OP_SET_5_E:
                case OP_SET_5_H:
                case OP_SET_5_L:
                case OP_SET_6_A:
                case OP_SET_6_B:
                case OP_SET_6_C:
                case OP_SET_6_D:
                case OP_SET_6_E:
                case OP_SET_6_H:
                case OP_SET_6_L:
                case OP_SET_7_A:
                case OP_SET_7_B:
                case OP_SET_7_C:
                case OP_SET_7_D:
                case OP_SET_7_E:
                case OP_SET_7_H:
                case OP_SET_7_L:
                    toReturn = CYCLES_SET_B_R;
                    break;
                case OP_SET_0_HLI:
                case OP_SET_1_HLI:
                case OP_SET_2_HLI:
                case OP_SET_3_HLI:
                case OP_SET_4_HLI:
                case OP_SET_5_HLI:
                case OP_SET_6_HLI:
                case OP_SET_7_HLI:
                    toReturn = CYCLES_SET_B_HLI;
                    break;
                case OP_RES_0_A:
                case OP_RES_0_B:
                case OP_RES_0_C:
                case OP_RES_0_D:
                case OP_RES_0_E:
                case OP_RES_0_H:
                case OP_RES_0_L:
                case OP_RES_1_A:
                case OP_RES_1_B:
                case OP_RES_1_C:
                case OP_RES_1_D:
                case OP_RES_1_E:
                case OP_RES_1_H:
                case OP_RES_1_L:
                case OP_RES_2_A:
                case OP_RES_2_B:
                case OP_RES_2_C:
                case OP_RES_2_D:
                case OP_RES_2_E:
                case OP_RES_2_H:
                case OP_RES_2_L:
                case OP_RES_3_A:
                case OP_RES_3_B:
                case OP_RES_3_C:
                case OP_RES_3_D:
                case OP_RES_3_E:
                case OP_RES_3_H:
                case OP_RES_3_L:
                case OP_RES_4_A:
                case OP_RES_4_B:
                case OP_RES_4_C:
                case OP_RES_4_D:
                case OP_RES_4_E:
                case OP_RES_4_H:
                case OP_RES_4_L:
                case OP_RES_5_A:
                case OP_RES_5_B:
                case OP_RES_5_C:
                case OP_RES_5_D:
                case OP_RES_5_E:
                case OP_RES_5_H:
                case OP_RES_5_L:
                case OP_RES_6_A:
                case OP_RES_6_B:
                case OP_RES_6_C:
                case OP_RES_6_D:
                case OP_RES_6_E:
                case OP_RES_6_H:
                case OP_RES_6_L:
                case OP_RES_7_A:
                case OP_RES_7_B:
                case OP_RES_7_C:
                case OP_RES_7_D:
                case OP_RES_7_E:
                case OP_RES_7_H:
                case OP_RES_7_L:
                    toReturn = CYCLES_RES_B_R;
                    break;
                case OP_RES_0_HLI:
                case OP_RES_1_HLI:
                case OP_RES_2_HLI:
                case OP_RES_3_HLI:
                case OP_RES_4_HLI:
                case OP_RES_5_HLI:
                case OP_RES_6_HLI:
                case OP_RES_7_HLI:
                    toReturn = CYCLES_RES_B_HLI;
                    break;
                default:
                    std::cout << "Unrecognized CB prefix opcode " << +cbop << ". Using default cycle length\n";
                    
                    break;
            }
            break;
        default:
            std::cout << "Unrecognized opcode " << +opcode << ". Using default cycle length\n";
            
            break;
    }
    
    return toReturn;
}

void GBZ80::process_opcode(uint8_t opcode){
    uint16_t cbop = 0;
    uint8_t nextByte = 0x00;
    
    //Show debug prompt if needed. //TODO - replace false with breakpoint check.
    if(m_bSingleStep || false){
        showDebugPrompt();
    }
        
    switch(opcode){
        case OP_LD_B:
            instruction_ld_B(read_next_byte());
            break;
        case OP_LD_C:
            instruction_ld_C(read_next_byte());
            break;
        case OP_LD_D:
            instruction_ld_D(read_next_byte());
            break;
        case OP_LD_E:
            instruction_ld_E(read_next_byte());
            break;
        case OP_LD_H:
            instruction_ld_H(read_next_byte());
            break;
        case OP_LD_L:
            instruction_ld_L(read_next_byte());
            break;
        case OP_LD_A_A:
            instruction_ld_A_A();
            break;
        case OP_LD_A_B:
            instruction_ld_A_B();
            break;
        case OP_LD_A_C:
            instruction_ld_A_C();
            break;
        case OP_LD_A_D:
            instruction_ld_A_D();
            break;
        case OP_LD_A_E:
            instruction_ld_A_E();
            break;
        case OP_LD_A_H:
            instruction_ld_A_H();
            break;
        case OP_LD_A_L:
            instruction_ld_A_L();
            break;
        case OP_LD_A_HLI:
            instruction_ld_A_HLI();
            break;
        case OP_LD_B_A:
            instruction_ld_B_A();
            break;
        case OP_LD_B_B:
            instruction_ld_B_B();
            break;
        case OP_LD_B_C:
            instruction_ld_B_C();
            break;
        case OP_LD_B_D:
            instruction_ld_B_D();
            break;
        case OP_LD_B_E:
            instruction_ld_B_E();
            break;
        case OP_LD_B_H:
            instruction_ld_B_H();
            break;
        case OP_LD_B_L:
            instruction_ld_B_L();
            break;
        case OP_LD_B_HLI:
            instruction_ld_B_HLI();
            break;
        case OP_LD_C_A:
            instruction_ld_C_A();
            break;
        case OP_LD_C_B:
            instruction_ld_C_B();
            break;
        case OP_LD_C_C:
            instruction_ld_C_C();
            break;
        case OP_LD_C_D:
            instruction_ld_C_D();
            break;
        case OP_LD_C_E:
            instruction_ld_C_E();
            break;
        case OP_LD_C_H:
            instruction_ld_C_H();
            break;
        case OP_LD_C_L:
            instruction_ld_C_L();
            break;
        case OP_LD_C_HLI:
            instruction_ld_C_HLI();
            break;
        case OP_LD_D_A:
            instruction_ld_D_A();
            break;
        case OP_LD_D_B:
            instruction_ld_D_B();
            break;
        case OP_LD_D_C:
            instruction_ld_D_C();
            break;
        case OP_LD_D_D:
            instruction_ld_D_D();
            break;
        case OP_LD_D_E:
            instruction_ld_D_E();
            break;
        case OP_LD_D_H:
            instruction_ld_D_H();
            break;
        case OP_LD_D_L:
            instruction_ld_D_L();
            break;
        case OP_LD_D_HLI:
            instruction_ld_D_HLI();
            break;
        case OP_LD_E_A:
            instruction_ld_E_A();
            break;
        case OP_LD_E_B:
            instruction_ld_E_B();
            break;
        case OP_LD_E_C:
            instruction_ld_E_C();
            break;
        case OP_LD_E_D:
            instruction_ld_E_D();
            break;
        case OP_LD_E_E:
            instruction_ld_E_E();
            break;
        case OP_LD_E_H:
            instruction_ld_E_H();
            break;
        case OP_LD_E_L:
            instruction_ld_E_L();
            break;
        case OP_LD_E_HLI:
            instruction_ld_E_HLI();
            break;
        case OP_LD_H_A:
            instruction_ld_H_A();
            break;
        case OP_LD_H_B:
            instruction_ld_H_B();
            break;
        case OP_LD_H_C:
            instruction_ld_H_C();
            break;
        case OP_LD_H_D:
            instruction_ld_H_D();
            break;
        case OP_LD_H_E:
            instruction_ld_H_E();
            break;
        case OP_LD_H_H:
            instruction_ld_H_H();
            break;
        case OP_LD_H_L:
            instruction_ld_H_L();
            break;
        case OP_LD_H_HLI:
            instruction_ld_H_HLI();
            break;
        case OP_LD_L_A:
            instruction_ld_L_A();
            break;
        case OP_LD_L_B:
            instruction_ld_L_B();
            break;
        case OP_LD_L_C:
            instruction_ld_L_C();
            break;
        case OP_LD_L_D:
            instruction_ld_L_D();
            break;
        case OP_LD_L_E:
            instruction_ld_L_E();
            break;
        case OP_LD_L_H:
            instruction_ld_L_H();
            break;
        case OP_LD_L_L:
            instruction_ld_L_L();
            break;
        case OP_LD_L_HLI:
            instruction_ld_L_HLI();
            break;
        case OP_LD_HLI_A:
            instruction_ld_HLI_A();
            break;
        case OP_LD_HLI_B:
            instruction_ld_HLI_B();
            break;
        case OP_LD_HLI_C:
            instruction_ld_HLI_C();
            break;
        case OP_LD_HLI_D:
            instruction_ld_HLI_D();
            break;
        case OP_LD_HLI_E:
            instruction_ld_HLI_E();
            break;
        case OP_LD_HLI_H:
            instruction_ld_HLI_H();
            break;
        case OP_LD_HLI_L:
            instruction_ld_HLI_L();
            break;
        case OP_LD_HLI_N:
            instruction_ld_HLI_N(read_next_byte());
            break;
        case OP_LD_A_BCI:
            instruction_ld_A_BCI();
            break;
        case OP_LD_A_DEI:
            instruction_ld_A_DEI();
            break;
        case OP_LD_A_NNI:
            instruction_ld_A_NNI(read_next_word());
            break;
        case OP_LD_A_CONST:
            instruction_ld_A_CONST(read_next_byte());
            break;
        case OP_LD_BCI_A:
            instruction_ld_BCI_A();
            break;
        case OP_LD_DEI_A:
            instruction_ld_DEI_A();
            break;
        case OP_LD_NNI_A:
            instruction_ld_NNI_A(read_next_word());
            break;
        case OP_LD_A_FF00CI:
            instruction_ld_A_FF00CI();
            break;
        case OP_LD_FF00CI_A:
            instruction_ld_FF00CI_A();
            break;
        case OP_LDD_A_HLI:
            instruction_ldd_A_HLI();
            break;
        case OP_LDD_HLI_A:
            instruction_ldd_HLI_A();
            break;
        case OP_LDI_A_HLI:
            instruction_ldi_A_HLI();
            break;
        case OP_LDI_HLI_A:
            instruction_ldi_HLI_A();
            break;
        case OP_LDH_FF00NI_A:
            instruction_ldh_FF00NI_A(read_next_byte());
            break;
        case OP_LDH_A_FF00NI:
            instruction_ldh_A_FF00NI(read_next_byte());
            break;
        case OP_LD_BC_NN:
            instruction_ld_BC_NN(read_next_word());
            break;
        case OP_LD_DE_NN:
            instruction_ld_DE_NN(read_next_word());
            break;
        case OP_LD_HL_NN:
            instruction_ld_HL_NN(read_next_word());
            break;
        case OP_LD_SP_NN:
            instruction_ld_SP_NN(read_next_word());
            break;
        case OP_LD_SP_HL:
            instruction_ld_SP_HL();
            break;
        case OP_LDHL_SP_N:
            nextByte = read_next_byte();
            instruction_ldhl(reinterpret_cast<int8_t &>(nextByte));
            break;
        case OP_LD_NNI_SP:
            instruction_ld_NNI_SP(read_next_word());
            break;
        case OP_PUSH_AF:
            instruction_push_AF();
            break;
        case OP_PUSH_BC:
            instruction_push_BC();
            break;
        case OP_PUSH_DE:
            instruction_push_DE();
            break;
        case OP_PUSH_HL:
            instruction_push_HL();
            break;
        case OP_POP_AF:
            instruction_pop_AF();
            break;
        case OP_POP_BC:
            instruction_pop_BC();
            break;
        case OP_POP_DE:
            instruction_pop_DE();
            break;
        case OP_POP_HL:
            instruction_pop_HL();
            break;
        case OP_ADD_A:
            instruction_add_A();
            break;
        case OP_ADD_B:
            instruction_add_B();
            break;
        case OP_ADD_C:
            instruction_add_C();
            break;
        case OP_ADD_D:
            instruction_add_D();
            break;
        case OP_ADD_E:
            instruction_add_E();
            break;
        case OP_ADD_H:
            instruction_add_H();
            break;
        case OP_ADD_L:
            instruction_add_L();
            break;
        case OP_ADD_HLI:
            instruction_add_HLI();
            break;
        case OP_ADD_CONST:
            instruction_add_CONST(read_next_byte());
            break;
        case OP_ADC_A:
            instruction_adc_A();
            break;
        case OP_ADC_B:
            instruction_adc_B();
            break;
        case OP_ADC_C:
            instruction_adc_C();
            break;
        case OP_ADC_D:
            instruction_adc_D();
            break;
        case OP_ADC_E:
            instruction_adc_E();
            break;
        case OP_ADC_H:
            instruction_adc_H();
            break;
        case OP_ADC_L:
            instruction_adc_L();
            break;
        case OP_ADC_HLI:
            instruction_adc_HLI();
            break;
        case OP_ADC_CONST:
            instruction_adc_CONST(read_next_byte());
            break;
        case OP_SUB_A:
            instruction_sub_A();
            break;
        case OP_SUB_B:
            instruction_sub_B();
            break;
        case OP_SUB_C:
            instruction_sub_C();
            break;
        case OP_SUB_D:
            instruction_sub_D();
            break;
        case OP_SUB_E:
            instruction_sub_E();
            break;
        case OP_SUB_H:
            instruction_sub_H();
            break;
        case OP_SUB_L:
            instruction_sub_L();
            break;
        case OP_SUB_HLI:
            instruction_sub_HLI();
            break;
        case OP_SUB_CONST:
            instruction_sub_CONST(read_next_byte());
            break;
        case OP_SBC_A:
            instruction_sbc_A();
            break;
        case OP_SBC_B:
            instruction_sbc_B();
            break;
        case OP_SBC_C:
            instruction_sbc_C();
            break;
        case OP_SBC_D:
            instruction_sbc_D();
            break;
        case OP_SBC_E:
            instruction_sbc_E();
            break;
        case OP_SBC_H:
            instruction_sbc_H();
            break;
        case OP_SBC_L:
            instruction_sbc_L();
            break;
        case OP_SBC_HLI:
            instruction_sbc_HLI();
            break;
        case OP_SBC_CONST:
            instruction_sbc_CONST(read_next_byte());
            break;
        case OP_AND_A:
            instruction_and_A();
            break;
        case OP_AND_B:
            instruction_and_B();
            break;
        case OP_AND_C:
            instruction_and_C();
            break;
        case OP_AND_D:
            instruction_and_D();
            break;
        case OP_AND_E:
            instruction_and_E();
            break;
        case OP_AND_H:
            instruction_and_H();
            break;
        case OP_AND_L:
            instruction_and_L();
            break;
        case OP_AND_HLI:
            instruction_and_HLI();
            break;
        case OP_AND_CONST:
            instruction_and_CONST(read_next_byte());
            break;
        case OP_OR_A:
            instruction_or_A();
            break;
        case OP_OR_B:
            instruction_or_B();
            break;
        case OP_OR_C:
            instruction_or_C();
            break;
        case OP_OR_D:
            instruction_or_D();
            break;
        case OP_OR_E:
            instruction_or_E();
            break;
        case OP_OR_H:
            instruction_or_H();
            break;
        case OP_OR_L:
            instruction_or_L();
            break;
        case OP_OR_HLI:
            instruction_or_HLI();
            break;
        case OP_OR_CONST:
            instruction_or_CONST(read_next_byte());
            break;
        case OP_XOR_A:
            instruction_xor_A();
            break;
        case OP_XOR_B:
            instruction_xor_B();
            break;
        case OP_XOR_C:
            instruction_xor_C();
            break;
        case OP_XOR_D:
            instruction_xor_D();
            break;
        case OP_XOR_E:
            instruction_xor_E();
            break;
        case OP_XOR_H:
            instruction_xor_H();
            break;
        case OP_XOR_L:
            instruction_xor_L();
            break;
        case OP_XOR_HLI:
            instruction_xor_HLI();
            break;
        case OP_XOR_CONST:
            instruction_xor_CONST(read_next_byte());
            break;
        case OP_CP_A:
            instruction_cp_A();
            break;
        case OP_CP_B:
            instruction_cp_B();
            break;
        case OP_CP_C:
            instruction_cp_C();
            break;
        case OP_CP_D:
            instruction_cp_D();
            break;
        case OP_CP_E:
            instruction_cp_E();
            break;
        case OP_CP_H:
            instruction_cp_H();
            break;
        case OP_CP_L:
            instruction_cp_L();
            break;
        case OP_CP_HLI:
            instruction_cp_HLI();
            break;
        case OP_CP_CONST:
            instruction_cp_CONST(read_next_byte());
            break;
        case OP_INC_A:
            instruction_inc_A();
            break;
        case OP_INC_B:
            instruction_inc_B();
            break;
        case OP_INC_C:
            instruction_inc_C();
            break;
        case OP_INC_D:
            instruction_inc_D();
            break;
        case OP_INC_E:
            instruction_inc_E();
            break;
        case OP_INC_H:
            instruction_inc_H();
            break;
        case OP_INC_L:
            instruction_inc_L();
            break;
        case OP_INC_HLI:
            instruction_inc_HLI();
            break;
        case OP_DEC_A:
            instruction_dec_A();
            break;
        case OP_DEC_B:
            instruction_dec_B();
            break;
        case OP_DEC_C:
            instruction_dec_C();
            break;
        case OP_DEC_D:
            instruction_dec_D();
            break;
        case OP_DEC_E:
            instruction_dec_E();
            break;
        case OP_DEC_H:
            instruction_dec_H();
            break;
        case OP_DEC_L:
            instruction_dec_L();
            break;
        case OP_DEC_HLI:
            instruction_dec_HLI();
            break;
        case OP_ADD_BC:
            instruction_add_BC();
            break;
        case OP_ADD_DE:
            instruction_add_DE();
            break;
        case OP_ADD_HL:
            instruction_add_HL();
            break;
        case OP_ADD_SP:
            instruction_add_SP();
            break;
        case OP_ADD_SP_CONST:
            nextByte = read_next_byte();
            instruction_add_SP_CONST(reinterpret_cast<int8_t &>(nextByte));
            break;
        case OP_INC_BC:
            instruction_inc_BC();
            break;
        case OP_INC_DE:
            instruction_inc_DE();
            break;
        case OP_INC_HL:
            instruction_inc_HL();
            break;
        case OP_INC_SP:
            instruction_inc_SP();
            break;
        case OP_DEC_BC:
            instruction_dec_BC();
            break;
        case OP_DEC_DE:
            instruction_dec_DE();
            break;
        case OP_DEC_HL:
            instruction_dec_HL();
            break;
        case OP_DEC_SP:
            instruction_dec_SP();
            break;
        //SWAP instructions appear later with other CB prefixes
        case OP_DAA:
            instruction_daa();
            break;
        case OP_CPL:
            instruction_cpl();
            break;
        case OP_CCF:
            instruction_ccf();
            break;
        case OP_SCF:
            instruction_scf();
            break;
        case OP_NOP:
            instruction_nop();
            break;
        case OP_HALT:
            instruction_halt();
            break;
        //Stop is at end since it is a two byte opcode
        case OP_DI:
            instruction_di();
            break;
        case OP_EI:
            instruction_ei();
            break;
        case OP_RLCA:
            instruction_rlca();
            break;
        case OP_RLA:
            instruction_rla();
            break;
        case OP_RRCA:
            instruction_rrca();
            break;
        case OP_RRA:
            instruction_rra();
            break;
        case OP_JP:
            instruction_jp(read_next_word());
            break;
        case OP_JP_NZ:
            instruction_jp_NZ(read_next_word());
            break;
        case OP_JP_Z:
            instruction_jp_Z(read_next_word());
            break;
        case OP_JP_NC:
            instruction_jp_NC(read_next_word());
            break;
        case OP_JP_C:
            instruction_jp_C(read_next_word());
            break;
        case OP_JP_HLI:
            instruction_jp_HLI();
            break;
        case OP_JR:
            nextByte = read_next_byte();
            instruction_jr(reinterpret_cast<int8_t &>(nextByte));
            break;
        case OP_JR_NZ:
            nextByte = read_next_byte();
            instruction_jr_NZ(reinterpret_cast<int8_t &>(nextByte));
            break;
        case OP_JR_Z:
            nextByte = read_next_byte();
            instruction_jr_Z(reinterpret_cast<int8_t &>(nextByte));
            break;
        case OP_JR_NC:
            nextByte = read_next_byte();
            instruction_jr_NC(reinterpret_cast<int8_t &>(nextByte));
            break;
        case OP_JR_C:
            nextByte = read_next_byte();
            instruction_jr_C(reinterpret_cast<int8_t &>(nextByte));
            break;
        case OP_CALL:
            instruction_call(read_next_word());
            break;
        case OP_CALL_NZ:
            instruction_call_NZ(read_next_word());
            break;
        case OP_CALL_Z:
            instruction_call_Z(read_next_word());
            break;
        case OP_CALL_NC:
            instruction_call_NC(read_next_word());
            break;
        case OP_CALL_C:
            instruction_call_C(read_next_word());
            break;
        case OP_RST_00H:
            instruction_rst_00H();
            break;
        case OP_RST_08H:
            instruction_rst_08H();
            break;
        case OP_RST_10H:
            instruction_rst_10H();
            break;
        case OP_RST_18H:
            instruction_rst_18H();
            break;
        case OP_RST_20H:
            instruction_rst_20H();
            break;
        case OP_RST_28H:
            instruction_rst_28H();
            break;
        case OP_RST_30H:
            instruction_rst_30H();
            break;
        case OP_RST_38H:
            instruction_rst_38H();
            break;
        case OP_RET:
            instruction_ret();
            break;
        case OP_RET_NZ:
            instruction_ret_NZ();
            break;
        case OP_RET_Z:
            instruction_ret_Z();
            break;
        case OP_RET_NC:
            instruction_ret_NC();
            break;
        case OP_RET_C:
            instruction_ret_C();
            break;
        case OP_RETI:
            instruction_reti();
            break;
        case OP_STOP8:
            //Reuse cbop variable...
            //TODO - rename cbop with nextOp!
            cbop = read_next_byte();
            if(cbop == 0x00){
                instruction_stop();
            } else {
                std::cout << "A non-zero byte is following the stop byte!!! Value: " << +cbop << std::endl;
                
                //According to https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf
                //Gameboy turns LCD on when invalid stop instruction is hit.
                if(BAD_STOP_IS_LCD_ON){
                    uint8_t lcdc = m_gblcd->getLCDC();
                    m_gblcd->setLCDC(lcdc | LCDC_DISPLAY_ENABLE);
                } 
                
                if(STOP_ON_BAD_STOP){
                    exit(1);
                }
            }
            break;
        case OP_IS_CB_PREFIXED: 
            cbop = setMSB(read_next_byte(), opcode);
            if(CONSOLE_OUTPUT_ENABLED) std::cout << "cbopcode: " << +cbop << "\n";
            switch(cbop){
                case OP_SWAP_A:
                    instruction_swap_A();
                    break;
                case OP_SWAP_B:
                    instruction_swap_B();
                    break;
                case OP_SWAP_C:
                    instruction_swap_C();
                    break;
                case OP_SWAP_D:
                    instruction_swap_D();
                    break;
                case OP_SWAP_E:
                    instruction_swap_E();
                    break;
                case OP_SWAP_H:
                    instruction_swap_H();
                    break;
                case OP_SWAP_L:
                    instruction_swap_L();
                    break;
                case OP_SWAP_HLI:
                    instruction_swap_HLI();
                    break;
                case OP_RLC_A:
                    instruction_rlc_A();
                    break;
                case OP_RLC_B:
                    instruction_rlc_B();
                    break;
                case OP_RLC_C:
                    instruction_rlc_C();
                    break;
                case OP_RLC_D:
                    instruction_rlc_D();
                    break;
                case OP_RLC_E:
                    instruction_rlc_E();
                    break;
                case OP_RLC_H:
                    instruction_rlc_H();
                    break;
                case OP_RLC_L:
                    instruction_rlc_L();
                    break;
                case OP_RLC_HLI:
                    instruction_rlc_HLI();
                    break;
                case OP_RL_A:
                    instruction_rl_A();
                    break;
                case OP_RL_B:
                    instruction_rl_B();
                    break;
                case OP_RL_C:
                    instruction_rl_C();
                    break;
                case OP_RL_D:
                    instruction_rl_D();
                    break;
                case OP_RL_E:
                    instruction_rl_E();
                    break;
                case OP_RL_H:
                    instruction_rl_H();
                    break;
                case OP_RL_L:
                    instruction_rl_L();
                    break;
                case OP_RL_HLI:
                    instruction_rl_HLI();
                    break;
                case OP_RRC_A:
                    instruction_rrc_A();
                    break;
                case OP_RRC_B:
                    instruction_rrc_B();
                    break;
                case OP_RRC_C:
                    instruction_rrc_C();
                    break;
                case OP_RRC_D:
                    instruction_rrc_D();
                    break;
                case OP_RRC_E:
                    instruction_rrc_E();
                    break;
                case OP_RRC_H:
                    instruction_rrc_H();
                    break;
                case OP_RRC_L:
                    instruction_rrc_L();
                    break;
                case OP_RRC_HLI:
                    instruction_rrc_HLI();
                    break;
                case OP_RR_A:
                    instruction_rr_A();
                    break;
                case OP_RR_B:
                    instruction_rr_B();
                    break;
                case OP_RR_C:
                    instruction_rr_C();
                    break;
                case OP_RR_D:
                    instruction_rr_D();
                    break;
                case OP_RR_E:
                    instruction_rr_E();
                    break;
                case OP_RR_H:
                    instruction_rr_H();
                    break;
                case OP_RR_L:
                    instruction_rr_L();
                    break;
                case OP_RR_HLI:
                    instruction_rr_HLI();
                    break;
                case OP_SLA_A:
                    instruction_sla_A();
                    break;
                case OP_SLA_B:
                    instruction_sla_B();
                    break;
                case OP_SLA_C:
                    instruction_sla_C();
                    break;
                case OP_SLA_D:
                    instruction_sla_D();
                    break;
                case OP_SLA_E:
                    instruction_sla_E();
                    break;
                case OP_SLA_H:
                    instruction_sla_H();
                    break;
                case OP_SLA_L:
                    instruction_sla_L();
                    break;
                case OP_SLA_HLI:
                    instruction_sla_HLI();
                    break;
                case OP_SRA_A:
                    instruction_sra_A();
                    break;
                case OP_SRA_B:
                    instruction_sra_B();
                    break;
                case OP_SRA_C:
                    instruction_sra_C();
                    break;
                case OP_SRA_D:
                    instruction_sra_D();
                    break;
                case OP_SRA_E:
                    instruction_sra_E();
                    break;
                case OP_SRA_H:
                    instruction_sra_H();
                    break;
                case OP_SRA_L:
                    instruction_sra_L();
                    break;
                case OP_SRA_HLI:
                    instruction_sra_HLI();
                    break;
                case OP_SRL_A:
                    instruction_srl_A();
                    break;
                case OP_SRL_B:
                    instruction_srl_B();
                    break;
                case OP_SRL_C:
                    instruction_srl_C();
                    break;
                case OP_SRL_D:
                    instruction_srl_D();
                    break;
                case OP_SRL_E:
                    instruction_srl_E();
                    break;
                case OP_SRL_H:
                    instruction_srl_H();
                    break;
                case OP_SRL_L:
                    instruction_srl_L();
                    break;
                case OP_SRL_HLI:
                    instruction_srl_HLI();
                    break;
                case OP_BIT_0_A:
                    instruction_bit_0_A();
                    break;
                case OP_BIT_0_B:
                    instruction_bit_0_B();
                    break;
                case OP_BIT_0_C:
                    instruction_bit_0_C();
                    break;
                case OP_BIT_0_D:
                    instruction_bit_0_D();
                    break;
                case OP_BIT_0_E:
                    instruction_bit_0_E();
                    break;
                case OP_BIT_0_H:
                    instruction_bit_0_H();
                    break;
                case OP_BIT_0_L:
                    instruction_bit_0_L();
                    break;
                case OP_BIT_0_HLI:
                    instruction_bit_0_HLI();
                    break;
                case OP_BIT_1_A:
                    instruction_bit_1_A();
                    break;
                case OP_BIT_1_B:
                    instruction_bit_1_B();
                    break;
                case OP_BIT_1_C:
                    instruction_bit_1_C();
                    break;
                case OP_BIT_1_D:
                    instruction_bit_1_D();
                    break;
                case OP_BIT_1_E:
                    instruction_bit_1_E();
                    break;
                case OP_BIT_1_H:
                    instruction_bit_1_H();
                    break;
                case OP_BIT_1_L:
                    instruction_bit_1_L();
                    break;
                case OP_BIT_1_HLI:
                    instruction_bit_1_HLI();
                    break;
                case OP_BIT_2_A:
                    instruction_bit_2_A();
                    break;
                case OP_BIT_2_B:
                    instruction_bit_2_B();
                    break;
                case OP_BIT_2_C:
                    instruction_bit_2_C();
                    break;
                case OP_BIT_2_D:
                    instruction_bit_2_D();
                    break;
                case OP_BIT_2_E:
                    instruction_bit_2_E();
                    break;
                case OP_BIT_2_H:
                    instruction_bit_2_H();
                    break;
                case OP_BIT_2_L:
                    instruction_bit_2_L();
                    break;
                case OP_BIT_2_HLI:
                    instruction_bit_2_HLI();
                    break;
                case OP_BIT_3_A:
                    instruction_bit_3_A();
                    break;
                case OP_BIT_3_B:
                    instruction_bit_3_B();
                    break;
                case OP_BIT_3_C:
                    instruction_bit_3_C();
                    break;
                case OP_BIT_3_D:
                    instruction_bit_3_D();
                    break;
                case OP_BIT_3_E:
                    instruction_bit_3_E();
                    break;
                case OP_BIT_3_H:
                    instruction_bit_3_H();
                    break;
                case OP_BIT_3_L:
                    instruction_bit_3_L();
                    break;
                case OP_BIT_3_HLI:
                    instruction_bit_3_HLI();
                    break;
                case OP_BIT_4_A:
                    instruction_bit_4_A();
                    break;
                case OP_BIT_4_B:
                    instruction_bit_4_B();
                    break;
                case OP_BIT_4_C:
                    instruction_bit_4_C();
                    break;
                case OP_BIT_4_D:
                    instruction_bit_4_D();
                    break;
                case OP_BIT_4_E:
                    instruction_bit_4_E();
                    break;
                case OP_BIT_4_H:
                    instruction_bit_4_H();
                    break;
                case OP_BIT_4_L:
                    instruction_bit_4_L();
                    break;
                case OP_BIT_4_HLI:
                    instruction_bit_4_HLI();
                    break;
                case OP_BIT_5_A:
                    instruction_bit_5_A();
                    break;
                case OP_BIT_5_B:
                    instruction_bit_5_B();
                    break;
                case OP_BIT_5_C:
                    instruction_bit_5_C();
                    break;
                case OP_BIT_5_D:
                    instruction_bit_5_D();
                    break;
                case OP_BIT_5_E:
                    instruction_bit_5_E();
                    break;
                case OP_BIT_5_H:
                    instruction_bit_5_H();
                    break;
                case OP_BIT_5_L:
                    instruction_bit_5_L();
                    break;
                case OP_BIT_5_HLI:
                    instruction_bit_5_HLI();
                    break;
                case OP_BIT_6_A:
                    instruction_bit_6_A();
                    break;
                case OP_BIT_6_B:
                    instruction_bit_6_B();
                    break;
                case OP_BIT_6_C:
                    instruction_bit_6_C();
                    break;
                case OP_BIT_6_D:
                    instruction_bit_6_D();
                    break;
                case OP_BIT_6_E:
                    instruction_bit_6_E();
                    break;
                case OP_BIT_6_H:
                    instruction_bit_6_H();
                    break;
                case OP_BIT_6_L:
                    instruction_bit_6_L();
                    break;
                case OP_BIT_6_HLI:
                    instruction_bit_6_HLI();
                    break;
                case OP_BIT_7_A:
                    instruction_bit_7_A();
                    break;
                case OP_BIT_7_B:
                    instruction_bit_7_B();
                    break;
                case OP_BIT_7_C:
                    instruction_bit_7_C();
                    break;
                case OP_BIT_7_D:
                    instruction_bit_7_D();
                    break;
                case OP_BIT_7_E:
                    instruction_bit_7_E();
                    break;
                case OP_BIT_7_H:
                    instruction_bit_7_H();
                    break;
                case OP_BIT_7_L:
                    instruction_bit_7_L();
                    break;
                case OP_BIT_7_HLI:
                    instruction_bit_7_HLI();
                    break;
                case OP_SET_0_A:
                    instruction_set_0_A();
                    break;
                case OP_SET_0_B:
                    instruction_set_0_B();
                    break;
                case OP_SET_0_C:
                    instruction_set_0_C();
                    break;
                case OP_SET_0_D:
                    instruction_set_0_D();
                    break;
                case OP_SET_0_E:
                    instruction_set_0_E();
                    break;
                case OP_SET_0_H:
                    instruction_set_0_H();
                    break;
                case OP_SET_0_L:
                    instruction_set_0_L();
                    break;
                case OP_SET_0_HLI:
                    instruction_set_0_HLI();
                    break;
                case OP_SET_1_A:
                    instruction_set_1_A();
                    break;
                case OP_SET_1_B:
                    instruction_set_1_B();
                    break;
                case OP_SET_1_C:
                    instruction_set_1_C();
                    break;
                case OP_SET_1_D:
                    instruction_set_1_D();
                    break;
                case OP_SET_1_E:
                    instruction_set_1_E();
                    break;
                case OP_SET_1_H:
                    instruction_set_1_H();
                    break;
                case OP_SET_1_L:
                    instruction_set_1_L();
                    break;
                case OP_SET_1_HLI:
                    instruction_set_1_HLI();
                    break;
                case OP_SET_2_A:
                    instruction_set_2_A();
                    break;
                case OP_SET_2_B:
                    instruction_set_2_B();
                    break;
                case OP_SET_2_C:
                    instruction_set_2_C();
                    break;
                case OP_SET_2_D:
                    instruction_set_2_D();
                    break;
                case OP_SET_2_E:
                    instruction_set_2_E();
                    break;
                case OP_SET_2_H:
                    instruction_set_2_H();
                    break;
                case OP_SET_2_L:
                    instruction_set_2_L();
                    break;
                case OP_SET_2_HLI:
                    instruction_set_2_HLI();
                    break;
                case OP_SET_3_A:
                    instruction_set_3_A();
                    break;
                case OP_SET_3_B:
                    instruction_set_3_B();
                    break;
                case OP_SET_3_C:
                    instruction_set_3_C();
                    break;
                case OP_SET_3_D:
                    instruction_set_3_D();
                    break;
                case OP_SET_3_E:
                    instruction_set_3_E();
                    break;
                case OP_SET_3_H:
                    instruction_set_3_H();
                    break;
                case OP_SET_3_L:
                    instruction_set_3_L();
                    break;
                case OP_SET_3_HLI:
                    instruction_set_3_HLI();
                    break;
                case OP_SET_4_A:
                    instruction_set_4_A();
                    break;
                case OP_SET_4_B:
                    instruction_set_4_B();
                    break;
                case OP_SET_4_C:
                    instruction_set_4_C();
                    break;
                case OP_SET_4_D:
                    instruction_set_4_D();
                    break;
                case OP_SET_4_E:
                    instruction_set_4_E();
                    break;
                case OP_SET_4_H:
                    instruction_set_4_H();
                    break;
                case OP_SET_4_L:
                    instruction_set_4_L();
                    break;
                case OP_SET_4_HLI:
                    instruction_set_4_HLI();
                    break;
                case OP_SET_5_A:
                    instruction_set_5_A();
                    break;
                case OP_SET_5_B:
                    instruction_set_5_B();
                    break;
                case OP_SET_5_C:
                    instruction_set_5_C();
                    break;
                case OP_SET_5_D:
                    instruction_set_5_D();
                    break;
                case OP_SET_5_E:
                    instruction_set_5_E();
                    break;
                case OP_SET_5_H:
                    instruction_set_5_H();
                    break;
                case OP_SET_5_L:
                    instruction_set_5_L();
                    break;
                case OP_SET_5_HLI:
                    instruction_set_5_HLI();
                    break;
                case OP_SET_6_A:
                    instruction_set_6_A();
                    break;
                case OP_SET_6_B:
                    instruction_set_6_B();
                    break;
                case OP_SET_6_C:
                    instruction_set_6_C();
                    break;
                case OP_SET_6_D:
                    instruction_set_6_D();
                    break;
                case OP_SET_6_E:
                    instruction_set_6_E();
                    break;
                case OP_SET_6_H:
                    instruction_set_6_H();
                    break;
                case OP_SET_6_L:
                    instruction_set_6_L();
                    break;
                case OP_SET_6_HLI:
                    instruction_set_6_HLI();
                    break;
                case OP_SET_7_A:
                    instruction_set_7_A();
                    break;
                case OP_SET_7_B:
                    instruction_set_7_B();
                    break;
                case OP_SET_7_C:
                    instruction_set_7_C();
                    break;
                case OP_SET_7_D:
                    instruction_set_7_D();
                    break;
                case OP_SET_7_E:
                    instruction_set_7_E();
                    break;
                case OP_SET_7_H:
                    instruction_set_7_H();
                    break;
                case OP_SET_7_L:
                    instruction_set_7_L();
                    break;
                case OP_SET_7_HLI:
                    instruction_set_7_HLI();
                    break;
                case OP_RES_0_A:
                    instruction_res_0_A();
                    break;
                case OP_RES_0_B:
                    instruction_res_0_B();
                    break;
                case OP_RES_0_C:
                    instruction_res_0_C();
                    break;
                case OP_RES_0_D:
                    instruction_res_0_D();
                    break;
                case OP_RES_0_E:
                    instruction_res_0_E();
                    break;
                case OP_RES_0_H:
                    instruction_res_0_H();
                    break;
                case OP_RES_0_L:
                    instruction_res_0_L();
                    break;
                case OP_RES_0_HLI:
                    instruction_res_0_HLI();
                    break;
                case OP_RES_1_A:
                    instruction_res_1_A();
                    break;
                case OP_RES_1_B:
                    instruction_res_1_B();
                    break;
                case OP_RES_1_C:
                    instruction_res_1_C();
                    break;
                case OP_RES_1_D:
                    instruction_res_1_D();
                    break;
                case OP_RES_1_E:
                    instruction_res_1_E();
                    break;
                case OP_RES_1_H:
                    instruction_res_1_H();
                    break;
                case OP_RES_1_L:
                    instruction_res_1_L();
                    break;
                case OP_RES_1_HLI:
                    instruction_res_1_HLI();
                    break;
                case OP_RES_2_A:
                    instruction_res_2_A();
                    break;
                case OP_RES_2_B:
                    instruction_res_2_B();
                    break;
                case OP_RES_2_C:
                    instruction_res_2_C();
                    break;
                case OP_RES_2_D:
                    instruction_res_2_D();
                    break;
                case OP_RES_2_E:
                    instruction_res_2_E();
                    break;
                case OP_RES_2_H:
                    instruction_res_2_H();
                    break;
                case OP_RES_2_L:
                    instruction_res_2_L();
                    break;
                case OP_RES_2_HLI:
                    instruction_res_2_HLI();
                    break;
                case OP_RES_3_A:
                    instruction_res_3_A();
                    break;
                case OP_RES_3_B:
                    instruction_res_3_B();
                    break;
                case OP_RES_3_C:
                    instruction_res_3_C();
                    break;
                case OP_RES_3_D:
                    instruction_res_3_D();
                    break;
                case OP_RES_3_E:
                    instruction_res_3_E();
                    break;
                case OP_RES_3_H:
                    instruction_res_3_H();
                    break;
                case OP_RES_3_L:
                    instruction_res_3_L();
                    break;
                case OP_RES_3_HLI:
                    instruction_res_3_HLI();
                    break;
                case OP_RES_4_A:
                    instruction_res_4_A();
                    break;
                case OP_RES_4_B:
                    instruction_res_4_B();
                    break;
                case OP_RES_4_C:
                    instruction_res_4_C();
                    break;
                case OP_RES_4_D:
                    instruction_res_4_D();
                    break;
                case OP_RES_4_E:
                    instruction_res_4_E();
                    break;
                case OP_RES_4_H:
                    instruction_res_4_H();
                    break;
                case OP_RES_4_L:
                    instruction_res_4_L();
                    break;
                case OP_RES_4_HLI:
                    instruction_res_4_HLI();
                    break;
                case OP_RES_5_A:
                    instruction_res_5_A();
                    break;
                case OP_RES_5_B:
                    instruction_res_5_B();
                    break;
                case OP_RES_5_C:
                    instruction_res_5_C();
                    break;
                case OP_RES_5_D:
                    instruction_res_5_D();
                    break;
                case OP_RES_5_E:
                    instruction_res_5_E();
                    break;
                case OP_RES_5_H:
                    instruction_res_5_H();
                    break;
                case OP_RES_5_L:
                    instruction_res_5_L();
                    break;
                case OP_RES_5_HLI:
                    instruction_res_5_HLI();
                    break;
                case OP_RES_6_A:
                    instruction_res_6_A();
                    break;
                case OP_RES_6_B:
                    instruction_res_6_B();
                    break;
                case OP_RES_6_C:
                    instruction_res_6_C();
                    break;
                case OP_RES_6_D:
                    instruction_res_6_D();
                    break;
                case OP_RES_6_E:
                    instruction_res_6_E();
                    break;
                case OP_RES_6_H:
                    instruction_res_6_H();
                    break;
                case OP_RES_6_L:
                    instruction_res_6_L();
                    break;
                case OP_RES_6_HLI:
                    instruction_res_6_HLI();
                    break;
                case OP_RES_7_A:
                    instruction_res_7_A();
                    break;
                case OP_RES_7_B:
                    instruction_res_7_B();
                    break;
                case OP_RES_7_C:
                    instruction_res_7_C();
                    break;
                case OP_RES_7_D:
                    instruction_res_7_D();
                    break;
                case OP_RES_7_E:
                    instruction_res_7_E();
                    break;
                case OP_RES_7_H:
                    instruction_res_7_H();
                    break;
                case OP_RES_7_L:
                    instruction_res_7_L();
                    break;
                case OP_RES_7_HLI:
                    instruction_res_7_HLI();
                    break;
                default:
                 std::cout << std::hex << "Unrecognized CB-prefix opcode " << +cbop << "!!!" << std::endl;
                
                if(STOP_ON_BAD_OPCODE){
                    exit(1);
                }
                instruction_stop();                    
            }
            break;
        default:
             std::cout << std::hex << "Unrecognized opcode " << +opcode << "!!!" << std::endl;
            
            if(STOP_ON_BAD_OPCODE){
                exit(1);
            }
                    
            instruction_stop();
    }
}

//If interrupts are enabled, handles interrupts
void GBZ80::process_interrupts(){
    uint8_t enabledInterrupts = m_gbmemory->direct_read(INTERRUPT_ENABLE);
    uint8_t raisedFlags = m_gbmemory->direct_read(ADDRESS_IF);
    
    //Only process interrupts if they're enabled and 
    if(m_bInterruptsEnabled && (raisedFlags > 0) && (enabledInterrupts > 0)){
        
        bool bPerformInterrupt = false;
        uint16_t interruptAddress = PC;
        uint8_t currentFlag = 0x00;
        
        //Check for interrupt and set address based on priority
        if((enabledInterrupts & INTERRUPT_FLAG_VBLANK) && (raisedFlags & INTERRUPT_FLAG_VBLANK)){
            if(CONSOLE_OUTPUT_ENABLED) std::cout << "VBlank interrupt";
            bPerformInterrupt = true;
            currentFlag = INTERRUPT_FLAG_VBLANK;
            interruptAddress = 0x40;
        } else if ((enabledInterrupts & INTERRUPT_FLAG_STAT) && (raisedFlags & INTERRUPT_FLAG_STAT)){
            if(CONSOLE_OUTPUT_ENABLED) std::cout << "STAT interrupt";
            bPerformInterrupt = true;
            currentFlag = INTERRUPT_FLAG_STAT;
            interruptAddress = 0x48;
        } else if ((enabledInterrupts & INTERRUPT_FLAG_TIMER) && (raisedFlags & INTERRUPT_FLAG_TIMER)){
            if(CONSOLE_OUTPUT_ENABLED) std::cout << "Timer interrupt";
            bPerformInterrupt = true;
            currentFlag = INTERRUPT_FLAG_TIMER;
            interruptAddress = 0x50;
        } else if ((enabledInterrupts & INTERRUPT_FLAG_SERIAL) && (raisedFlags & INTERRUPT_FLAG_SERIAL)){
            if(CONSOLE_OUTPUT_ENABLED) std::cout << "Serial interrupt";
            bPerformInterrupt = true;
            currentFlag = INTERRUPT_FLAG_SERIAL;
            interruptAddress = 0x58;
        } else if ((enabledInterrupts & INTERRUPT_FLAG_JOYPAD) && (raisedFlags & INTERRUPT_FLAG_JOYPAD)){
            if(CONSOLE_OUTPUT_ENABLED) std::cout << "Joypad interrupt";
            bPerformInterrupt = true;
            currentFlag = INTERRUPT_FLAG_JOYPAD;
            interruptAddress = 0x60;
            
            //Joypad also disbales stop
            m_bStop = false;
        } 
        
        //Perform the selected interrupt
        if(bPerformInterrupt){
            //Reset flag. 
            m_gbmemory->direct_write(ADDRESS_IF, raisedFlags & ~currentFlag);
            
            //Disable interrupts - originally true. Was this because of the above comment?
            m_bInterruptsEnabled = false;
            //m_bInterruptsEnabledNext = false;
                        
            //Store next instruction on the stack
            instruction_push_generic(PC);
            
            //Jump to interrupt handler
            PC = interruptAddress;
        }
    }
    
    //Disable halt, even if interrupts are disabled
    if(raisedFlags & enabledInterrupts){
        m_bHalt = false;
    }
}

