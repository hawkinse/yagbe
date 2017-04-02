#pragma once

//Opcodes from GBCPUman.pdf
//I suffix indicates an address or indirect access, denoted in documentation as (), ex. HLI is (HL)

//8-bit load ops

//LD Register,8-bit val
#define OP_LD_B 0x06
#define OP_LD_C 0x0E
#define OP_LD_D 0x16
#define OP_LD_E 0x1E
#define OP_LD_H 0x26
#define OP_LD_L 0x2E

//LD Register,Register
#define OP_LD_A_A   0x7F
#define OP_LD_A_B   0x78
#define OP_LD_A_C   0x79
#define OP_LD_A_D   0x7A
#define OP_LD_A_E   0x7B
#define OP_LD_A_H   0x7C
#define OP_LD_A_L   0x7D
#define OP_LD_A_HLI 0x7E
#define OP_LD_B_A   0x47
#define OP_LD_B_B   0x40
#define OP_LD_B_C   0x41
#define OP_LD_B_D   0x42
#define OP_LD_B_E   0x43
#define OP_LD_B_H   0x44
#define OP_LD_B_L   0x45
#define OP_LD_B_HLI 0x46
#define OP_LD_C_A   0x4F
#define OP_LD_C_B   0x48
#define OP_LD_C_C   0x49
#define OP_LD_C_D   0x4A
#define OP_LD_C_E   0x4B
#define OP_LD_C_H   0x4C
#define OP_LD_C_L   0x4D
#define OP_LD_C_HLI 0x4E
#define OP_LD_D_A   0x57
#define OP_LD_D_B   0x50
#define OP_LD_D_C   0x51
#define OP_LD_D_D   0x52
#define OP_LD_D_E   0x53
#define OP_LD_D_H   0x54
#define OP_LD_D_L   0x55
#define OP_LD_D_HLI 0x56
#define OP_LD_E_A   0x5F
#define OP_LD_E_B   0x58
#define OP_LD_E_C   0x59
#define OP_LD_E_D   0x5A
#define OP_LD_E_E   0x5B
#define OP_LD_E_H   0x5C
#define OP_LD_E_L   0x5D
#define OP_LD_E_HLI 0x5E
#define OP_LD_H_A   0x67
#define OP_LD_H_B   0x60
#define OP_LD_H_C   0x61
#define OP_LD_H_D   0x62
#define OP_LD_H_E   0x63
#define OP_LD_H_H   0x64
#define OP_LD_H_L   0x65
#define OP_LD_H_HLI 0x66
#define OP_LD_L_A   0x6F
#define OP_LD_L_B   0x68
#define OP_LD_L_C   0x69
#define OP_LD_L_D   0x6A
#define OP_LD_L_E   0x6B
#define OP_LD_L_H   0x6C
#define OP_LD_L_L   0x6D
#define OP_LD_L_HLI 0x6E
#define OP_LD_HLI_A 0x77
#define OP_LD_HLI_B 0x70
#define OP_LD_HLI_C 0x71
#define OP_LD_HLI_D 0x72
#define OP_LD_HLI_E 0x73
#define OP_LD_HLI_H 0x74
#define OP_LD_HLI_L 0x75
#define OP_LD_HLI_N 0x36

#define OP_LD_A_BCI     0x0A
#define OP_LD_A_DEI     0x1A
#define OP_LD_A_NNI     0xFA
#define OP_LD_A_CONST   0x3E
#define OP_LD_BCI_A     0x02
#define OP_LD_DEI_A     0x12
#define OP_LD_NNI_A     0xEA

#define OP_LD_A_FF00CI  0xF2
#define OP_LD_FF00CI_A  0xE2

#define OP_LDD_A_HLI    0x3A
#define OP_LDD_HLI_A    0x32
#define OP_LDI_A_HLI    0x2A
#define OP_LDI_HLI_A    0x22

