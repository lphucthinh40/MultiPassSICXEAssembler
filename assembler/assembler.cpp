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
    if (address - PC >= -2048 && address - PC <= 2047)
    {   
        // std::cout << "use PC: " << address << " - " << PC << std::endl;
        usePC = true;
        useBASE = false;
        useExt = false;
        return address - PC;
    }
    // BASE relative
    else if (address - BASE >= 0 && address - BASE <= 4095)
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

int MultiPassAssembler::processByteOperand(string operand, int* byteSize=nullptr)
{
    if (!operand.empty()) {
                int operand_len = operand.length();
                if (operand_len>3) { 
                    char type = operand[0];
                    string value = operand.substr(2, operand_len-3);
                    int len = value.length();
                    if (type == 'X' && value.length() <= 8) {       
                        if (byteSize != nullptr) *byteSize = len / 2;            
                        return std::stoul(value, nullptr, 16);
                    } else if (type == 'C' && value.length() <= 4) {
                        int result = 0;
                        if (byteSize != nullptr) *byteSize = len;
                        for (int i = 0; i<len; i++) {
                            result = result + (int(value[i]) << ((len-i-1)*8));
                        }
                        return result;
                    }
                }
            }
    return -1;
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

ObjectCode MultiPassAssembler::processFormat3(Instruction &inst)
{   
    bool flags[6] = {0}; // nixbpe
    bool isSymbol = false;
    bool isLiteral = false;
    string opcode_str;
    string address_str;
    int address, disp;
    // format 3 - no extended format 
    flags[Flag::E] = false;
    opcode_str = inst.opcode;
    PC = BLOCKTAB[inst.block_id].address + inst.address + 3;
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
            // std::cout << "->" << inst.operand << "<-" << std::endl;
            // std::cout << "->" << address_str << "<-" << std::endl;
        } else {
            flags[Flag::X] = false;
            address_str = inst.operand;
        }
    }
    // check if address is a SYMBOL or CONSTANT
    if (SYMTAB.find(address_str) != SYMTAB.end())
        {   // SYMBOL -> lookup SYMTAB  
            address = ((SYMTAB[address_str].isAbsolute)? 0 : BLOCKTAB[SYMTAB[address_str].blockId].address) + SYMTAB[address_str].value;
            isSymbol = true;
        }
    else if (LITTAB.find(address_str) != LITTAB.end())
        {   // LITERAL -> lookup LITTAB
            address = BLOCKTAB[LITTAB[address_str].blockId].address + LITTAB[address_str].address;
            isLiteral = true;
        }
    else
    {   // CONSTANT -> keep it as is
        address = std::stol(address_str, NULL, 10);
    }
    // get address displacement and update P,B,E flags (PC-relative, Base-relative, or Extended)
    if (isSymbol || isLiteral) {
        disp = getAddressOffset(address, flags[Flag::P], flags[Flag::B], flags[Flag::E]);
    } else {
        disp = address;
    }
    return ObjectCode(Format3(OPTAB[opcode_str].opcode >> 2, flags_to_int(flags, 6), disp));
}

ObjectCode MultiPassAssembler::processFormat4(Instruction &inst)
{   
    bool flags[6] = {0}; // nixbpe
    bool isSymbol = false;
    bool isLiteral = false;
    string opcode_str;
    string address_str;
    int address, disp;
    // check format type (3 or 4)
    flags[Flag::E] = true;
    opcode_str = inst.opcode.substr(1); // remove '+'
    PC = BLOCKTAB[inst.block_id].address + inst.address + 4;
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
            // std::cout << "->" << inst.operand << "<-" << std::endl;
            // std::cout << "->" << address_str << "<-" << std::endl;
        } else {
            flags[Flag::X] = false;
            address_str = inst.operand;
        }
    }
    // check if address is a SYMBOL or CONSTANT
    if (SYMTAB.find(address_str) != SYMTAB.end())
        {   // SYMBOL -> lookup SYMTAB
            address = ((SYMTAB[address_str].isAbsolute)? 0 : BLOCKTAB[SYMTAB[address_str].blockId].address) + SYMTAB[address_str].value;
            isSymbol = true;
        }
    else if (LITTAB.find(address_str) != LITTAB.end())
        {   // LITERAL -> lookup LITTAB
            address = BLOCKTAB[LITTAB[address_str].blockId].address + LITTAB[address_str].address;
            isLiteral = true;
        }
    else
        {   // CONSTANT -> keep it as is
            address = std::stol(address_str, NULL, 10);
        }
    // for format 4, there is no need to calculate address displacement
    return ObjectCode(Format4(OPTAB[opcode_str].opcode >> 2, flags_to_int(flags, 6), address));
    
}


