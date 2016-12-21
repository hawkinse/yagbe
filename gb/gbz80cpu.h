#pragma once
#include <stdint.h>
#include <vector>
#include "gbmem.h"
#include "gblcd.h"
#include "../IRenderer.h"
#include "../IInputChecker.h"
#include "../constants.h"

//Clock speeds for supported system types
#define CLOCK_GB 4.194304
#define CLOCK_SGB 4.295454
#define CLOCK_GBC CLOCK_GB * 2

#define MHZ_TO_HZ 1000000

#define SINGLE_STEP false

class GBZ80{
  public:
    GBZ80(GBMem* memory, GBLCD* lcd);
    ~GBZ80();
    void init();
    void tick(float deltaTime);

    void setInputChecker(IInputChecker* checker);
    
    //Debug
    void showDebugPrompt();
    void showHelp();
    void setBreakpoint(int line);
    void clearBreakpoint(int line);
    void showMem(uint16_t addr);
    void showMem(uint16_t addr, uint16_t count);
    void showRegisters();
    void step();
    
  private:
    GBMem* m_gbmemory;
    GBLCD* m_gblcd;
    
    //TODO - find a better place for this? Dont think it belongs in the CPU!
    IInputChecker* m_InputChecker;
    
    //Interrupts are enabled when InterruptsEnabled true
    //When InterruptsEnabledNext is different from InterruptsEnabled, InterruptsEnabled will be set to the value in Next AFTER the next instruction.
    bool m_bInterruptsEnabled;
    bool m_bInterruptsEnabledNext; 
    
    //Halt stops execution until next interrupt. Stop stops all execution, period. 
    bool m_bHalt;
    bool m_bStop;
    
    //Debug single step
    bool m_bSingleStep;
    
    //System clock speed
    float m_Clock;
    
    //Registers. Gameboy treats these as combined 16-bit values, but for ease of implementation we store as 8 bit values where possible
    uint16_t AF; //Accumulator (upper) and Flags (lower)
    uint16_t BC; 
    uint16_t DE;
    uint16_t HL;
    uint16_t SP; //Stack Pointer
    uint16_t PC; //Program Counter / Pointer


    //Register access funcs
    uint16_t getRegisterAF();
    void setRegisterAF(uint16_t val);
    uint8_t getRegisterA();
    void setRegisterA(uint8_t val);
    uint8_t getRegisterF();
    void setRegisterF(uint8_t val);

    uint16_t getRegisterBC();
    void setRegisterBC(uint16_t);
    uint8_t getRegisterB();
    void setRegisterB(uint8_t val);
    uint8_t getRegisterC();
    void setRegisterC(uint8_t val);

    uint16_t getRegisterDE();
    void setRegisterDE(uint16_t val);
    uint8_t getRegisterD();
    void setRegisterD(uint8_t val);
    uint8_t getRegisterE();
    void setRegisterE(uint8_t val);
  
    uint16_t getRegisterHL();
    void setRegisterHL(uint16_t val);
    uint8_t getRegisterH();
    void setRegisterH(uint8_t val);
    uint8_t getRegisterL();
    void setRegisterL(uint8_t val);
  
    uint16_t getRegisterSP();
    void setRegisterSP(uint16_t val);

    uint16_t getRegisterPC();
    void setRegisterPC(uint16_t val);
    
    //Flag funcs (lower 8 bits of AF register)
    bool getFlag_Z(); //Zero flag, bit 7
    void setFlag_Z(bool val); //?
    bool getFlag_N(); //Add/Sub flag (BCD), bit 6
    void setFlag_N(bool val);
    bool getFlag_H(); //Half Carry flag (BCD), bit 5
    void setFlag_H(bool val);
    bool getFlag_C(); //Carry Flag, bit 4
    void setFlag_C(bool val); //?
    //Bits 3 through 0 are unused
 
    //8-bit load instructions
  
