#include "assembler.h"

MultiPassAssembler::MultiPassAssembler()
{
    // initialize opcode table
    string line;
    ifstream input("assembler/optab.txt") ;
    while (getline(input, line))
    {   OpcodeEntry entry = OpcodeEntry::parse(line);
        if (!entry.mnemonic.empty()) {
            OPTAB[entry.mnemonic] = entry;
            // cout << entry;
        }
    }
    cout << "OPTAB initialized with " << OPTAB.size() << " entries" << endl;
}

MultiPassAssembler::~MultiPassAssembler()
{
}

int MultiPassAssembler::getAddressOffset(int address, bool &usePC, bool &useBASE, bool &useExt)
{   
    // PC relative
    if (address - PC > -2048 && address - PC < 2047)
    {   
        std::cout << "use PC: " << address << " - " << PC << std::endl;
        usePC = true;
        useBASE = false;
        useExt = false;
        return address - PC;
    }
    // BASE relative
    else if (address - BASE > 0 && address - BASE < 4095)
    {
        // std::cout << "use BASE: " << address << " - " << BASE << std::endl;
        usePC = false;
        useBASE = true;
        useExt = false;
        return address - BASE;
    }
    // Use Extended Format (added just for precaution, this should not happen)
    else {
        // std::cout << "use Extended: " << address << " - " << BASE << std::endl;
        usePC = false;
        useBASE = false;
        useExt = true;
        return address;
    }
}


ObjectCode MultiPassAssembler::processFormat1(Instruction &inst)
{
    return ObjectCode(Format1(OPTAB[inst.opcode].opcode));
}

ObjectCode MultiPassAssembler::processFormat2(Instruction &inst)
{   
    vector<string> r1r2 = split(inst.operand, ",");
    // For instructions with two operands (R1,R2) || Note: R2 maybe a number (check SHIFTL)
    if (r1r2.size() == 2 && REGISTERS.find(r1r2[0][0])!=REGISTERS.end()){
        // R2 is a REGISTER
        if (REGISTERS.find(r1r2[1][0])!=REGISTERS.end()) {
            return ObjectCode(Format2(OPTAB[inst.opcode].opcode, REGISTERS[r1r2[0][0]], REGISTERS[r1r2[1][0]]));            
        }
        // R2 is a number
        else if (is_number(r1r2[1])) {
            int r2 = (int)std::strtol(&r1r2[1][0], NULL, 10) - 1; // check SHIFTL
            return ObjectCode(Format2(OPTAB[inst.opcode].opcode, REGISTERS[r1r2[0][0]], r2));
        }
        // Otherwise return FFFFFFFF
        else {
            return ObjectCode(-1);
        }
    }
    // For instructions with one operand (R1) || Note: R1 maybe a number (check SVC)
    else if (r1r2.size() == 1) {      
        // R1 is a REGISTER
        if (REGISTERS.find(r1r2[0][0])!=REGISTERS.end())
            return ObjectCode(Format2(OPTAB[inst.opcode].opcode, REGISTERS[r1r2[0][0]], 0));    
        // R1 is a number
        else if (is_number(r1r2[0])) {
            int r1 = (int)std::strtol(&r1r2[1][0], NULL, 10);
            return ObjectCode(Format2(OPTAB[inst.opcode].opcode, REGISTERS[r1r2[0][0]], 0));
        }
        // Otherwise return FFFFFFFF
        else {
            return ObjectCode(-1);
        }        
    }
    return ObjectCode(-1);
}

ObjectCode MultiPassAssembler::processFormat34(Instruction &inst)
{   
    bool flags[6] = {0}; // nixbpe
    bool isSymbol = false;
    string opcode_str;
    string address_str;
    int address, disp;
    // check format type (3 or 4)
    if (inst.opcode[0] == '+') {
        flags[Flag::E] = true;
        opcode_str = inst.opcode.substr(1); // remove '+'
    }
    else {
        flags[Flag::E] = false;
        opcode_str = inst.opcode;
    }
    // check addressing types
    if (inst.operand.empty()) {
        // RSUB instruction has no operand
        flags[Flag::N] = true;
        flags[Flag::I] = true;
        address_str = "0"; // quick fix for now
    }
    else if (inst.operand[0] == '#')
    {   // immediate addressing mode
        flags[Flag::N] = false;
        flags[Flag::I] = true;
        flags[Flag::X] = false;
        address_str = inst.operand.substr(1); // remove '#'     
    }
    else if (inst.operand[0] == '@')
    {   // indirect addressing mode
        flags[Flag::N] = true;
        flags[Flag::I] = false;
        flags[Flag::X] = false;
        address_str = inst.operand.substr(1); // remove '@'
    }      
    else
    {   // simple addressing types (we only use SIC/XE addressing modes)
        flags[Flag::N] = true;
        flags[Flag::I] = true;
        // check if this is index addressing
        if (inst.operand.substr(inst.operand.length()-2)==",X") {
            flags[Flag::X] = true;
            address_str = inst.operand.substr(0, inst.operand.length()-2); // remove ',X'
            std::cout << "->" << inst.operand << "<-" << std::endl;
            std::cout << "->" << address_str << "<-" << std::endl;
        } else {
            flags[Flag::X] = false;
            address_str = inst.operand;
        }
    }
    // check if address is a SYMBOL or CONSTANT
    if (SYMTAB.find(address_str) != SYMTAB.end())
        {   // SYMBOL -> lookup SYMTAB
            address = SYMTAB[address_str].value;
            isSymbol = true;
        }
        else
        {   // CONSTANT -> keep it as is
            address = std::stol(address_str, NULL, 10);
        }
    // generate object code
    if (flags[Flag::E]) {
        // for format 4, there is no need to calculate address displacement
        return ObjectCode(Format4(OPTAB[opcode_str].opcode >> 2, flags_to_int(flags, 6), address));
    }
    else {
        // get address displacement and update P,B,E flags (PC-relative, Base-relative, or Extended)
        if (isSymbol) {
            disp = getAddressOffset(address, flags[Flag::P], flags[Flag::B], flags[Flag::E]);
        } else {
            disp = address;
        }
        return ObjectCode(Format3(OPTAB[opcode_str].opcode >> 2, flags_to_int(flags, 6), disp));
    }
}