int MultiPassAssembler::getObjectCode(Instruction &inst, int* byteSize = nullptr)
{   
    if (OPTAB[inst.opcode].format == 1) {
        if (byteSize != nullptr) *byteSize = 1;
        return processFormat1(inst).code;
    }
    else if (OPTAB[inst.opcode].format == 2) {
        if (byteSize != nullptr) *byteSize = 2;
        return processFormat2(inst).code;
    }
    else if (OPTAB[inst.opcode].format == 3) {
        if (byteSize != nullptr) *byteSize = 3;
        return processFormat3(inst).code;
    }
    else if (!inst.opcode.empty() && inst.opcode[0]=='+' && OPTAB[inst.opcode.substr(1)].format == 3) {
        if (byteSize != nullptr) *byteSize = 4;
        return processFormat4(inst).code;
    }
    return -1;   
}

int MultiPassAssembler::processExpression(const string expression, bool &isAbsolute, vector<string>& unresolved_labels) {
    // Assume there are only 2 possibilities: LABEL or LABEL1|Operator(+,-,/,*)|LABEL2
    vector<char> operators;
    int operator_idx = -1;
    vector<string> operands = splitMultiDelims(expression, operators, "+-/*");
    // Checking if all operands are available
    int result = 0;
    bool isResolved = true;
    for (string operand : operands) {
        // Checking if operand is number
        int value = 0;
        try {
            value = std::stoi(operand);
        } catch (...) {
            if (SYMTAB.find(operand) != SYMTAB.end()) {
                value = SYMTAB[operand].value;
            } else {
                unresolved_labels.push_back(operand);
                isResolved = false;
            }            
        }
        if (isResolved) {
            if (operator_idx == -1)
                result = value;
            else if (operator_idx > -1) {
                result = calculate(result, value, operators[operator_idx]);
                if (operators[operator_idx]=='-') isAbsolute = true;
            }
            operator_idx++;
        }
    }
    return result;    
}