#define OP_LDH_FF00NI_A 0xE0
#define OP_LDH_A_FF00NI 0xF0

//16-bit load ops

#define OP_LD_BC_NN  0x01
#define OP_LD_DE_NN  0x11
#define OP_LD_HL_NN  0x21
#define OP_LD_SP_NN  0x31

#define OP_LD_SP_HL  0xF9 /*SP is listed as indirect, but documentation wording implies direct?*/
#define OP_LDHL_SP_N 0xF8

#define OP_LD_NNI_SP 0x08

#define OP_PUSH_AF 0xF5
#define OP_PUSH_BC 0xC5
#define OP_PUSH_DE 0xD5
#define OP_PUSH_HL 0xE5

#define OP_POP_AF 0xF1
#define OP_POP_BC 0xC1
#define OP_POP_DE 0xD1
#define OP_POP_HL 0xE1

//8 Bit ALU

#define OP_ADD_A     0x87
#define OP_ADD_B     0x80
#define OP_ADD_C     0x81
#define OP_ADD_D     0x82
#define OP_ADD_E     0x83
#define OP_ADD_H     0x84
#define OP_ADD_L     0x85
#define OP_ADD_HLI   0x86
#define OP_ADD_CONST 0xC6

#define OP_ADC_A     0x8F
#define OP_ADC_B     0x88
#define OP_ADC_C     0x89
#define OP_ADC_D     0x8A
#define OP_ADC_E     0x8B
#define OP_ADC_H     0x8C
#define OP_ADC_L     0x8D
#define OP_ADC_HLI   0x8E
#define OP_ADC_CONST 0xCE

#define OP_SUB_A     0x97
#define OP_SUB_B     0x90
#define OP_SUB_C     0x91
#define OP_SUB_D     0x92
#define OP_SUB_E     0x93
#define OP_SUB_H     0x94
#define OP_SUB_L     0x95
#define OP_SUB_HLI   0x96
#define OP_SUB_CONST 0xD6

#define OP_SBC_A     0x9F
#define OP_SBC_B     0x98
#define OP_SBC_C     0x99
#define OP_SBC_D     0x9A
#define OP_SBC_E     0x9B
#define OP_SBC_H     0x9C
#define OP_SBC_L     0x9D
#define OP_SBC_HLI   0x9E
#define OP_SBC_CONST 0xDE

#define OP_AND_A     0xA7
#define OP_AND_B     0xA0
#define OP_AND_C     0xA1
#define OP_AND_D     0xA2
#define OP_AND_E     0xA3
#define OP_AND_H     0xA4
#define OP_AND_L     0xA5
#define OP_AND_HLI   0xA6
#define OP_AND_CONST 0xE6

#define OP_OR_A     0xB7
#define OP_OR_B     0xB0
#define OP_OR_C     0xB1
#define OP_OR_D     0xB2
#define OP_OR_E     0xB3
#define OP_OR_H     0xB4
#define OP_OR_L     0xB5
#define OP_OR_HLI   0xB6
#define OP_OR_CONST 0xF6

#define OP_XOR_A     0xAF
#define OP_XOR_B     0xA8
#define OP_XOR_C     0xA9
#define OP_XOR_D     0xAA
#define OP_XOR_E     0xAB
#define OP_XOR_H     0xAC
#define OP_XOR_L     0xAD
#define OP_XOR_HLI   0xAE
#define OP_XOR_CONST 0xEE /*Listed as asterisk, not pound in doc. Assuming typo*/

#define OP_CP_A     0xBF
#define OP_CP_B     0xB8
#define OP_CP_C     0xB9
#define OP_CP_D     0xBA
#define OP_CP_E     0xBB
#define OP_CP_H     0xBC
#define OP_CP_L     0xBD
#define OP_CP_HLI   0xBE
#define OP_CP_CONST 0xFE

