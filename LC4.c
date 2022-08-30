/*
 * LC4.c: Defines simulator functions for executing instructions
 */

#include "LC4.h"
#include <stdio.h>

// macro definitions
#define INSN_OP(I) I >> 12              // Determine the Op type
#define INSN_5(I) I >> 5 & 0x1          // Decide if it is immediate arithmetic operation
#define INSN_AL_3(I) I >> 3 & 0x7       // For arithmetic operations, the instruction type
#define INSN_AL_4(I) I >> 4 & 0x3       // For shift/mod operations, operation type
#define INSN_RD_9(I) I >> 9 & 0x7       // Rd name
#define INSN_RS_6(I) I >> 6 & 0x7       // Rs name
#define INSN_RT_0(I) I & 0x7            // Rt name
#define INSN_CMP_7(I) I >> 7 & 0x3      // For CMP operations, the instruction type

//sign extend helper function
short int Sext(unsigned short int value, unsigned int numBits) {
    int sign = (value >> (numBits - 1)) & 1;
    int mask = 0xFFFF;
    if (sign == 0) {
        return value;
    }
    mask = mask << numBits;
    return value | mask;
}

/*
 * Reset the machine state as Pennsim would do
 */
void Reset(MachineState* CPU)
{
    int i;

    CPU->PC = 0x8200;
    CPU->PSR = 0x8002;

    for (i = 0; i < 8; i++) {
        CPU->R[i] = 0;
    }

    ClearSignals(CPU);

    for (i = 0; i <= 0xFFFF; i++) {
        CPU->memory[i] = 0;
    }
}


/*
 * Clear all of the control signals (set to 0)
 */
void ClearSignals(MachineState* CPU)
{
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;

    CPU->regInputVal = 0;
    CPU->NZPVal = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
}


/*
 * This function should write out the current state of the CPU to the file output.
 */
void WriteOut(MachineState* CPU, FILE* output)
{
    unsigned short int inst = CPU->memory[CPU->PC];

    // print to file
    // 1.the current PC 
    fprintf(output,"%04X ",CPU->PC);
  
    // 2.the current instruction (written in binary)
    for (int i = 0; i < 16; i++) {
        if (((inst << i) & 0x8000) == 0x8000) {
	        fprintf(output,"1");
        }
        else {
            fprintf(output,"0");
        }
    }

    // 3.the register file WE
    fprintf(output, " %d", CPU->regFile_WE);
    
    // 4.if regFileWE is high, which register is being written to
    if (CPU->regFile_WE) {      
        fprintf(output, " %d", CPU->rdMux_CTL);
        // 5.if regFileWE is high, what value is being written to the register file
        fprintf(output, " %04X", CPU->regInputVal);
    } else {
        fprintf(output, " %d", 0);
        // 5.if regFileWE is high, what value is being written to the register file
        fprintf(output, " %04X", 0);
    }
    
    // 6.the NZP WE
    fprintf(output, " %d", CPU->NZP_WE);

    // 7.if NZP WE is high, what value is being written to the NZP register
    if (CPU->NZP_WE) {
        fprintf(output, " %d", CPU->NZPVal);
    } else {
        fprintf(output, " %d", 0);      
    }
       
    // 8.the data WE
    fprintf(output, " %d", CPU->DATA_WE);
    
    // 9.the data memory address
    fprintf(output, " %04X", CPU->dmemAddr);
    // 10.what value is being loaded or stored into memory
    fprintf(output, " %04X", CPU->dmemValue);

    // close the current line
    fprintf(output, "\n");

    return;
}

/*
 * Parses rest of branch operation and updates state of machine.
 */
void LDROp(MachineState* CPU, FILE* output)
{
    unsigned short int inst = CPU->memory[CPU->PC]; //retrieve instruction
    short int imm6 = Sext(inst & 0x3F, 6); //calculate sext(IMM6)

    CPU->regFile_WE = 1;    //regFile_WE HIGH
    CPU->DATA_WE = 0;   //DATA_WE Low

    CPU->rsMux_CTL = INSN_RS_6(inst);   //Rs Register (0-7)
    CPU->rdMux_CTL = INSN_RD_9(inst);   //Rd Register (0-7)
    CPU->rtMux_CTL = 0;                 //Rt Register not used

    CPU->dmemAddr = CPU->R[CPU->rsMux_CTL] + imm6;  //address
    CPU->dmemValue = CPU->memory[CPU->dmemAddr];    //value

    //set input value to dmem[Rs + sext(IMM6)]
    CPU->regInputVal = CPU->dmemValue;

    //sets Rd equal to previously obtained value
    CPU->R[CPU->rdMux_CTL] = CPU->regInputVal;

    //set NZP_WE and NZPVal
    SetNZP(CPU, CPU->regInputVal);

    //write out to output file
    WriteOut(CPU, output);

    CPU->rsMux_CTL = 0;
    CPU->rdMux_CTL = 0;

    //next PC state
    CPU->PC += 1;
}