void MultiPassAssembler::translate(string filename) {
    int lineCounter = 0;
    bool isSymtabComplete = true;
    
    // Initialize Default Program Block
    blockNameToID[""] = 0;
    BLOCKTAB.push_back({0, "", 0, 0});
    BLOCKID = 0;
    LOCCTRs.push_back(0);

    // PASS 1
    std::cout << std::endl << "***************** PASS 1 ********************" << std::endl << std::endl;
    string line;
    ifstream input(filename) ;
    while (getline(input, line))
    {   Instruction instruction = Instruction::parse(line);
        if (!instruction.opcode.empty()) {
            // Step 1: Assign current address to this instruction
            instruction.block_id = BLOCKID;
            instruction.address = LOCCTRs[BLOCKID];
            // Step 2: Add label to SYMTAB (if any)
            if (!instruction.label.empty()) {
                if (SYMTAB.find(instruction.label) == SYMTAB.end()) {
                    if (instruction.opcode == "EQU") {
                        if (instruction.operand == "*") {
                            SYMTAB[instruction.label] = {instruction.label, LOCCTRs[BLOCKID], true};
                        } else {
                        vector<string> unresolved_labels;
                        bool isAbsolute = false;
                        int value = processExpression(instruction.operand, isAbsolute, unresolved_labels);
                        if (unresolved_labels.empty()) 
                            {   // Expression is resolved
                                SYMTAB[instruction.label] = {instruction.label, value , BLOCKID, isAbsolute, true};
                            } else {
                                isSymtabComplete = false;
                            }
                        }
                    } else {
                        SYMTAB[instruction.label] = {instruction.label, LOCCTRs[BLOCKID], BLOCKID};
                    }
                }
                else {
                    errors.push_back((Error){lineCounter, 1, line, "Duplicate Labels"});
                }
            }
            // Step 3: Process opcode field
            if (instruction.opcode == "START") {
                // - update location counter
                int address = (int)std::strtol(&instruction.operand[0], NULL, 16);
                if (address != 0) {
                    BLOCKTAB[0].address = address;
                }
                instruction.address = LOCCTRs[BLOCKID]; // assign starting address to START instruction
            }
            else if (instruction.opcode == "END") {
                // push END instruction to intermediate file
                intermediate.push_back(instruction);
                // generate literal pool if necessary
                while (!literal_queue.empty()) {
                    string literal_name = literal_queue.front();
                    LITTAB[literal_name].address = LOCCTRs[BLOCKID];
                    Instruction inst = {"", literal_name, "", 0, LOCCTRs[BLOCKID], BLOCKID, -1};
                    LOCCTRs[BLOCKID] += LITTAB[literal_name].length;
                    intermediate.push_back(inst);
                    literal_queue.pop();
                }
                break;
            }
            else if (instruction.opcode == "USE") {
                string block_name = instruction.operand;
                // update block table and location counter
                if (blockNameToID.find(block_name) == blockNameToID.end()) {
                    // if program block not exist, add it to BLOCKTAB
                    blockNameToID[block_name] = BLOCKTAB.size();
                    BLOCKTAB.push_back({(int)BLOCKTAB.size(), block_name, 0, 0});
                    LOCCTRs.push_back(0);
                }
                BLOCKID = blockNameToID[block_name];
                // update address & block id of this instruction
                instruction.block_id = BLOCKID;
                instruction.address = LOCCTRs[BLOCKID];
            }
            else if (instruction.opcode == "WORD") {
                // - handle WORD directive   
                LOCCTRs[BLOCKID] += 3;
            }
            else if (instruction.opcode == "RESW") {
                // - handle RESW directive
                LOCCTRs[BLOCKID] += std::stoi(instruction.operand) * 3;
            }
            else if (instruction.opcode == "RESB") {
                // - handle RESB directive
                LOCCTRs[BLOCKID] += std::stoi(instruction.operand);
            }
            else if (instruction.opcode == "BYTE") {
                // - handle BYTE directive
                LOCCTRs[BLOCKID] += (instruction.operand[0]=='X')? 1 : 
                            (instruction.operand[0]=='C')? instruction.operand.size() - 3 :
                                0;
            }
            else if (instruction.opcode == "LTORG") {
                // Generate Literal Pool after LTORG directive
                while (!literal_queue.empty()) {
                    string literal_name = literal_queue.front();
                    LITTAB[literal_name].address = LOCCTRs[BLOCKID];
                    LITTAB[literal_name].blockId = BLOCKID;
                    Instruction inst = {"", literal_name, "", 0, LOCCTRs[BLOCKID], BLOCKID, -1};
                    LOCCTRs[BLOCKID] += LITTAB[literal_name].length;
                    intermediate.push_back(inst);
                    literal_queue.pop();
                }
                lineCounter++;
                continue;
            }
            else if (instruction.opcode[0] == '+') {
                // - handle Extended Format (Format 4)
                LOCCTRs[BLOCKID] += 4;
            }
            else if (OPTAB.find(instruction.opcode) != OPTAB.end())
            {   // - handle opcode that belongs to OPTAB
                LOCCTRs[BLOCKID] += OPTAB[instruction.opcode].format;
            }
            // Step 4: Process operands
            // Literals
            
            if ((instruction.operand.substr(0,3)=="=X'" || instruction.operand.substr(0,3)=="=C'") && (instruction.operand.substr(instruction.operand.length()-1)=="'")) {
                string name = instruction.operand;
                int length = 0;
                int value = processByteOperand(name.substr(1), &length);
                if (LITTAB.find(name) == LITTAB.end()) {
                    LITTAB[name] = {name, value, length, -1};
                    literal_queue.push(name);
                }
            }
            // Step 5: Save to intermediate file            
            intermediate.push_back(instruction);
            // update line counter
            lineCounter++;
        }
    }

    // print symbol table
    std::cout << "SYMBOL TABLE -> " << SYMTAB.size() << "entries" << std::endl;
    std::cout << std::setw(10) << "Label" << " | " << std::setw(10) << "Value" << std::endl;

    for (auto entry : SYMTAB) {
        std::cout << std::setw(10) << entry.first << " | " << std::setw(10) << entry.second.blockId << " | " << std::setw(10) << entry.second.value << std::endl;
    }

    for (auto instr : intermediate) {
        std::cout << instr;
    }

     for (auto entry : LITTAB) {
        std::cout << std::setw(10) << " |" << entry.first << "| " << std::setw(10) << std::hex << entry.second.address << std::dec << std::endl;
    }

    // UPDATE PROGRAM BLOCK TABLE
    BLOCKTAB[0].length = LOCCTRs[0];
    for (int t=1; t<BLOCKTAB.size(); t++) {
        BLOCKTAB[t].address = BLOCKTAB[t-1].address + BLOCKTAB[t-1].length;
        BLOCKTAB[t].length = LOCCTRs[t];
    }
    LENGTH = BLOCKTAB[BLOCKTAB.size()-1].address + BLOCKTAB[BLOCKTAB.size()-1].length;
    for (auto block : BLOCKTAB) {
        std::cout << std::setw(10) << block.name << std::setw(10) << block.id << std::hex << std::setw(10) << block.address << std::setw(10) << block.length << std::dec << endl;
    }

    // MIDDLE PASSES
    if (!isSymtabComplete) {
        std::cout << std::endl << "***************** MIDDLE PASSES *********************" << std::endl << std::endl;
    }
    const int MAX_ITERATIONS = 50;
    int iter_counter = 0;
    while (!isSymtabComplete) {
        if (iter_counter == MAX_ITERATIONS) {
            cout << "Unable to resolve all labels in SYMTAB after " << MAX_ITERATIONS << " middle passes." << endl;
            break;
        }
        isSymtabComplete = true;
        for (Instruction instruction : intermediate) {
            if ((!instruction.label.empty()) 
                    && instruction.opcode == "EQU"
                        && SYMTAB.find(instruction.label) == SYMTAB.end()) {
                vector<string> unresolved_labels;
                bool isAbsolute = false;
                int value = processExpression(instruction.operand, isAbsolute, unresolved_labels);
                if (unresolved_labels.empty()) {
                    // Expression is resolved
                    SYMTAB[instruction.label] = {instruction.label, value , instruction.block_id, isAbsolute, true};
                } else {
                    isSymtabComplete = false;
                }
            }            
        }

    }

    if (isSymtabComplete) {

    for (auto entry : SYMTAB) {
        std::cout << std::setw(10) << entry.first << " | " << std::setw(10) << entry.second.blockId << " | " << std::setw(10) << entry.second.value << std::endl;
    }

    // PASS 2
    std::cout << std::endl << "***************** PASS 2 *********************" << std::endl << std::endl;

    int i;
    for (i = 0; i<intermediate.size(); i++)
    // for (i = 0; i<20; i++)
    {   
        Instruction* inst = &intermediate[i];
        // cout << "2nd pass inst: " << *inst << "check: " << (inst->opcode[0] == '=') << '-' << (LITTAB.find(inst->opcode.substr(1)) != LITTAB.end()) << endl;

        int objectCode; // object code of this instruction
        if (inst->opcode == "BASE")
        {   
            BASE = BLOCKTAB[SYMTAB[inst->operand].blockId].address +  SYMTAB[inst->operand].value;
        }
        else if (inst->opcode == "NOBASE")
        {
            BASE = 0;
        }
        else if (inst->opcode == "BYTE") {
            inst->objectCode = processByteOperand(inst->operand, &inst->objectCodeSize);
        }
        else if (inst->opcode[0] == '=' && LITTAB.find(inst->opcode) != LITTAB.end()) {
            string literal_name = inst->opcode;
            inst->objectCode = LITTAB[literal_name].value;
            inst->objectCodeSize = LITTAB[literal_name].length;
        }
        else if (OPTAB.find(inst->opcode) != OPTAB.end() || OPTAB.find(inst->opcode.substr(1)) != OPTAB.end())
        {   // - handle opcode that belongs to OPTAB
            inst->objectCode = getObjectCode(*inst, &inst->objectCodeSize);
        }
    }

    // simply print instructions with object code
    cout<< std::setw(4) << "LOC/BLOCK" << std::setfill(' ') << std::setw(10) << "LABEL" << std::setw(10) << "OPCODE" << std::setw(20) << "OPERAND" << std::setw(10) << "OJBECT" << std::setw(5) << "SIZE" << endl << endl;
    for (auto instr : intermediate) {
        std::cout << instr;
    }

    }
    else {
        cout << "Unable to complete symbol table. Translation stopped at middle step." << endl;
    }
}