#define OP_INC_A   0x3C
#define OP_INC_B   0x04
#define OP_INC_C   0x0C
#define OP_INC_D   0x14
#define OP_INC_E   0x1C
#define OP_INC_H   0x24
#define OP_INC_L   0x2C
#define OP_INC_HLI 0x34

#define OP_DEC_A   0x3D
#define OP_DEC_B   0x05
#define OP_DEC_C   0x0D
#define OP_DEC_D   0x15
#define OP_DEC_E   0x1D
#define OP_DEC_H   0x25
#define OP_DEC_L   0x2D
#define OP_DEC_HLI 0x35

//16 Bit arithmetic

//These ADDs occure to HL
#define OP_ADD_BC 0x09
#define OP_ADD_DE 0x19
#define OP_ADD_HL 0x29 /*Not address in HL! 16-bit, not 8!*/
#define OP_ADD_SP 0x39

#define OP_ADD_SP_CONST 0xE8 /*Separate since applies to SP, not HL like other 16-bit ADD*/

#define OP_INC_BC 0x03
#define OP_INC_DE 0x13
#define OP_INC_HL 0x23
#define OP_INC_SP 0x33

#define OP_DEC_BC 0x0B
#define OP_DEC_DE 0x1B
#define OP_DEC_HL 0x2B
#define OP_DEC_SP 0x3B

//Swap instructions listed CB with op code. Significant?
#define OP_SWAP_A   0xCB37
#define OP_SWAP_B   0xCB30
#define OP_SWAP_C   0xCB31
#define OP_SWAP_D   0xCB32
#define OP_SWAP_E   0xCB33
#define OP_SWAP_H   0xCB34
#define OP_SWAP_L   0xCB35
#define OP_SWAP_HLI 0xCB36

#define OP_DAA  0x27

#define OP_CPL  0x2F
#define OP_CCF  0x3F
#define OP_SCF  0x37
#define OP_NOP  0x00
#define OP_HALT 0x76
#define OP_STOP 0x10
#define OP_DI   0xF3
#define OP_EI   0xFB

//Rotates and shifts

#define OP_RLCA 0x07
#define OP_RLA  0x17
#define OP_RRCA 0x0F
#define OP_RRA  0x1F

//RLC instructions listed with CB in op code. Significant?
#define OP_RLC_A   0xCB07
#define OP_RLC_B   0xCB00
#define OP_RLC_C   0xCB01
#define OP_RLC_D   0xCB02
#define OP_RLC_E   0xCB03
#define OP_RLC_H   0xCB04
#define OP_RLC_L   0xCB05
#define OP_RLC_HLI 0xCB06

//RL instructions listed with CB in op code
#define OP_RL_A   0xCB17
#define OP_RL_B   0xCB10
#define OP_RL_C   0xCB11
#define OP_RL_D   0xCB12
#define OP_RL_E   0xCB13
#define OP_RL_H   0xCB14
#define OP_RL_L   0xCB15
#define OP_RL_HLI 0xCB16

//RRC instructions listed with CB in op code
#define OP_RRC_A   0xCB0F
#define OP_RRC_B   0xCB08
#define OP_RRC_C   0xCB09
#define OP_RRC_D   0xCB0A
#define OP_RRC_E   0xCB0B
#define OP_RRC_H   0xCB0C
#define OP_RRC_L   0xCB0D
#define OP_RRC_HLI 0xCB0E

//RR instructions listead with CB in op code
#define OP_RR_A   0xCB1F
#define OP_RR_B   0xCB18
#define OP_RR_C   0xCB19
#define OP_RR_D   0xCB1A
#define OP_RR_E   0xCB1B
#define OP_RR_H   0xCB1C
#define OP_RR_L   0xCB1D
#define OP_RR_HLI 0xCB1E

#define OP_SLA_A   0xCB27
#define OP_SLA_B   0xCB20
#define OP_SLA_C   0xCB21
#define OP_SLA_D   0xCB22
#define OP_SLA_E   0xCB23
#define OP_SLA_H   0xCB24
#define OP_SLA_L   0xCB25
#define OP_SLA_HLI 0xCB26