/*
 * Parses rest of branch operation and updates state of machine.
 */
void STROp(MachineState* CPU, FILE* output)
{
    unsigned short int inst = CPU->memory[CPU->PC];
    short int imm6 = Sext(inst & 0x3F, 6); //calculate sext(IMM6)

    CPU->regFile_WE = 0; // Low
    CPU->NZP_WE = 0;    // Low
    CPU->DATA_WE = 1; // high

    CPU->rsMux_CTL = INSN_RS_6(inst);
    CPU->rtMux_CTL = INSN_RD_9(inst);
    CPU->rdMux_CTL=0;   //unused

    CPU->regInputVal = 0; //unused
    CPU->NZPVal = 0;    //unused

    CPU->dmemAddr = CPU->R[CPU->rsMux_CTL] + imm6;  //address to store value in
    CPU->dmemValue = CPU->R[CPU->rtMux_CTL];    //value to store in address

    CPU->memory[CPU->dmemAddr]= CPU->dmemValue; //dmem[Rs + sext(IMM6)] = Rt

    WriteOut(CPU, output);

    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 1;

    CPU->PC += 1;
}

/*
 * Parses rest of branch operation and updates state of machine.
 */
void RTIOp(MachineState* CPU, FILE* output)
{
    // get current instruction
    unsigned short int inst = CPU->memory[CPU->PC];

    //all unused
    CPU->rsMux_CTL = 1;
    CPU->rdMux_CTL = 0;
    CPU->rtMux_CTL = 0;

    // all low
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;

    //all unused
    CPU->regInputVal = 0; 
    CPU->NZPVal = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    WriteOut(CPU, output);

    CPU->PC = CPU->R[7]; //PC = R7
    CPU->PSR = CPU->PSR & 0x7FFF; //and op to 0111 1111 1111 1111 (PSR [15] = 0)
}

/*
 * Parses rest of CONST operation and prints out.
 */
void ConstOp(MachineState* CPU, FILE* output)
{
    // get current instruction
    unsigned short int inst = CPU->memory[CPU->PC];

    CPU->rsMux_CTL = 0; //unused
    CPU->rdMux_CTL = INSN_RD_9(inst);   // Rd value
    CPU->rtMux_CTL = 0; //unused

    CPU->regFile_WE = 1;    //high
    CPU->DATA_WE = 0;   //low

    CPU->regInputVal = Sext(inst & 0x1FF, 9);    // sext(imm9) value
    CPU->R[CPU->rdMux_CTL] = CPU->regInputVal;  //Rd = sext(IMM9)

    //NZP_WE is high
    SetNZP(CPU, CPU->regInputVal);

    //unused
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    WriteOut(CPU, output);

    CPU->rdMux_CTL = 0;   // Rd value

    CPU->PC += 1;

}

/*
 * Parses rest of CONST operation and prints out.
 */
void HiConstOp(MachineState* CPU, FILE* output)
{
    // get current instruction
    unsigned short int inst = CPU->memory[CPU->PC];
    unsigned short int u_imm8 = inst & 0xFF; //unsigned imm8 value

    CPU->rsMux_CTL = 2; //unused
    CPU->rdMux_CTL = INSN_RD_9(inst);   // Rd value
    CPU->rtMux_CTL = 0; //unused

    CPU->regFile_WE = 1;    //high
    CPU->DATA_WE = 0;   //low

    CPU->regInputVal = (CPU->R[CPU->rdMux_CTL] & 0xFF) | (u_imm8 << 8);    // HICONST Rd UIMM8
    CPU->R[CPU->rdMux_CTL] = CPU->regInputVal;  //Rd = (Rd & 0xFF) | (UIMM8) << 8)
    
    //NZP_WE is high
    SetNZP(CPU, CPU->regInputVal);

    //unused
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    WriteOut(CPU, output);

    CPU->rdMux_CTL = 0;   // Rd value

    CPU->PC += 1;

}

