#include "structs.h"

std::ostream& operator << (std::ostream& out,const OpcodeEntry &op){
	out<< op.mnemonic << "(" << op.format << ",0x" << std::hex << op.opcode << std::dec << ")" << std::endl;
	return out;
}

std::ostream& operator << (std::ostream& out,const Instruction &instr){
	out<< std::setw(4) << std::setfill('0') << std::hex << instr.address << std::dec << std::setfill(' ') << std::setw(10) << instr.label << std::setw(10) << instr.opcode << std::setw(10) << instr.operand;
	if (instr.objectCode!=-1)
		out << std::setw(10) << std::hex << instr.objectCode << std::dec << std::endl;
	else
		out << std::endl;	
	return out;
}