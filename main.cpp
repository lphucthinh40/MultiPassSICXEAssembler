#include "assembler/assembler.h"

int main() {
    MultiPassAssembler assembler = MultiPassAssembler();
    assembler.translate("test/sample_input.txt");
}