/*
 * Parses rest TRAP operation and prints out.
 */
void TrapOp(MachineState* CPU, FILE* output)
{
    // get current instruction
    unsigned short int inst = CPU->memory[CPU->PC];

    CPU->rsMux_CTL = 0; //unused
    CPU->rdMux_CTL = 7; //Rd is R7
    CPU->rtMux_CTL = 0; //unused

    CPU->regFile_WE = 1; //high
    CPU->DATA_WE = 0; //low

    CPU->regInputVal = CPU->PC + 1; 

    //unused
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    //NZP_WE is high
    SetNZP(CPU, CPU->regInputVal);

    CPU->R[CPU->rdMux_CTL] = CPU->regInputVal; //R7 = PC + 1
    WriteOut(CPU, output);

    CPU->rdMux_CTL = 1;

    CPU->PC = 0x8000 | (inst & 0xFF); //PC = (0x8000 | UIMM8)
    CPU->PSR = CPU->PSR | 0x8000; //PSR[15] = 1
}

int CheckErrors(MachineState* CPU) {
    unsigned short int instAddr = CPU->PC;
    unsigned short int inst = CPU->memory[CPU->PC];
    unsigned short int inst_type = INSN_OP(inst);

    unsigned short int rs = INSN_RS_6(inst);   //Rs register
    short int imm6 = Sext(inst & 0x3F, 6); //calculate sext(IMM6)
    unsigned short int address = CPU->R[rs] + imm6;

    // executing data as code
    if ((instAddr >= 0x2000 && instAddr < 0x8000) || (instAddr >= 0xA000)) {
        return 1;
    }
    
    // reading code as data
    if (inst_type == 6 || inst_type == 7) {
        if ((address >= 0x8000 && address < 0xA000) || address < 0x2000) {
            return 2;
        }
        if (address >= 0xA000) {
            //trying to store data in OS without permission
            if((CPU->PSR & 0x8000) == 0) {
                return 3;
            }
        }
    }

    //trying to access OS without permission
    if (instAddr >= 0x8000) {
        if (CPU->PSR & 0x8000 == 0) {
            return 3;
        }
    }
    return 0;
}

/*
 * This function should execute one LC4 datapath cycle.
 */
int UpdateMachineState(MachineState* CPU, FILE* output)
{   
    // Get the current PC value
    unsigned short int inst = CPU->memory[CPU->PC];
    unsigned short int inst_type = INSN_OP(inst);
    
    //exit address check
    if (CPU->PC == 0x80FF) {
        return 4;
    }

    // has errors in the code
    int errorCode = CheckErrors(CPU);
    if (errorCode > 0) {
        return errorCode;
    }

    switch (inst_type) {
        case 0x0000:        // branch operations
            BranchOp(CPU, output);
            break;
        case 0x0001:             // It is an arithmetic operation
 //           printf("Entering Arith\n");
            ArithmeticOp(CPU, output);
            break;
        case 0x0002:        // comparative operations
            ComparativeOp(CPU, output);
            break;
        case 0x0003:        // no operations
            break;
        case 0x0004:        // JSR operations
            JSROp(CPU, output);
            break;
        case 0x0005:        // logical operations
            LogicalOp(CPU, output);
            break;
        case 0x0006:        // LDR operation
            LDROp(CPU, output);
            break;
        case 0x0007:        // STR operation
            STROp(CPU, output);
            break;
        case 0x0008:        // RTI operation 
            RTIOp(CPU, output);
            break;
        case 0x0009:        // Constant Op
            ConstOp(CPU, output);
            break;
        case 0x000A: // Shift/mod ops
            ShiftModOp(CPU, output);
            break;
        case 0x000B:        // no operation
            break;
        case 0x000C:        // JMPR and JMP operations
            JumpOp(CPU, output);
            break;
        case 0x000D:        // HICONST operation
            HiConstOp(CPU, output);
            break;
        case 0x000E:        // no operation
            break;
        case 0x000F:    //TRAP ops
            TrapOp(CPU, output);
            break;
        default:
            break;
    }

    return 0;
}

/*
 * Parses rest of branch operation and updates state of machine.
 */
