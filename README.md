# Multi-Pass SIC/XE Assembler
A very basic sic/xe assembler created for CMPE 220 Project

build:
```
g++ -I. -o output  main.cpp assembler\assembler.cpp assembler\structs.cpp utils\utils.cpp
```
run:
```
./output
```
---
**Todos:**
- [x] Handle 4 instruction formats
- [x] Handle all addressing modes
- [ ] Add Makefile
- [ ] Generate object code for BYTE & WORD directive
- [ ] Perform Error Detections
- [ ] Generate Object File
- [ ] Program Relocation
- [ ] Literals
- [ ] Symbol-defined Statements
- [ ] Program Block
- [ ] Control Section