    //LD value into register
    void instruction_ld_B(uint8_t n);
    void instruction_ld_C(uint8_t n);
    void instruction_ld_D(uint8_t n);
    void instruction_ld_E(uint8_t n);
    void instruction_ld_H(uint8_t n);
    void instruction_ld_L(uint8_t n);

    //LD register into register
    void instruction_ld_A_A();
    void instruction_ld_A_B();
    void instruction_ld_A_C();
    void instruction_ld_A_D();
    void instruction_ld_A_E();
    void instruction_ld_A_H();
    void instruction_ld_A_L();
    void instruction_ld_A_HLI();
    void instruction_ld_B_A();
    void instruction_ld_B_B();
    void instruction_ld_B_C();
    void instruction_ld_B_D();
    void instruction_ld_B_E();
    void instruction_ld_B_H();
    void instruction_ld_B_L();
    void instruction_ld_B_HLI();
    void instruction_ld_C_A();
    void instruction_ld_C_B();
    void instruction_ld_C_C();
    void instruction_ld_C_D();
    void instruction_ld_C_E();
    void instruction_ld_C_H();
    void instruction_ld_C_L();
    void instruction_ld_C_HLI();
    void instruction_ld_D_A();
    void instruction_ld_D_B();
    void instruction_ld_D_C();
    void instruction_ld_D_D();
    void instruction_ld_D_E();
    void instruction_ld_D_H();
    void instruction_ld_D_L();
    void instruction_ld_D_HLI();
    void instruction_ld_E_A();
    void instruction_ld_E_B();
    void instruction_ld_E_C();
    void instruction_ld_E_D();
    void instruction_ld_E_E();
    void instruction_ld_E_H();
    void instruction_ld_E_L();
    void instruction_ld_E_HLI();
    void instruction_ld_H_A();
    void instruction_ld_H_B();
    void instruction_ld_H_C();
    void instruction_ld_H_D();
    void instruction_ld_H_E();
    void instruction_ld_H_H();
    void instruction_ld_H_L();
    void instruction_ld_H_HLI();
    void instruction_ld_L_A();
    void instruction_ld_L_B();
    void instruction_ld_L_C();
    void instruction_ld_L_D();
    void instruction_ld_L_E();
    void instruction_ld_L_H();
    void instruction_ld_L_L();
    void instruction_ld_L_HLI();
    void instruction_ld_HLI_A();
    void instruction_ld_HLI_B();
    void instruction_ld_HLI_C();
    void instruction_ld_HLI_D();
    void instruction_ld_HLI_E();
    void instruction_ld_HLI_H();
    void instruction_ld_HLI_L();
    void instruction_ld_HLI_N(uint8_t n);

    void instruction_ld_A_BCI();
    void instruction_ld_A_DEI();
    void instruction_ld_A_NNI(uint16_t nn);
    //Value is next byte, need to increment SP? Implied by https://www.reddit.com/r/EmuDev/comments/4rs2zl/gameboy_load_into_a/?st=itdi53kk&sh=09e4c743
    void instruction_ld_A_CONST(uint8_t val);//Size of const?
    void instruction_ld_BCI_A();
    void instruction_ld_DEI_A();
    void instruction_ld_NNI_A(uint16_t nn);

    //LD with memory address $FF00 + register C into A
    void instruction_ld_A_FF00CI();
    //LD with A into memory address $FF00 + register C
    void instruction_ld_FF00CI_A();

    //Put val at address in HL into A, decrement HL
    void instruction_ldd_A_HLI();
    //Put val in A into memory address of AL, decrement HL
    void instruction_ldd_HLI_A();
    //Put val at address HL into A, increment HL
    void instruction_ldi_A_HLI();
    //Put val in A into memory address of AL, increment HL
    void instruction_ldi_HLI_A();

    //Put A into memory address $FF00+n
    void instruction_ldh_FF00NI_A(uint8_t n);
    //Put memory address $FF00+n into A
    void instruction_ldh_A_FF00NI(uint8_t n);

    //16-bit load ops

