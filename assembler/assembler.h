#include <bits/stdc++.h>
#include "structs.h"
#include "utils/utils.h"

using namespace std;

class MultiPassAssembler
{
private:
    // Opcode Table
    unordered_map<string, OpcodeEntry> OPTAB;
    // Symbol Table
    unordered_map<string, SymbolEntry> SYMTAB;
    // Literal Table
    unordered_map<string, LiteralEntry> LITTAB;
    queue<string> literal_queue;
    // Program Block Table
    vector<ProgramBlockEntry> BLOCKTAB;
    vector<int> LOCCTRs;
    unordered_map<string, int> blockNameToID;
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
    int BLOCKID = 0;
    // Program Length
    int LENGTH = 0;
    // Base Register Value (for BASE & NOBASE directive)
    int PC = 0;
    int BASE = 0;

    private:
        // Process an instruction & generate its object code (For Pass 2)
        ObjectCode processFormat1(Instruction &instr);
        ObjectCode processFormat2(Instruction &instr);
        ObjectCode processFormat3(Instruction &instr);
        ObjectCode processFormat4(Instruction &instr);
        int processByteOperand(string operand, int* byteSize);
        int processExpression(const string expression, bool &isAbsolute, vector<string>& unresolved_labels);
        // Check whether address is pc-relative or base-relative, and calculate address displacement
        int getAddressOffset(int address, bool &usePC, bool &useBASE, bool &useExt);
        // Generate object code based on instruction format (support 4 instruction formats)
        int getObjectCode(Instruction &inst, int* byteSize);

    public:
        MultiPassAssembler();
        ~MultiPassAssembler();
        void translate(string filename);
};