void BranchOp(MachineState* CPU, FILE* output)
{
    // get current instruction
    unsigned short int inst = CPU->memory[CPU->PC];

    // Branch Op type
    unsigned short int inst_type = INSN_RD_9(inst) & 0x7;

    // IMM9 value
    signed short int imm9 = Sext(inst & 0x1FF, 9);

    //all unused
    CPU->rsMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->rtMux_CTL = 0;

    //all low
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;

    //unused
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    CPU->regInputVal = 0; //unused
    
    WriteOut(CPU, output);

    switch (inst_type) {
        case 0:             // NOP
            break;
        case 1:             // BRp
            if (CPU->NZPVal == 1) {
                CPU->PC += imm9; //( P) ? PC = PC + 1 + sext(IMM9 offset to <Label>)
            }
            break;
        case 2:             // BRz
            if (CPU->NZPVal == 2) {
                CPU->PC += imm9; //( Z ) ? PC = PC + 1 + sext(IMM9 offset to <Label>)
            }
            break;
        case 3:             // BRzp
            if (CPU->NZPVal == 1 || CPU->NZPVal == 2) {
                CPU->PC += imm9; //( Z|P) ? PC = PC + 1 + sext(IMM9 offset to <Label>)
            }
            break;
        case 4:             // BRn
            if (CPU->NZPVal == 4) {
                CPU->PC += imm9; //(N ) ? PC = PC + 1 + sext(IMM9 offset to <Label>)
            }
            break;
        case 5:             // BRnp
            if (CPU->NZPVal == 1 || CPU->NZPVal == 4) {
                CPU->PC += imm9; //(N | P) ? PC = PC + 1 + sext(IMM9 offset to <Label>)
            }
            break;
        case 6:                 //BRnz
            if (CPU->NZPVal == 2 || CPU->NZPVal == 4) {
                CPU->PC += imm9; //(N|Z ) ? PC = PC + 1 + sext(IMM9 offset to <Label>)
            }
            break;
        case 7:                 //BRnzp
            CPU->PC += imm9; //(N|Z|P) ? PC = PC + 1 + sext(IMM9 offset to <Label>)
            break;
        default:
            break;
    }

    CPU->PC += 1;
}

/*
 * Parses rest of arithmetic operation and prints out.
 */
void ArithmeticOp(MachineState* CPU, FILE* output)
{
    // get current instruction
    unsigned short int inst = CPU->memory[CPU->PC];

    CPU->regFile_WE = 1; //high
    CPU->DATA_WE = 0; //low

    CPU->rdMux_CTL = INSN_RD_9(inst);   //Rd register (0-7)
    CPU->rsMux_CTL = INSN_RS_6(inst);   //Rs register
    CPU->rtMux_CTL = INSN_RT_0(inst);   //Rt register

    //unused
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    unsigned short int inst_type = INSN_5(inst); //determine which specific operation
    if (inst_type == 1) {    // it is immediate op
        CPU->regInputVal = CPU->R[CPU->rsMux_CTL] + Sext(inst & 0x1F, 5); //Rd = Rs + sext(IMM5)
    } else {

        CPU->rtMux_CTL = INSN_RT_0(inst);   //Rt register
        // standard arithmetic operations
        inst_type = INSN_AL_3(inst);

        switch (inst_type) {
            case 0:         // ADD Rs Rd Rt
                CPU->regInputVal = CPU->R[CPU->rsMux_CTL] + CPU->R[CPU->rtMux_CTL];
                break;
            case 1:         // MUL Rs Rd Rt
                CPU->regInputVal = CPU->R[CPU->rsMux_CTL] * CPU->R[CPU->rtMux_CTL];
                break;
            case 2:         // SUB Rs Rd Rt
                CPU->regInputVal = CPU->R[CPU->rsMux_CTL] - CPU->R[CPU->rtMux_CTL];
                break;
            case 3:         // DIV Rs Rd Rt
                CPU->regInputVal = CPU->R[CPU->rsMux_CTL] / CPU->R[CPU->rtMux_CTL];
                break;
            default:
                break;
        } 
    }
    
    CPU->R[CPU->rdMux_CTL] = CPU->regInputVal; //set RD to result
    SetNZP(CPU, CPU->regInputVal); //NZP_WE is high

    // write the current line
    WriteOut(CPU, output);

    CPU->rdMux_CTL = 0;
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;

    CPU->PC += 1;
}