    void instruction_ld_BC_NN(uint16_t val);
    void instruction_ld_DE_NN(uint16_t val);
    void instruction_ld_HL_NN(uint16_t val);
    void instruction_ld_SP_NN(uint16_t val);

    //Put HL into SP
    void instruction_ld_SP_HL();
    //Put address of SP+n into HL
    void instruction_ldhl(int8_t n);

    //Put SP into address n
    void instruction_ld_NNI_SP(uint16_t nn);

    //Generic push.
    void instruction_push_generic(uint16_t val);
    
    //Push specified register (two bytes) onto stack. Decrement SP twice.
    void instruction_push_AF();
    void instruction_push_BC();
    void instruction_push_DE();
    void instruction_push_HL();

    //Generic pop
    uint16_t instruction_pop_generic();
    //Pop two bytes off stack and store in specified register. Increment SP twice
    void instruction_pop_AF();
    void instruction_pop_BC();
    void instruction_pop_DE();
    void instruction_pop_HL();

    //8 Bit ALU

    //Add the specified register to A
    void instruction_add_generic(uint8_t val);
    void instruction_add_A();
    void instruction_add_B();
    void instruction_add_C();
    void instruction_add_D();
    void instruction_add_E();
    void instruction_add_H();
    void instruction_add_L();
    void instruction_add_HLI();
    void instruction_add_CONST(uint8_t val);//Size of const?

    //Add register value + carry flag to A
    void instruction_adc_generic(uint8_t val);
    void instruction_adc_A();
    void instruction_adc_B();
    void instruction_adc_C();
    void instruction_adc_D();
    void instruction_adc_E();
    void instruction_adc_H();
    void instruction_adc_L();
    void instruction_adc_HLI();
    void instruction_adc_CONST(uint8_t val); //Size of const?

    //Sub the specified register from A
    void instruction_sub_generic(uint8_t val);
    void instruction_sub_A();
    void instruction_sub_B();
    void instruction_sub_C();
    void instruction_sub_D();
    void instruction_sub_E();
    void instruction_sub_H();
    void instruction_sub_L();
    void instruction_sub_HLI();
    void instruction_sub_CONST(uint8_t val);

    //Sub register + carry flag from A
    void instruction_sbc_generic(uint8_t val);
    void instruction_sbc_A();
    void instruction_sbc_B();
    void instruction_sbc_C();
    void instruction_sbc_D();
    void instruction_sbc_E();
    void instruction_sbc_H();
    void instruction_sbc_L();
    void instruction_sbc_HLI();
    void instruction_sbc_CONST(uint8_t val);

    //Logically AND two registers, store in A
    uint8_t instruction_and_generic(uint8_t r1, uint8_t r2);    
    void instruction_and_A();
    void instruction_and_B();
    void instruction_and_C();
    void instruction_and_D();
    void instruction_and_E();
    void instruction_and_H();
    void instruction_and_L();
    void instruction_and_HLI();
    void instruction_and_CONST(uint8_t val);

    //Logically OR registers, store in register A
    uint8_t instruction_or_generic(uint8_t r1, uint8_t r2);
    void instruction_or_A();
    void instruction_or_B();
    void instruction_or_C();
    void instruction_or_D();
    void instruction_or_E();
    void instruction_or_H();
    void instruction_or_L();
    void instruction_or_HLI();
    void instruction_or_CONST(uint8_t val);

    //Logically XOR registers, store result in A
    uint8_t instruction_xor_generic(uint8_t r1, uint8_t r2);
    void instruction_xor_A();
    void instruction_xor_B();
    void instruction_xor_C();
    void instruction_xor_D();
    void instruction_xor_E();
    void instruction_xor_H();
    void instruction_xor_L();
    void instruction_xor_HLI();
    void instruction_xor_CONST(uint8_t val); //Not sure if actually constant. Listed as * in documentation, assuming typo