#define OP_SRA_A   0xCB2F
#define OP_SRA_B   0xCB28
#define OP_SRA_C   0xCB29
#define OP_SRA_D   0xCB2A
#define OP_SRA_E   0xCB2B
#define OP_SRA_H   0xCB2C
#define OP_SRA_L   0xCB2D
#define OP_SRA_HLI 0xCB2E

#define OP_SRL_A   0xCB3F
#define OP_SRL_B   0xCB38
#define OP_SRL_C   0xCB39
#define OP_SRL_D   0xCB3A
#define OP_SRL_E   0xCB3B
#define OP_SRL_H   0xCB3C
#define OP_SRL_L   0xCB3D
#define OP_SRL_HLI 0xCB3E

//Bit instructions

#define OP_BIT_0_A   0xCB47
#define OP_BIT_0_B   0xCB40
#define OP_BIT_0_C   0xCB41
#define OP_BIT_0_D   0xCB42
#define OP_BIT_0_E   0xCB43
#define OP_BIT_0_H   0xCB44
#define OP_BIT_0_L   0xCB45
#define OP_BIT_0_HLI 0xCB46
#define OP_BIT_1_A   0xCB4F
#define OP_BIT_1_B   0xCB48
#define OP_BIT_1_C   0xCB49
#define OP_BIT_1_D   0xCB4A
#define OP_BIT_1_E   0xCB4B
#define OP_BIT_1_H   0xCB4C
#define OP_BIT_1_L   0xCB4D
#define OP_BIT_1_HLI 0xCB4E
#define OP_BIT_2_A   0xCB57
#define OP_BIT_2_B   0xCB50
#define OP_BIT_2_C   0xCB51
#define OP_BIT_2_D   0xCB52
#define OP_BIT_2_E   0xCB53
#define OP_BIT_2_H   0xCB54
#define OP_BIT_2_L   0xCB55
#define OP_BIT_2_HLI 0xCB56
#define OP_BIT_3_A   0xCB5F
#define OP_BIT_3_B   0xCB58
#define OP_BIT_3_C   0xCB59
#define OP_BIT_3_D   0xCB5A
#define OP_BIT_3_E   0xCB5B
#define OP_BIT_3_H   0xCB5C
#define OP_BIT_3_L   0xCB5D
#define OP_BIT_3_HLI 0xCB5E
#define OP_BIT_4_A   0xCB67
#define OP_BIT_4_B   0xCB60
#define OP_BIT_4_C   0xCB61
#define OP_BIT_4_D   0xCB62
#define OP_BIT_4_E   0xCB63
#define OP_BIT_4_H   0xCB64
#define OP_BIT_4_L   0xCB65
#define OP_BIT_4_HLI 0xCB66
#define OP_BIT_5_A   0xCB6F
#define OP_BIT_5_B   0xCB68
#define OP_BIT_5_C   0xCB69
#define OP_BIT_5_D   0xCB6A
#define OP_BIT_5_E   0xCB6B
#define OP_BIT_5_H   0xCB6C
#define OP_BIT_5_L   0xCB6D
#define OP_BIT_5_HLI 0xCB6E
#define OP_BIT_6_A   0xCB77
#define OP_BIT_6_B   0xCB70
#define OP_BIT_6_C   0xCB71
#define OP_BIT_6_D   0xCB72
#define OP_BIT_6_E   0xCB73
#define OP_BIT_6_H   0xCB74
#define OP_BIT_6_L   0xCB75
#define OP_BIT_6_HLI 0xCB76
#define OP_BIT_7_A   0xCB7F
#define OP_BIT_7_B   0xCB78
#define OP_BIT_7_C   0xCB79
#define OP_BIT_7_D   0xCB7A
#define OP_BIT_7_E   0xCB7B
#define OP_BIT_7_H   0xCB7C
#define OP_BIT_7_L   0xCB7D
#define OP_BIT_7_HLI 0xCB7E

