#pragma once

#define CYCLES_OP_LD_R_N     8 //Ex. LD B,45
#define CYCLES_OP_LD_R_R     4 //Ex. LD B,C
#define CYCLES_OP_LD_R_RRI   8 //Ex. LD B, (HL) or LD (BC),B
#define CYCLES_OP_LD_HLI_N  12 //Ex. LD (HL),45
#define CYCLES_OP_LD_NNI_R  16 //Ex. LD (45),A
#define CYCLES_OP_LD_FF00CI  8 //Ex. LD A,(C) or LD (0xFF00+C),A
#define CYCLES_OP_LDD        8
#define CYCLES_OP_LDI        8
#define CYCLES_OP_LDH       12
#define CYCLES_OP_LD_RR_NN  12 //Ex. LD BC,45
#define CYCLES_OP_LD_SP_HL   8
#define CYCLES_OP_LDHL      12
#define CYCLES_OP_LD_NNI_SP 20 //Ex. LD (45), SP

#define CYCLES_OP_PUSH      16
#define CYCLES_OP_POP       12

#define CYCLES_OP_ADD_R      4
#define CYCLES_OP_ADD_HLI    8
#define CYCLES_OP_ADD_N      8
#define CYCLES_OP_ADC_R      4
#define CYCLES_OP_ADC_HLI    8
#define CYCLES_OP_ADC_N      8

#define CYCLES_OP_SUB_R      4
#define CYCLES_OP_SUB_HLI    8
#define CYCLES_OP_SUB_N      8
#define CYCLES_OP_SBC_R      4
#define CYCLES_OP_SBC_HLI    8
#define CYCLES_OP_SBC_N      8 //Documentation lists this as ?. Using 8 since this follows pattern for ADC.

#define CYCLES_OP_AND_R      4
#define CYCLES_OP_AND_HLI    8
#define CYCLES_OP_AND_N      8

#define CYCLES_OP_OR_R       4
#define CYCLES_OP_OR_HLI     8
#define CYCLES_OP_OR_N       8

#define CYCLES_XOR_R         4
#define CYCLES_XOR_HLI       8
#define CYCLES_XOR_N         8

#define CYCLES_CP_R          4
#define CYCLES_CP_HLI        8
#define CYCLES_CP_N          8

#define CYCLES_INC_R         4
#define CYCLES_INC_HLI      12

#define CYCLES_DEC_R         4
#define CYCLES_DEC_HLI      12

#define CYCLES_ADD_HL_RR     8 //Ex. ADD HL,BC
#define CYCLES_ADD_SP_N     16 //Ex. ADD SP,45

#define CYCLES_INC_RR        8
#define CYCLES_DEC_RR        8

#define CYCLES_SWAP_R        8
#define CYCLES_SWAP_HLI     16

#define CYCLES_DAA           4
#define CYCLES_CPL           4
#define CYCLES_CCF           4
#define CYCLES_SCF           4
#define CYCLES_NOP           4
#define CYCLES_HALT          4
#define CYCLES_STOP          4
#define CYCLES_DI            4
#define CYCLES_EI            4

#define CYCLES_RLCA          4
#define CYCLES_RLA           4
#define CYCLES_RRCA          4
#define CYCLES_RRA           4
#define CYCLES_RLC_R         8
#define CYCLES_RLC_HLI      16

#define CYCLES_RL_R          8
#define CYCLES_RL_HLI       16

#define CYCLES_RRC_R         8
#define CYCLES_RRC_HLI      16

#define CYCLES_RR_R          8
#define CYCLES_RR_HLI       16

#define CYCLES_SLA_R         8
#define CYCLES_SLA_HLI      16

#define CYCLES_SRA_R         8
#define CYCLES_SRA_HLI      16

#define CYCLES_SRL_R         8
#define CYCLES_SRL_HLI      16

#define CYCLES_BIT_B_R       8
#define CYCLES_BIT_B_HLI    16

#define CYCLES_SET_B_R       8
#define CYCLES_SET_B_HLI     8

#define CYCLES_RES_B_R       8
#define CYCLES_RES_B_HLI    16

#define CYCLES_JP_NN        12 //Jumps to any direct address, including all conditions.
#define CYCLES_JP_HLI        4
#define CYCLES_JR            8

#define CYCLES_CALL         12
#define CYCLES_RST          32
#define CYCLES_RET           8 //Applies to both RET and RETI