/*
 * Parses rest of comparative operation and prints out.
 */
void ComparativeOp(MachineState* CPU, FILE* output)
{
    // get current instruction
    unsigned short int inst = CPU->memory[CPU->PC];

    CPU->rsMux_CTL = INSN_RD_9(inst); //Rs register (0-7)
    CPU->rtMux_CTL = INSN_RT_0(inst); //Rt register (0-7)
    CPU->rdMux_CTL = 0; //unused

    CPU->regFile_WE = 0; //low
    CPU->DATA_WE = 0; //low

    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;
   
    // Branch Op type
    unsigned short int inst_type = INSN_CMP_7(inst);
    unsigned short int imm7 = inst & 0x7F;

    switch (inst_type) {
        case 0:             // CMP Rs Rt
            CPU->regInputVal = Sext(CPU->R[CPU->rsMux_CTL], 16) - Sext(CPU->R[CPU->rtMux_CTL], 16);
            break;
        case 1:             // CMPU Rs Rt
            CPU->regInputVal = CPU->R[CPU->rsMux_CTL] - CPU->R[CPU->rtMux_CTL];
            break;
        case 2:             // CMPI Rs imm7
            CPU->regInputVal = CPU->R[CPU->rsMux_CTL] - Sext(imm7, 7);
            break;
        case 3:             // CMPIU Rs uimm7
            CPU->regInputVal = CPU->R[CPU->rsMux_CTL] - imm7;
            break;
        default:
            break;
    }

    SetNZP(CPU, CPU->regInputVal); //NZP_WE is high
    CPU->regInputVal = 0; //reset it

    // print out before change PC
    WriteOut(CPU, output);

    CPU->rsMux_CTL = 2;
    CPU->rtMux_CTL = 0;
    
    CPU->PC += 1;    
}

/*
 * Parses rest of logical operation and prints out.
 */
void LogicalOp(MachineState* CPU, FILE* output)
{
    // get current instruction
    unsigned short int inst = CPU->memory[CPU->PC];

    CPU->regFile_WE = 1; //high
    CPU->DATA_WE = 0; //low

    // all used
    CPU->rdMux_CTL = INSN_RD_9(inst);
    CPU->rsMux_CTL = INSN_RS_6(inst);
    CPU->rtMux_CTL = INSN_RT_0(inst);

    //unused
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    unsigned short int inst_type = INSN_5(inst);
    if (inst_type == 1) {    // it is immediate op
        CPU->rtMux_CTL = 0; //unused
        CPU->regInputVal = CPU->R[CPU->rsMux_CTL] & Sext(inst & 0x1F, 5);
    } else {  
        // standard arithmetic operations
        inst_type = INSN_AL_3(inst);

        switch (inst_type) {
            case 0:         // AND Rs Rd Rt
                CPU->regInputVal = CPU->R[CPU->rsMux_CTL] & CPU->R[CPU->rtMux_CTL];
                break;
            case 1:         // NOT Rd Rs
                CPU->regInputVal = ~ CPU->R[CPU->rsMux_CTL];
                break;
            case 2:         // OR Rs Rd Rt
                CPU->regInputVal = CPU->R[CPU->rsMux_CTL] | CPU->R[CPU->rtMux_CTL];
                break;
            case 3:         // XOR Rs Rd Rt
                CPU->regInputVal = CPU->R[CPU->rsMux_CTL] ^ CPU->R[CPU->rtMux_CTL];
                break;
            default:
                break;
        } 
    }
    
    CPU->R[CPU->rdMux_CTL] = CPU->regInputVal;

    SetNZP(CPU, CPU->regInputVal); //NZP_WE is high

    // write the current line
    WriteOut(CPU, output);

    CPU->rdMux_CTL = 0;
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;

    CPU->PC += 1;
}

/*
 * Parses rest of jump operation and prints out.
 */
void JumpOp(MachineState* CPU, FILE* output)
{
    // get current instruction
    unsigned short int inst = CPU->memory[CPU->PC];
    short int imm11 = Sext(inst & 0x7FF, 11); //Sext(IMM11) value

    //unused
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;

    //unused
    CPU->rsMux_CTL = INSN_RS_6(inst); //Rs register (0-7)
    CPU->rdMux_CTL = 0;
    CPU->rtMux_CTL = 0;

    //unused
    CPU->NZPVal = 0;
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    WriteOut(CPU, output);

    if (inst & 0x800 == 0) { // JMPR   
        CPU->PC = CPU->R[CPU->rsMux_CTL]; //PC = Rs
    }
    else { //JMP
        CPU->PC += (imm11 + 1); //PC = PC + 1 + sext(IMM11 offset to <Label>)
    }

    CPU->rsMux_CTL = 0;
}