    //Compare A with register. Basically A - register without storing result
    void instruction_cp_generic(uint8_t val);
    void instruction_cp_A();
    void instruction_cp_B();
    void instruction_cp_C();
    void instruction_cp_D();
    void instruction_cp_E();
    void instruction_cp_H();
    void instruction_cp_L();
    void instruction_cp_HLI();
    void instruction_cp_CONST(uint8_t val);

    //Increment register
    uint8_t instruction_inc_generic(uint8_t val);
    void instruction_inc_A();
    void instruction_inc_B();
    void instruction_inc_C();
    void instruction_inc_D();
    void instruction_inc_E();
    void instruction_inc_H();
    void instruction_inc_L();
    void instruction_inc_HLI();

    //Decrement register
    uint8_t instruction_dec_generic(uint8_t val);
    void instruction_dec_A();
    void instruction_dec_B();
    void instruction_dec_C();
    void instruction_dec_D();
    void instruction_dec_E();
    void instruction_dec_H();
    void instruction_dec_L();
    void instruction_dec_HLI();

    //16 Bit arithmetic

    //Add register to HL directly
    void instruction_add_generic(uint16_t val);
    void instruction_add_BC();
    void instruction_add_DE();
    void instruction_add_HL();
    void instruction_add_SP();

    //Add value to SP
    void instruction_add_SP_CONST(int8_t val);

    //Increment register
    void instruction_inc_BC();
    void instruction_inc_DE();
    void instruction_inc_HL();
    void instruction_inc_SP();

    //Decrement register
    void instruction_dec_BC();
    void instruction_dec_DE();
    void instruction_dec_HL();
    void instruction_dec_SP();

    //Swap upper and lower nibbles of register
    uint8_t instruction_swap_generic(uint8_t val);
    void instruction_swap_A();
    void instruction_swap_B();
    void instruction_swap_C();
    void instruction_swap_D();
    void instruction_swap_E();
    void instruction_swap_H();
    void instruction_swap_L();
    void instruction_swap_HLI();

    //Adjusts register A to contain correct representation of Binary Coded Decimal
    void instruction_daa();

    //Compliments register A (flips all bits)
    void instruction_cpl();
  
    //Compliments/Flips C(arry) flag
    void instruction_ccf();
  
    //Sets the Cary flag to true.
    void instruction_scf();

    //No op
    void instruction_nop();
  
    //Powers down CPU until an interupt occures
    void instruction_halt();
  
    //Halt CPU and LCD until button press
    void instruction_stop();
  
    //Disable interrupts after next instruction
    void instruction_di();
  
    //Enable interrupts after next instruction
    void instruction_ei();

    //Rotates and shifts

    //Generic rotate left. Old bit 7 is stored in Carry flag AND as bit 0
    uint8_t instruction_rlc_generic(uint8_t val);
    
    //Generic rotate left through carry flag. Old bit 7 is stored in Carry flag. Bit 0 set to old Carry flag value
    uint8_t instruction_rl_generic(uint8_t val);
    
    //Generic rotate right. Old bit 0 is stored in Carry flag AND as bit 7
    uint8_t instruction_rrc_generic(uint8_t val);
    
    //Generic rotate right through carry flag. Old bit 0 is stored in Carry. Bit 7 set to old Carry value
    uint8_t instruction_rr_generic(uint8_t val);

    //Rotate A left. Old bit 7 is stored in Carry flag
    void instruction_rlca();
  
    //Rotate A left through carry flag (How is this different from rlca?)
    void instruction_rla();
  
    //Rotate A right. Old bit 0 is stored in Carry flag
    void instruction_rrca();
  
    //Rotate A right through carry flag (How is this different from rrca?)
    void instruction_rra();

    //Rotate register left. Old bit 7 stored in Carry flag
    void instruction_rlc_A();
    void instruction_rlc_B();
    void instruction_rlc_C();
    void instruction_rlc_D();
    void instruction_rlc_E();
    void instruction_rlc_H();
    void instruction_rlc_L();
    void instruction_rlc_HLI();

