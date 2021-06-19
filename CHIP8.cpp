#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

typedef struct Chip8 {
/*	
    The Chip-8 language is capable of accessing up to 4KB (4,096 bytes) of RAM,
    from location 0x000 (0) to 0xFFF (4095). The first 512 bytes, from 0x000 to
    0x1FF, are where the original interpreter was located, and should not be used
    by programs.

    Most Chip-8 programs start at location 0x200 (512), but some begin at 0x600
    (1536). Programs beginning at 0x600 are intended for the ETI 660 computer.

    Memory Map:
    +---------------+= 0xFFF (4095) End of Chip-8 RAM
    |               |
    |               |
    |               |
    |               |
    |               |
    | 0x200 to 0xFFF|
    |     Chip-8    |
    | Program / Data|
    |     Space     |
    |               |
    |               |
    |               |
    +- - - - - - - -+= 0x600 (1536) Start of ETI 660 Chip-8 programs
    |               |
    |               |
    |               |
    +---------------+= 0x200 (512) Start of most Chip-8 programs
    | 0x000 to 0x1FF|
    | Reserved for  |
    |  interpreter  |
    +---------------+= 0x000 (0) Start of Chip-8 RAM */
    
    //  Our 4kb of ram
    uint8_t ram[4096];

    //  Chip-8 has 16 general purpose 8-bit registers, usually referred to as Vx, where
    //  x is a hexadecimal digit (0 through F). There is also a 16-bit register called
    //  I. This register is generally used to store memory addresses, so only the
    //  lowest (rightmost) 12 bits are usually used.

    //  The VF register should not be used by any program, as it is used as a flag by
    //  some instructions. See section 3.0, Instructions for details.

    //  Chip-8 also has two special purpose 8-bit registers, for the delay and sound
    //  timers. When these registers are non-zero, they are automatically decremented
    //  at a rate of 60Hz. See the section 2.5, Timers & Sound, for more information on
    //  these.
    
    //  There are also some "pseudo-registers" which are not accessable from Chip-8
    //  programs. The program counter (PC) should be 16-bit, and is used to store the		
    //  currently executing address. The stack pointer (SP) can be 8-bit, it is used to
    //	point to the topmost level of the stack.
    
    //	The stack is an array of 16 16-bit values, used to store the address that the
    //	interpreter shoud return to when finished with a subroutine. Chip-8 allows for
    //	up to 16 levels of nested subroutines.

    // Registers V0 -- VF
    uint8_t V[16];

    // 16 bit Index register
    uint16_t I;

    // Program Counter
    uint16_t PC;

    // Opcode Scheme:
    //                   ++++--:Operation Select
    //                   ||||
    //                   ||||++++--Vx: Regsister Select
    //                   ||||||||
    //                   ||||||||++++--Vy: Resgister Select
    //                   ||||||||||||
    //                   ||||||||||||++++--N: 4 bit number / nibble
    //                   ||||||||||||||||
    char16_t instr; // 0b0000000000000000;
    //                       ||||||||||||
    //                       ||||++++++++--NN: Second Byte
    //                       ||||
    //                       ++++--NNN: 2nd 3rd and 4th nibbles
    #define OP  0xF000
    #define Vx  0x0F00
    #define Vy  0x00F0
    #define N   0x000F
    #define NN  0x00FF
    #define NNN 0x0FFF

    // Stack Pointer
    uint8_t SP;

    // Stack
    uint16_t stack[16];

    // Display -- 64x32 pixels
    bool display[64][32];
    
    // 8  bit Delay timer register @60Hz
    uint8_t dTimer;

    // 8 bit sound timer register @60Hz
    uint8_t sTimer;

    // Fontset -- "Cosmacvip"
    uint8_t fontset[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

} Chip8;

// Load Chip-8 ROM file into memory, starting at 0x200
void loadROM(char const* filename, Chip8* vm) {
    ifstream file(filename, ios::binary | ios::ate);

    if (file.is_open()) {
        streamsize fileSize = file.tellg();
        
        // Create a buffer as large as the ROM file
        vector<char> buffer(fileSize);
        
        // Read file into buffer
        file.seekg(0, ios::beg);
        file.read(buffer.data(), fileSize);
        file.close();
        
        // Read the ROM into memory
        for (int i = 0; i < fileSize; i++) {
            vm->ram[i + 0x200] = buffer[i];
        }
    }
}
// Grab opcode, Increment Program Counter
void fetch (Chip8* vm) {
    vm->instr = vm->ram[vm->PC];
    vm->PC += 2;
}

void reset (bool arr[64][32]) {
    for (int i = 0; i<64; i++) {
        for (int j = 0; j<32; j++) {
            arr[i][j] = 0;
        }
    }
} 

void draw (const uint8_t* x, const uint8_t* y, const uint8_t* n, const uint16_t* I) {
        
}
#define INSTRUCTION_LIST(o)\
    o("SYS addr",           "0nnn", )/*Execute machine language subroutine at address NNN*/\
    o("CLS",                "00E0", u == 0x0 && kk == 0xE0, reset(display))/*Clear the screen*/\
    o("RET",                "00EE", u == 0x0 && kk == 0xEE, )/*Return from a subroutine*/\
    o("JP addr",            "1nnn", u == 0x1, PC = nnn)/*Jump to address NNN*/\
    o("CALL addr",          "2nnn", u == 0x2, )/*Execute subroutine starting at address NNN*/\
    o("SE Vx, byte",        "3xkk", u == 0x3, if (V[x] == nn)  PC += 2 )/*Skip the following instruction if the value of register VX equals NN*/\
    o("SNE Vx, byte",       "4xkk", u == 0x4, if (V[x] != nn)  PC += 2 )/*Skip the following instruction if the value of register VX is not equal to NN*/\
    o("SE Vx, Vy",          "5xy0", u == 0x5, if (V[x] == V[y] PC += 2 ))/*Skip the following instruction if the value of register VX is equal to the value 
                                                            of register VY*/\
    o("LD Vx, byte",        "6xkk", u == 0x6, V[x] = kk)/*Store number NN in register VX*/\
    o("ADD Vx, byte",       "7xkk", u == 0x7, V[x] += kk)/*Add the value NN to register VX*/\
    o("LD Vx, Vy",          "8xy0", u == 0x8 && n == 0x0, V[x] = V[y])/*Store the value of register VY in register VX*/\
    o("OR Vx, Vy",          "8xy1", u == 0x8 && n == 0x1, V[x] = V[x] | V[y])/*Set VX to VX OR VY*/\
    o("AND Vx, Vy",         "8xy2", u == 0x8 && n == 0x2, V[x] = V[x] & V[y])/*Set VX to VX AND VY*/\
    o("XOR Vx, Vy",         "8xy3", u == 0x8 && n == 0x3, V[x] = V[x] ^ V[y])/*Set VX to VX XOR VY*/\
    o("ADD Vx, Vy",         "8xy4", u == 0x8 && n == 0x4, )/*Add the value of register VY to register VX
                                                            Set VF to 01 if a carry occurs
                                                            Set VF to 00 if a carry does not occur*/\
    o("SUB Vx, Vy",         "8xy5", u == 0x8 && n == 0x5, )/*Subtract the value of register VY from register VX
                                                            Set VF to 00 if a borrow occurs
                                                            Set VF to 01 if a borrow does not occur*/\
    o("SHR Vx {, Vy}",      "8xy6", u == 0x8 && n == 0x6, )/*Store the value of register VY shifted right one bit in register VX¹
                                                            Set register VF to the least significant bit prior to the shift
                                                            VY is unchanged*/\
    o("SUBN Vx, Vy",        "8xy7", u == 0x8 && n == 0x7, )/*Set register VX to the value of VY minus VX
                                                            Set VF to 00 if a borrow occurs
                                                            Set VF to 01 if a borrow does not occur*/\
    o("SHL Vx {, Vy}",      "8xyE", u == 0x8 && n == 0xE, )/*Store the value of register VY shifted left one bit in register VX¹
                                                            Set register VF to the most significant bit prior to the shift
                                                            VY is unchanged*/\
    o("SNE Vx, Vy",         "9xy0", u == 0x9, )/*Skip the following instruction if the value of register VX is not equal to the 
                                                           value of register VY*/\
    o("LD I, addr",         "Annn", u == 0xA , I = nnn)/*Store memory address NNN in register I*/\
    o("JP V0, addr",        "Bnnn", u == 0xB, )/*Jump to address NNN + V0*/\
    o("RND Vx, byte",       "Cxkk",)/*Set VX to a random number with a mask of NN*/\
    o("DRW Vx, Vy, nibble", "Dxyn", u == 0xD, display(x, y, n, I)) /*Draw a sprite at position VX, VY with N bytes of sprite data starting at the 
                                                            address stored in I
                                                            Set VF to 01 if any set pixels are changed to unset, and 00 otherwise*/\
    o("SKP Vx",             "Ex9E", u == 0xE, )/*Skip the following instruction if the key corresponding to the hex value 
                                                           currently stored register VX is pressed*/\
    o("LD Vx, DT",          "Fx07", u == 0xF && kk == 0x07, )/*Store the current value of the delay timer in register VX*/\
    o("LD Vx, K",           "Fx0A", u == 0xF && kk == 0x0A, )/*Wait for a keypress and store the result in register VX*/\
    o("LD DT, Vx",          "Fx15", u == 0xF && kk == 0x15, )/*Set the delay timer to the value of register VX*/\
    o("LD ST, Vx",          "Fx18", u == 0xF && kk == 0x18, )/*Set the sound timer to the value of register VX*/\
    o("ADD I, Vx",          "Fx1E", u == 0xF && kk == 0x1E, V[x] += I)/*Add the value stored in register VX to register I*/\
    o("LD F, Vx",           "Fx29", u == 0xF && kk == 0x29, )/*Set I to the memory address of the sprite data corresponding to the hexadecimal 
                                                           digit stored in register VX*/\
    o("LD B, Vx",           "Fx33", u == 0xF && kk == 0x33, )/*Store the binary-coded decimal equivalent of the value stored in register VX at 
                                                            addresses I, I + 1, and I + 2*/\
    o("LD [I], Vx",         "Fx55", u == 0xF && kk == 0x55, )/*Store the values of registers V0 to VX inclusive in memory starting at address I
                                                            I is set to I + X + 1 after operation*/\
    o("LD Vx, [I]",         "Fx65", u == 0xF && kk == 0x65, )/*Fill registers V0 to VX inclusive with the values stored in memory starting at 
                                                           address I. I is set to I + X + 1 after operation*/

void decode (Chip8* vm) {
    vm->instr = vm->ram[vm->PC & 0xFFF]*0x100 + vm->ram[(vm->PC+1) & 0xFFF]; 

    unsigned u   = vm->instr & OP; // u - First 4 bits of instruction
    unsigned x   = vm->instr & Vx; // x - A 4-bit value, the lower 4 bits of the high byte of the instruction
    unsigned y   = vm->instr & Vy; // y - A 4-bit value, the upper 4 bits of the low byte of the instruction
    unsigned n   = vm->instr & N;  // n or nibble - A 4-bit value, the lowest 4 bits of the instruction
    unsigned kk  = vm->instr & NN; // kk or byte - An 8-bit value, the lowest 8 bits of the instruction
    unsigned nnn = vm->instr & NNN;// nnn or addr - A 12-bit value, the lowest 12 bits of the instruction

    #define o(mnemonic, hex, test, op) if (test) { op; } else
    INSTRUCTION_LIST(o) {}
    #undef o
}

void initVM (Chip8* vm, const char* ROMfilename) {
    // Initialize PC to point to the 0x200 position in RAM
    vm->PC = 0x200;
    loadROM(ROMfilename, vm);
}

int main(int argc, char** argv) {
    Chip8* vm;

    // Load ROM, set up registers and Program Counter
    initVM (vm, argv[1]);

    // Chip-8 Cycle
    for(;;)
    {
        /************** Fetch *********************/
        fetch (vm);
        /************** Decode ********************/
        decode (vm);
        /************** Execute *******************/
    }
}
