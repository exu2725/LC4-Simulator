/*
 * loader.c : Defines loader functions for opening and loading object files
 */

#include "loader.h"
#define CONCAT_DIGITS(D1, D2) ((D1 << 8) | D2)   // macro definition for concat two bytes

/*
 * Read an object file and modify the machine state as described in the writeup
 */
int ReadObjectFile(char* filename, MachineState* CPU)
{
  unsigned short int dig1; //store first byte
  unsigned short int dig2; //store second byte
  
  FILE* input_p = fopen(filename, "r"); //open file for reading

  if (input_p == NULL) {  //return error message if file does not exist
    perror("error: File does not exist");
    return -1;
  }

  while (!feof(input_p)) {
    dig1 = fgetc(input_p);  //retrieve first two bytes
    dig2 = fgetc(input_p);

    if ((dig1 == 0xCA && dig2 == 0xDE) ||(dig1 == 0xDA && dig2 == 0xDA)){ //handle DATA or CODE directives
      parse(input_p, CPU);
    }
    else if (dig1 == 0xC3 && dig2 == 0xB7) {  //handle symbol directives
      parse_symbolOrFile(input_p, CPU, 0);
    }
    else if (dig1 == 0xF1 && dig2 == 0x7E) {  //handle file directives
      parse_symbolOrFile(input_p, CPU, 1);
    }
    else if (dig1 == 0x71 && dig2 == 0x5E) {  // handle line number directives
      dig1 = fgetc(input_p);
      dig2 = fgetc(input_p);
      dig1 = fgetc(input_p);
      dig2 = fgetc(input_p);
      dig1 = fgetc(input_p);
      dig2 = fgetc(input_p);
    }
  }

  close(input_p); //close file
  
  return 0;
}

//helper function to parse data and code directives
parse(FILE* input_p, MachineState* CPU) {
    unsigned short int dig1;
    unsigned short int dig2;
    unsigned short int address;
    unsigned short int numLines;
    unsigned short int i;

    //retrieve address to store data/code
    dig1 = fgetc(input_p);
    dig2 = fgetc(input_p);
    address = CONCAT_DIGITS(dig1, dig2);
    
    //retrieve number of lines/commands
    dig1 = fgetc(input_p);
    dig2 = fgetc(input_p);
    numLines = CONCAT_DIGITS(dig1, dig2);

    for (i = 0; i < numLines; i++) {   // navigate numLines words and store in memory
        dig1 = fgetc(input_p);
        dig2 = fgetc(input_p);
        CPU->memory[address] = CONCAT_DIGITS(dig1, dig2);

        address++;
    }
}

//helper function to parse symbol or file directives
parse_symbolOrFile(FILE* input_p, MachineState* CPU, int type) {
    unsigned short int dig1;
    unsigned short int dig2;
    unsigned short int numLines;
    //unsigned short int line;
    unsigned short int i;

    //if symbol, retrieve address
    if (type == 0) {
      //retrieve address
      dig1 = fgetc(input_p);
      dig2 = fgetc(input_p);
    }
    
    //retrieve number of lines/commands
    dig1 = fgetc(input_p);
    dig2 = fgetc(input_p);
    numLines = CONCAT_DIGITS(dig1, dig2);

    // perform actions for the number of words in the directive (1 byte)
    for (i = 0; i < numLines; i++) {
        dig1 = fgetc(input_p);
    }
}