    //Rotate register left through Carry flag. (How is this different from rlc?)
    void instruction_rl_A();
    void instruction_rl_B();
    void instruction_rl_C();
    void instruction_rl_D();
    void instruction_rl_E();
    void instruction_rl_H();
    void instruction_rl_L();
    void instruction_rl_HLI();

    //Rotate register right. Old bit 0 stored in Carry flag
    void instruction_rrc_A();
    void instruction_rrc_B();
    void instruction_rrc_C();
    void instruction_rrc_D();
    void instruction_rrc_E();
    void instruction_rrc_H();
    void instruction_rrc_L();
    void instruction_rrc_HLI();

    //Rotate register right through Carry flag (How is this different form rrc?)
    void instruction_rr_A();
    void instruction_rr_B();
    void instruction_rr_C();
    void instruction_rr_D();
    void instruction_rr_E();
    void instruction_rr_H();
    void instruction_rr_L();
    void instruction_rr_HLI();

    //Shift register left into Carry. LSB of register set to 0
    uint8_t instruction_sla_generic(uint8_t val);
    void instruction_sla_A();
    void instruction_sla_B();
    void instruction_sla_C();
    void instruction_sla_D();
    void instruction_sla_E();
    void instruction_sla_H();
    void instruction_sla_L();
    void instruction_sla_HLI();

    //Shift register right into carry. MSB doesn't change
    uint8_t instruction_sra_generic(uint8_t val);
    void instruction_sra_A();
    void instruction_sra_B();
    void instruction_sra_C();
    void instruction_sra_D();
    void instruction_sra_E();
    void instruction_sra_H();
    void instruction_sra_L();
    void instruction_sra_HLI();
  
    //Shift register left into Carry. MSB of register set to 0
    uint8_t instruction_srl_generic(uint8_t val);
    void instruction_srl_A();
    void instruction_srl_B();
    void instruction_srl_C();
    void instruction_srl_D();
    void instruction_srl_E();
    void instruction_srl_H();
    void instruction_srl_L();
    void instruction_srl_HLI();


    //Bit instructions

    //Test the given bit of register
    bool instruction_bit_generic(uint8_t bit, uint8_t val);
    void instruction_bit_0_A();
    void instruction_bit_0_B();
    void instruction_bit_0_C();
    void instruction_bit_0_D();
    void instruction_bit_0_E();
    void instruction_bit_0_H();
    void instruction_bit_0_L();
    void instruction_bit_0_HLI();
    void instruction_bit_1_A();
    void instruction_bit_1_B();
    void instruction_bit_1_C();
    void instruction_bit_1_D();
    void instruction_bit_1_E();
    void instruction_bit_1_H();
    void instruction_bit_1_L();
    void instruction_bit_1_HLI();
    void instruction_bit_2_A();
    void instruction_bit_2_B();
    void instruction_bit_2_C();
    void instruction_bit_2_D();
    void instruction_bit_2_E();
    void instruction_bit_2_H();
    void instruction_bit_2_L();
    void instruction_bit_2_HLI();
    void instruction_bit_3_A();
    void instruction_bit_3_B();
    void instruction_bit_3_C();
    void instruction_bit_3_D();
    void instruction_bit_3_E();
    void instruction_bit_3_H();
    void instruction_bit_3_L();
    void instruction_bit_3_HLI();
    void instruction_bit_4_A();
    void instruction_bit_4_B();
    void instruction_bit_4_C();
    void instruction_bit_4_D();
    void instruction_bit_4_E();
    void instruction_bit_4_H();
    void instruction_bit_4_L();
    void instruction_bit_4_HLI();
    void instruction_bit_5_A();
    void instruction_bit_5_B();
    void instruction_bit_5_C();
    void instruction_bit_5_D();
    void instruction_bit_5_E();
    void instruction_bit_5_H();
    void instruction_bit_5_L();
    void instruction_bit_5_HLI();
    void instruction_bit_6_A();
    void instruction_bit_6_B();
    void instruction_bit_6_C();
    void instruction_bit_6_D();
    void instruction_bit_6_E();
    void instruction_bit_6_H();
    void instruction_bit_6_L();
    void instruction_bit_6_HLI();
    void instruction_bit_7_A();
    void instruction_bit_7_B();
    void instruction_bit_7_C();
    void instruction_bit_7_D();
    void instruction_bit_7_E();
    void instruction_bit_7_H();
    void instruction_bit_7_L();
    void instruction_bit_7_HLI();
    