#define OP_SET_0_A   0xCBC7
#define OP_SET_0_B   0xCBC0
#define OP_SET_0_C   0xCBC1
#define OP_SET_0_D   0xCBC2
#define OP_SET_0_E   0xCBC3 //CBC3 or CBCB? Gameboy documentation says C3 but z80 docs say CB
#define OP_SET_0_H   0xCBC4
#define OP_SET_0_L   0xCBC5
#define OP_SET_0_HLI 0xCBC6
#define OP_SET_1_A   0xCBCF
#define OP_SET_1_B   0xCBC8
#define OP_SET_1_C   0xCBC9
#define OP_SET_1_D   0xCBCA
#define OP_SET_1_E   0xCBCB
#define OP_SET_1_H   0xCBCC
#define OP_SET_1_L   0xCBCD
#define OP_SET_1_HLI 0xCBCE
#define OP_SET_2_A   0xCBD7
#define OP_SET_2_B   0xCBD0
#define OP_SET_2_C   0xCBD1
#define OP_SET_2_D   0xCBD2
#define OP_SET_2_E   0xCBD3
#define OP_SET_2_H   0xCBD4
#define OP_SET_2_L   0xCBD5
#define OP_SET_2_HLI 0xCBD6
#define OP_SET_3_A   0xCBDF
#define OP_SET_3_B   0xCBD8
#define OP_SET_3_C   0xCBD9
#define OP_SET_3_D   0xCBDA
#define OP_SET_3_E   0xCBDB
#define OP_SET_3_H   0xCBDC
#define OP_SET_3_L   0xCBDD
#define OP_SET_3_HLI 0xCBDE
#define OP_SET_4_A   0xCBE7
#define OP_SET_4_B   0xCBE0
#define OP_SET_4_C   0xCBE1
#define OP_SET_4_D   0xCBE2
#define OP_SET_4_E   0xCBE3
#define OP_SET_4_H   0xCBE4
#define OP_SET_4_L   0xCBE5
#define OP_SET_4_HLI 0xCBE6
#define OP_SET_5_A   0xCBEF
#define OP_SET_5_B   0xCBE8
#define OP_SET_5_C   0xCBE9
#define OP_SET_5_D   0xCBEA
#define OP_SET_5_E   0xCBEB
#define OP_SET_5_H   0xCBEC
#define OP_SET_5_L   0xCBED
#define OP_SET_5_HLI 0xCBEE
#define OP_SET_6_A   0xCBF7
#define OP_SET_6_B   0xCBF0
#define OP_SET_6_C   0xCBF1
#define OP_SET_6_D   0xCBF2
#define OP_SET_6_E   0xCBF3
#define OP_SET_6_H   0xCBF4
#define OP_SET_6_L   0xCBF5
#define OP_SET_6_HLI 0xCBF6
#define OP_SET_7_A   0xCBFF
#define OP_SET_7_B   0xCBF8
#define OP_SET_7_C   0xCBF9
#define OP_SET_7_D   0xCBFA
#define OP_SET_7_E   0xCBFB
#define OP_SET_7_H   0xCBFC
#define OP_SET_7_L   0xCBFD
#define OP_SET_7_HLI 0xCBFE

