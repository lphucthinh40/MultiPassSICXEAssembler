#include <bits/stdc++.h>
#include "structs.h"
#include "utils/utils.h"

using namespace std;

class MultiPassAssembler
{
private:
    // Opcode Table
    map<string, OpcodeEntry> OPTAB;
    // Symbol Table
    map<string, SymbolEntry> SYMTAB;
    // Registers
    map<char,int> REGISTERS = {
        {'A',0},
        {'X',1},
        {'L',2},
        {'B',3},
        {'S',4},
        {'T',5},
        {'F',6}
    };
    // Outputs
    vector<Instruction> intermediate;
    vector<Error> errors;
    // Location Counter
    int LOCCTR = 0;
    // Program Length
    int LENGTH = 0;
    // Base Register Value (for BASE & NOBASE directive)
    int PC = 0;
    int BASE = 0;
    
    // Process an instruction & generate its object code (For Pass 2)
    ObjectCode processFormat1(Instruction &instr);
    ObjectCode processFormat2(Instruction &instr);
    ObjectCode processFormat34(Instruction &instr);

    // Check whether address is pc-relative or base-relative, and calculate address displacement
    int getAddressOffset(int address, bool &usePC, bool &useBASE, bool &useExt);

    int getObjectCode(Instruction &inst);

public:
    MultiPassAssembler();
    ~MultiPassAssembler();
    void translate(string filename);
};