/*
 * Parses rest of JSR operation and prints out.
 */
void JSROp(MachineState* CPU, FILE* output)
{
    // get current instruction
    unsigned short int inst = CPU->memory[CPU->PC];
    short int imm11 = Sext(inst & 0x7FF, 11);

    CPU->regFile_WE = 1; //high
    CPU->DATA_WE = 0; //low

    //unused
    CPU->rdMux_CTL = 1;
    CPU->rsMux_CTL = INSN_RS_6(inst); //Rs register value
    CPU->rtMux_CTL = 0;

    //unused
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    //R7 = PC + 1
    CPU->regInputVal = CPU->PC + 1;
    CPU->R[7] = CPU->regInputVal;

    SetNZP(CPU, CPU->regInputVal); //NZP_WE is high

    WriteOut(CPU, output);

    if (inst & 0x800 == 0) { // JSRR
        
        CPU->PC = CPU->R[CPU->rsMux_CTL];
    }
    else { //JSR
        //PC = (PC & 0x8000) | (IMM11 << 4 offset to <Label>)
        CPU->PC = (CPU->PC & 0x8000) | (imm11 << 4);
    }
    
    CPU->rsMux_CTL = 0;
}

/*
 * Parses rest of shift/mod operations and prints out.
 */
void ShiftModOp(MachineState* CPU, FILE* output)
{
    // get current instruction
    unsigned short int inst = CPU->memory[CPU->PC];
    unsigned short int u_imm4 = inst & 0xF;
    int i;
    unsigned short int placeholder;

    CPU->regFile_WE = 1; //high
    CPU->DATA_WE = 0; //low

    CPU->rdMux_CTL = INSN_RD_9(inst); //Rd register value
    CPU->rsMux_CTL = INSN_RS_6(inst); //Rs register value
    CPU->rtMux_CTL = 0; //unused

    //unused
    CPU->dmemAddr = 0;
    CPU->dmemValue = 0;

    unsigned short int inst_type = INSN_AL_4(inst); //determine instruction type
    switch (inst_type) {
        case 0:         // SLL Rd Rs UIMM4
            CPU->regInputVal = CPU->R[CPU->rsMux_CTL] << u_imm4; //Rd = Rs << UIMM4
            break;
        case 1:         // SRA Rd Rs UIMM4           
            placeholder = CPU->R[CPU->rsMux_CTL];    //preserve original value of RS
            for (i = 0; i < u_imm4; i++) {  //for each iteration, arithmetic shift right once
                placeholder = (placeholder & 0x8000) + (placeholder >> 1);
            }
            CPU->regInputVal = placeholder; //assign correct value
            break;
        case 2:         // SRL Rd Rs UIMM4
            CPU->regInputVal = CPU->R[CPU->rsMux_CTL] >> u_imm4; //Rd = Rs >> UIMM4
            break;
        case 3:         // MOD Rd Rs Rt
            CPU->rtMux_CTL = INSN_RT_0(inst); //Rt register value
            CPU->regInputVal = CPU->R[CPU->rsMux_CTL] % CPU->R[CPU->rtMux_CTL]; //Rd = Rs % Rt
            break;
        default:
            break;
    }
    
    CPU->R[CPU->rdMux_CTL] = CPU->regInputVal; //Set Rd to correct value

    SetNZP(CPU, CPU->regInputVal); //NZP_WE is high

    // write the current line
    WriteOut(CPU, output);

    CPU->rdMux_CTL = 0;
    CPU->rsMux_CTL = 0;

    CPU->PC += 1;
}

/*
 * Set the NZP bits in the PSR.
 */
void SetNZP(MachineState* CPU, short result)
{
    //set NZP_WE to high
    CPU->NZP_WE = 1;

    //if positive
    if (result > 0) {
        CPU->NZPVal = 1;
    } else if (result == 0) { //if zero
        CPU->NZPVal = 2;
    } else {    // if negative
        CPU->NZPVal = 4;
    }
}