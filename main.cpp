#include "assembler.h"

int main() {
    MultiPassAssembler assembler = MultiPassAssembler();
    assembler.translate("input.txt");
}