    //Set the given bit of register to 1
    uint8_t instruction_set_generic(uint8_t bit, uint8_t val);
    void instruction_set_0_A();
    void instruction_set_0_B();
    void instruction_set_0_C();
    void instruction_set_0_D();
    void instruction_set_0_E();
    void instruction_set_0_H();
    void instruction_set_0_L();
    void instruction_set_0_HLI();
    void instruction_set_1_A();
    void instruction_set_1_B();
    void instruction_set_1_C();
    void instruction_set_1_D();
    void instruction_set_1_E();
    void instruction_set_1_H();
    void instruction_set_1_L();
    void instruction_set_1_HLI();
    void instruction_set_2_A();
    void instruction_set_2_B();
    void instruction_set_2_C();
    void instruction_set_2_D();
    void instruction_set_2_E();
    void instruction_set_2_H();
    void instruction_set_2_L();
    void instruction_set_2_HLI();
    void instruction_set_3_A();
    void instruction_set_3_B();
    void instruction_set_3_C();
    void instruction_set_3_D();
    void instruction_set_3_E();
    void instruction_set_3_H();
    void instruction_set_3_L();
    void instruction_set_3_HLI();
    void instruction_set_4_A();
    void instruction_set_4_B();
    void instruction_set_4_C();
    void instruction_set_4_D();
    void instruction_set_4_E();
    void instruction_set_4_H();
    void instruction_set_4_L();
    void instruction_set_4_HLI();
    void instruction_set_5_A();
    void instruction_set_5_B();
    void instruction_set_5_C();
    void instruction_set_5_D();
    void instruction_set_5_E();
    void instruction_set_5_H();
    void instruction_set_5_L();
    void instruction_set_5_HLI();
    void instruction_set_6_A();
    void instruction_set_6_B();
    void instruction_set_6_C();
    void instruction_set_6_D();
    void instruction_set_6_E();
    void instruction_set_6_H();
    void instruction_set_6_L();
    void instruction_set_6_HLI();
    void instruction_set_7_A();
    void instruction_set_7_B();
    void instruction_set_7_C();
    void instruction_set_7_D();
    void instruction_set_7_E();
    void instruction_set_7_H();
    void instruction_set_7_L();
    void instruction_set_7_HLI();

    //Reset the given bit of register to 0
    uint8_t instruction_res_generic(uint8_t bit, uint8_t val);
    void instruction_res_0_A();
    void instruction_res_0_B();
    void instruction_res_0_C();
    void instruction_res_0_D();
    void instruction_res_0_E();
    void instruction_res_0_H();
    void instruction_res_0_L();
    void instruction_res_0_HLI();
    void instruction_res_1_A();
    void instruction_res_1_B();
    void instruction_res_1_C();
    void instruction_res_1_D();
    void instruction_res_1_E();
    void instruction_res_1_H();
    void instruction_res_1_L();
    void instruction_res_1_HLI();
    void instruction_res_2_A();
    void instruction_res_2_B();
    void instruction_res_2_C();
    void instruction_res_2_D();
    void instruction_res_2_E();
    void instruction_res_2_H();
    void instruction_res_2_L();
    void instruction_res_2_HLI();
    void instruction_res_3_A();
    void instruction_res_3_B();
    void instruction_res_3_C();
    void instruction_res_3_D();
    void instruction_res_3_E();
    void instruction_res_3_H();
    void instruction_res_3_L();
    void instruction_res_3_HLI();
    void instruction_res_4_A();
    void instruction_res_4_B();
    void instruction_res_4_C();
    void instruction_res_4_D();
    void instruction_res_4_E();
    void instruction_res_4_H();
    void instruction_res_4_L();
    void instruction_res_4_HLI();
    void instruction_res_5_A();
    void instruction_res_5_B();
    void instruction_res_5_C();
    void instruction_res_5_D();
    void instruction_res_5_E();
    void instruction_res_5_H();
    void instruction_res_5_L();
    void instruction_res_5_HLI();
    void instruction_res_6_A();
    void instruction_res_6_B();
    void instruction_res_6_C();
    void instruction_res_6_D();
    void instruction_res_6_E();
    void instruction_res_6_H();
    void instruction_res_6_L();
    void instruction_res_6_HLI();
    void instruction_res_7_A();
    void instruction_res_7_B();
    void instruction_res_7_C();
    void instruction_res_7_D();
    void instruction_res_7_E();
    void instruction_res_7_H();
    void instruction_res_7_L();
    void instruction_res_7_HLI();
    
