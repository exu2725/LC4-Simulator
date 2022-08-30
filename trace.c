/*
 * trace.c: location of main() to start the simulator
 */

#include "loader.h"

// Global variable defining the current state of the machine
MachineState* CPU;

int main(int argc, char** argv)
{
    MachineState machine;
    FILE * output_p;
    int i;
    unsigned short line;
    CPU = &machine;

    // check if enough number of command line arguments given
    if (argc < 3) { // If missing one of the necessary components, return error message
        perror("Please enter ./trace output_filename.txt first.obj ...\n");
        return -1;
    }
    
    Reset(CPU);
    
    output_p = fopen(argv[1], "w");   // open output_filename for writing

    for (i = 2; i < argc; i++) {
        if (ReadObjectFile(argv[i], CPU) == -1) { // If obj file doesn't exist, return error message
            perror("Please enter ./trace output_filename.txt first.obj ...\n");
            return -1;
        }
    }

    /*for (i = 0; i < 0xFFFF; i++) {  //cycle through all memory addresses
        line = CPU->memory[i];
        if (line != 0) {
            fprintf(output_p,"address: %05d contents: 0x%04X\n", i, line); //add non-zero values to output file
        }
    }*/
    
    // CPU->PC = 0;

    while (1) {
        // program should exit upon errors or ending
        if (UpdateMachineState(CPU, output_p) != 0) {
            break;
        }
    }

    fclose(output_p);     // close output file
    return 0;
}