int MultiPassAssembler::getObjectCode(Instruction &inst)
{   
    if (OPTAB[inst.opcode].format == 1) {
        return processFormat1(inst).code;
    }
    else if (OPTAB[inst.opcode].format == 2) {
        return processFormat2(inst).code;
    }
    else if (OPTAB[inst.opcode].format == 3 || OPTAB[inst.opcode.substr(1)].format == 3) {
        return processFormat34(inst).code;
    }
    return -1;   
}



void MultiPassAssembler::translate(string filename) {
    int lineCounter = 0;
    // Reset Location Counter
    LOCCTR = 0;
    // PASS 1
    std::cout << std::endl << "***************** PASS 1 ********************" << std::endl << std::endl;
    string line;
    ifstream input(filename) ;
    while (getline(input, line))
    {   Instruction instruction = Instruction::parse(line);
        if (!instruction.opcode.empty()) {
            // Step 1: Assign current address to this instruction
            instruction.address = LOCCTR;
            // Step 2: Add label to SYMTAB (if any)
            if (!instruction.label.empty()) {
                if (SYMTAB.find(instruction.label) == SYMTAB.end()) {
                    SYMTAB[instruction.label] = {instruction.label, LOCCTR};
                }
                else {
                    errors.push_back((Error){lineCounter, 1, line, "Duplicate Labels"});
                }
            }
            // Step 3: Process opcode field
            if (instruction.opcode == "START") {
                // - update location counter
                LOCCTR = (int)std::strtol(&instruction.operand[0], NULL, 16);
                instruction.address = LOCCTR; // assign starting address to START instruction
            }
            else if (instruction.opcode == "END") {
                // - save program length then break loop
                LENGTH = LOCCTR;
                intermediate.push_back(instruction);
                break;
            }
            else if (instruction.opcode == "WORD") {
                // - handle WORD directive   
                LOCCTR += 3;
            }
            else if (instruction.opcode == "RESW") {
                // - handle RESW directive
                LOCCTR += std::stoi(instruction.operand) * 3;
            }
            else if (instruction.opcode == "RESB") {
                // - handle RESB directive
                LOCCTR += std::stoi(instruction.operand);
            }
            else if (instruction.opcode == "BYTE") {
                // - handle BYTE directive
                LOCCTR += (instruction.operand[0]=='X')? 1 : 
                            (instruction.operand[0]=='C')? instruction.operand.size() - 3 :
                                0;
            }
            else if (instruction.opcode[0] == '+') {
                // - handle Extended Format (Format 4)
                LOCCTR += 4;
            }
            else if (OPTAB.find(instruction.opcode) != OPTAB.end())
            {   // - handle opcode that belongs to OPTAB
                LOCCTR += OPTAB[instruction.opcode].format;
            }
            // Step 4: Save to intermediate file            
            intermediate.push_back(instruction);
            // update line counter
            lineCounter++;
        }
    }

    // print symbol table
    std::cout << "SYMBOL TABLE -> " << SYMTAB.size() << "entries" << std::endl;
    std::cout << std::setw(10) << "Label" << " | " << std::setw(10) << "Value" << std::endl;

    for (auto entry : SYMTAB) {
        std::cout << std::setw(10) << entry.first << " | " << std::setw(10) << entry.second.value << std::endl;
    }

    for (auto instr : intermediate) {
        std::cout << instr;
    }

    // PASS 2
    std::cout << std::endl << "***************** PASS 2 *********************" << std::endl << std::endl;

    int i,j;
    for (i = 0; i<intermediate.size(); i++)
    // for (i = 0; i<20; i++)
    {   
        Instruction* inst = &intermediate[i];
        for (j = i; j < intermediate.size(); j++)
        // for (j = i; i<20; j++)
        {
            if (intermediate[j].address != inst->address)
                break;
        }
        PC = intermediate[j].address;

        int objectCode; // object code of this instruction
        if (inst->opcode == "BASE")
        {   
            BASE = SYMTAB[inst->operand].value;
        }
        else if (inst->opcode == "NOBASE")
        {
            BASE = 0;
        }
        else if (OPTAB.find(inst->opcode) != OPTAB.end() || OPTAB.find(inst->opcode.substr(1)) != OPTAB.end())
        {   // - handle opcode that belongs to OPTAB
            inst->objectCode = getObjectCode(*inst);
        }
    }

    // simply print instructions with object code
    for (auto instr : intermediate) {
        std::cout << instr;
    }
}