#define OP_RES_0_A   0xCB87
#define OP_RES_0_B   0xCB80
#define OP_RES_0_C   0xCB81
#define OP_RES_0_D   0xCB82
#define OP_RES_0_E   0xCB83
#define OP_RES_0_H   0xCB84
#define OP_RES_0_L   0xCB85
#define OP_RES_0_HLI 0xCB86
#define OP_RES_1_A   0xCB8F
#define OP_RES_1_B   0xCB88
#define OP_RES_1_C   0xCB89
#define OP_RES_1_D   0xCB8A
#define OP_RES_1_E   0xCB8B
#define OP_RES_1_H   0xCB8C
#define OP_RES_1_L   0xCB8D
#define OP_RES_1_HLI 0xCB8E
#define OP_RES_2_A   0xCB97
#define OP_RES_2_B   0xCB90
#define OP_RES_2_C   0xCB91
#define OP_RES_2_D   0xCB92
#define OP_RES_2_E   0xCB93
#define OP_RES_2_H   0xCB94
#define OP_RES_2_L   0xCB95
#define OP_RES_2_HLI 0xCB96
#define OP_RES_3_A   0xCB9F
#define OP_RES_3_B   0xCB98
#define OP_RES_3_C   0xCB99
#define OP_RES_3_D   0xCB9A
#define OP_RES_3_E   0xCB9B
#define OP_RES_3_H   0xCB9C
#define OP_RES_3_L   0xCB9D
#define OP_RES_3_HLI 0xCB9E
#define OP_RES_4_A   0xCBA7
#define OP_RES_4_B   0xCBA0
#define OP_RES_4_C   0xCBA1
#define OP_RES_4_D   0xCBA2
#define OP_RES_4_E   0xCBA3
#define OP_RES_4_H   0xCBA4
#define OP_RES_4_L   0xCBA5
#define OP_RES_4_HLI 0xCBA6
#define OP_RES_5_A   0xCBAF
#define OP_RES_5_B   0xCBA8
#define OP_RES_5_C   0xCBA9
#define OP_RES_5_D   0xCBAA
#define OP_RES_5_E   0xCBAB
#define OP_RES_5_H   0xCBAC
#define OP_RES_5_L   0xCBAD
#define OP_RES_5_HLI 0xCBAE
#define OP_RES_6_A   0xCBB7
#define OP_RES_6_B   0xCBB0
#define OP_RES_6_C   0xCBB1
#define OP_RES_6_D   0xCBB2
#define OP_RES_6_E   0xCBB3
#define OP_RES_6_H   0xCBB4
#define OP_RES_6_L   0xCBB5
#define OP_RES_6_HLI 0xCBB6
#define OP_RES_7_A   0xCBBF
#define OP_RES_7_B   0xCBB8
#define OP_RES_7_C   0xCBB9
#define OP_RES_7_D   0xCBBA
#define OP_RES_7_E   0xCBBB
#define OP_RES_7_H   0xCBBC
#define OP_RES_7_L   0xCBBD
#define OP_RES_7_HLI 0xCBBE

//Jump instructions

#define OP_JP 0xC3

#define OP_JP_NZ  0xC2
#define OP_JP_Z   0xCA
#define OP_JP_NC  0xD2
#define OP_JP_C   0xDA
#define OP_JP_HLI 0xE9
#define OP_JR     0x18
#define OP_JR_NZ  0x20
#define OP_JR_Z   0x28
#define OP_JR_NC  0x30
#define OP_JR_C   0x38

//Call instructions

#define OP_CALL    0xCD
#define OP_CALL_NZ 0xC4
#define OP_CALL_Z  0xCC
#define OP_CALL_NC 0xD4
#define OP_CALL_C  0xDC

//Restart instructions

#define OP_RST_00H 0xC7
#define OP_RST_08H 0xCF
#define OP_RST_10H 0xD7
#define OP_RST_18H 0xDF
#define OP_RST_20H 0xE7
#define OP_RST_28H 0xEF
#define OP_RST_30H 0xF7
#define OP_RST_38H 0xFF

//Return instructions

#define OP_RET    0xC9
#define OP_RET_NZ 0xC0
#define OP_RET_Z  0xC8
#define OP_RET_NC 0xD0
#define OP_RET_C  0xD8
#define OP_RETI   0xD9

//Non-opcode helpers
#define OP_IS_CB_PREFIXED 0xCB /*Used to detect opcodes starting with CB*/
#define OP_STOP8 0x10 /*Used to detect opcode OP_STOP. Sanity check next byte is 00 first!*/

