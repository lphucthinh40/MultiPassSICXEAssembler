#include <bits/stdc++.h>

#include "utils.h"

struct Instruction {
    std::string label;
    std::string opcode;
    std::string operand;
    int address = -1;
    int objectCode = -1;

    static Instruction parse(std::string str) {
        if (str.empty() || str[0] == '.') {
            return {"", "", ""};
        }
        std::vector<std::string> tokens = split(str, " ");
        if (tokens.size() == 3) {
            return {tokens[0], tokens[1], tokens[2]};
        }
        else if (tokens.size() == 2) {
            return {"", tokens[0], tokens[1]};
        }
        else if (tokens.size() == 1) {
            return {"", tokens[0], ""};
        }
        else {
            return {"", "", ""};
        }
    }
};

struct Format1 {
    unsigned int _ : 24;
    unsigned int opcode : 8;
    Format1(int opcode=0) : opcode(opcode), _(0) {};
};

struct Format2 {
    unsigned int _ : 16;
    unsigned int opcode : 8;
    unsigned int r1 : 4;
    unsigned int r2 : 4;
    Format2(int opcode=0, int r1=0, int r2=0) : opcode(opcode), r1(r1), r2(r2), _(0) {};
};

struct Format3 {
    unsigned int _ : 8;
    unsigned int opcode : 6;
    unsigned int nixbpe : 6;
    unsigned int disp : 12;
    Format3(int opcode=0, int nixbpe=0, int disp=0) : opcode(opcode), nixbpe(nixbpe), disp(disp), _(0) {};
};

struct Format4 {
    unsigned int opcode : 6;
    unsigned int nixbpe : 6;
    unsigned int disp : 20;
    Format4(int opcode=0, int nixbpe=0, int disp=0) : opcode(opcode), nixbpe(nixbpe), disp(disp) {};
};

enum Flag {
    N = 0,
    I = 1,
    X = 2,
    B = 3,
    P = 4,
    E = 5,
};

union ObjectCode {
    uint32_t code;
    Format1 f1;
    Format2 f2;
    Format3 f3;
    Format4 f4;
    ObjectCode(int code=0) : code(code) {};
    ObjectCode(Format1 f1) : f1(f1) {};
    ObjectCode(Format2 f2) : f2(f2) {};
    ObjectCode(Format3 f3) : f3(f3) {};
    ObjectCode(Format4 f4) : f4(f4) {};
    ~ObjectCode(){};
};

struct OpcodeEntry{
	std::string mnemonic;
	int format;
	int opcode;

    static OpcodeEntry parse(std::string str) {
        std::vector<std::string> tokens = split(str, " ");
        if (tokens.size() == 3) {
            return {tokens[0], std::stoi(tokens[1]), (int)std::strtol(&tokens[2][0], NULL, 16)};
        }
        else {
            return {"",0,0};
        }
    }
};

std::ostream& operator << (std::ostream& out,const OpcodeEntry &op);

std::ostream& operator << (std::ostream& out,const Instruction &instr);

struct SymbolEntry{
    std::string label;
	int value;
};

struct Error {
    int lineNumber;
    int passNumber;
    std::string instruction;
    std::string message;
};