    //Jump instructions

    //Jumps to the given address
    void instruction_jp(uint16_t nn);

    //Jumps to the given address if NZ
    void instruction_jp_NZ(uint16_t nn);
  
    //Jumps to the given address if Z
    void instruction_jp_Z(uint16_t nn);
  
    //Jumps to the given address if NC
    void instruction_jp_NC(uint16_t nn);
  
    //Jumps to the given address if C
    void instruction_jp_C(uint16_t nn);
  
    //Jumps to address contained in HL
    void instruction_jp_HLI();

    //Adds n to current address and jumps to it
    void instruction_jr(int8_t n);

    //Adds n to current address and jumps to it if NZ
    void instruction_jr_NZ(int8_t n);
  
    //Adds n to current address and jumps to it if Z
    void instruction_jr_Z(int8_t n);
  
    //Adds n to current address and jumps to it if NC
    void instruction_jr_NC(int8_t n);
  
    //Adds n to current address and jumps to it if C
    void instruction_jr_C(int8_t n);

    //Call instructions

    //Push address of next instruction onto stack and jump to address nn
    void instruction_call(uint16_t nn);

    //Calls address nn if NZ
    void instruction_call_NZ(uint16_t nn);
  
    //Calls address nn if Z
    void instruction_call_Z(uint16_t nn);
  
    //Calls address nn if NC
    void instruction_call_NC(uint16_t nn);
  
    //Calls address nn if C
    void instruction_call_C(uint16_t nn);

    //Restart instructions

    //Push present address onto stack and jump to $0000 + n
    void instruction_rst_00H();
    void instruction_rst_08H();
    void instruction_rst_10H();
    void instruction_rst_18H();
    void instruction_rst_20H();
    void instruction_rst_28H();
    void instruction_rst_30H();
    void instruction_rst_38H();

    //Return instructions

    //Pop two bytes from stack and jump to that address
    void instruction_ret();

    //Pop two bytes from stack and jump to that address if NZ
    void instruction_ret_NZ();
  
    //Pop two bytes from stack and jump to that address if Z
    void instruction_ret_Z();
  
    //Pop two bytes from stack and jump to that address if NC
    void instruction_ret_NC();
  
    //Pop two bytes from stack and jump to that address if C
    void instruction_ret_C();
  
    //Pop two bytes from stack, jump to that address, then enable interrupts (Immediately? Or after one instruction?)
    void instruction_reti();
    
    //Execution functions
    
    //Grabs the next two bytes and increments PC
    uint8_t read_next_byte();
    
    //Grabs the next two bytes and increments PC twice
    uint16_t read_next_word();
    
    //Returns the cycle length for the passed in opcode
    int get_cycle_length(uint8_t opcode);
    
    //Calls the appropriate function for the passed in opcode
    void process_opcode(uint8_t opcode);
    
    //If interrupts are enabled, handles interrupts
    void process_interrupts();
};
