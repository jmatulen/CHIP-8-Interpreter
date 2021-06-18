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
    
    // Our 4k of ram
    uint16_t ram[4096];

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
    //	 point to the topmost level of the stack.
    
    //	 The stack is an array of 16 16-bit values, used to store the address that the
    //	 interpreter shoud return to when finished with a subroutine. Chip-8 allows for
    //	 up to 16 levels of nested subroutines.

    // Registers V0 -- VF
    uint8_t V[16];

    // 16 bit I register
    uint16_t I;

    // Program Counter
    uint16_t PC;

    // Current Instruction
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
        
        vector<char> buffer(fileSize);
        
        file.seekg(0, ios::beg);
        file.read(buffer.data(), fileSize);
        file.close();

        for (int i = 0; i < fileSize; i++) {
            vm->ram[i + 0x200] = buffer[i];
        }
    }
}
#define INSTRUCTION_LIST(o)\
    o("SYS addr"           "0nnn",)\
    o("CLS"                "00E0",)\
    o("RET"                "00EE",)\
    o("JP addr"            "1nnn",)\
    o("CALL addr"          "2nnn",)\
    o("SE Vx, byte"        "3xkk",)\
    o("SNE Vx, byte"       "4xkk",)\
    o("SE Vx, Vy"          "5xy0",)\
    o("LD Vx, byte"        "6xkk",)\
    o("ADD Vx, byte"       "7xkk",)\
    o("LD Vx, Vy"          "8xy0",)\
    o("OR Vx, Vy"          "8xy1",)\
    o("AND Vx, Vy"         "8xy2",)\
    o("XOR Vx, Vy"         "8xy3",)\
    o("ADD Vx, Vy"         "8xy4",)\
    o("SUB Vx, Vy"         "8xy5",)\
    o("SHR Vx {, Vy}"      "8xy6",)\
    o("SUBN Vx, Vy"        "8xy7",)\
    o("SHL Vx {, Vy}"      "8xyE",)\
    o("SNE Vx, Vy"         "9xy0",)\
    o("LD I, addr"         "Annn",)\
    o("JP V0, addr"        "Bnnn",)\
    o("RND Vx, byte"       "Cxkk",)\
    o("DRW Vx, Vy, nibble" "Dxyn",)\
    o("SKP Vx"             "Ex9E",)\
    o("LD Vx, DT"          "Fx07",)\
    o("LD Vx, K"           "Fx0A",)\
    o("LD DT, Vx"          "Fx15",)\
    o("LD ST, Vx"          "Fx18",)\
    o("ADD I, Vx"          "Fx1E",)\
    o("LD F, Vx"           "Fx29",)\
    o("LD B, Vx"           "Fx33",)\
    o("LD [I], Vx"         "Fx55",)\
    o("LD Vx, [I]"         "Fx65",)

void decode (Chip8* vm) {

    unsigned u   = vm->instr & OP; // u - First 4 bits of instruction
    unsigned x   = vm->instr & Vx; // x - A 4-bit value, the lower 4 bits of the high byte of the instruction
    unsigned y   = vm->instr & Vy; // y - A 4-bit value, the upper 4 bits of the low byte of the instruction
    unsigned n   = vm->instr & N;  // n or nibble - A 4-bit value, the lowest 4 bits of the instruction
    unsigned kk  = vm->instr & NN; // kk or byte - An 8-bit value, the lowest 8 bits of the instruction
    unsigned nnn = vm->instr & NNN;// nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
}

void initVM (Chip8* vm, const char* ROMfilename) {
    // Initialize PC to point to the 0x200 position in RAM
    vm->PC = 0x200;
    loadROM(ROMfilename, vm);
}

int main() {
    Chip8* vm;
    // Chip-8 Cycle
    for(;;)
    {
        /************** Fetch *********************/
        //************** Decode *********************//
        /************** Execute *********************/
    }
}
