#include "core/cpu.hpp"

#include "util/util.hpp"

#include <fstream>
#include <iostream>

const std::array<CPU::Opcode, CPU::MAX_NUM_OPCODES> CPU::lookup = CPU::initLookup();

void CPU::initCPU() {
    // Init registers
    a = 0;
    x = 0;
    y = 0;
    sp = 0;
    sr = 0;
    setFlag(Flag::UNUSED, 1);

    totalCycles = 0;

    reset();
}

void CPU::setBus(Bus* bus) {
    this->bus = bus;
}

void CPU::executeCycle() {
    if (remainingCycles == 0) {
        // By default, we should advance the program counter to the next instruction.
        // However certian instructions (e.g. jumps and breaks) instead set the program counter directly.
        // Those instructions should set shouldAdvancePC to false.
        shouldAdvancePC = true;

        uint8_t index = bus->read(pc);
        const Opcode& currentOpcode = lookup[index];
        const Instruction& inst = currentOpcode.instruction;
        const AddressingMode& mode = currentOpcode.addressingMode;
        const AddressingMode::ReturnType& operand = (this->*mode.getOperand)();

        (this->*inst.execute)(operand);

        if (shouldAdvancePC) {
            pc += mode.instructionSize;
        }

        remainingCycles += currentOpcode.numDefaultCycles;

        bool needExtraCycle = operand.mightNeedExtraCycle && inst.mightNeedExtraCycle;
        remainingCycles += needExtraCycle;
    }

    remainingCycles--;
    totalCycles++;
}

// Reset (description from from https://www.masswerk.at/6502/6502_instruction_set.html)
// An active-low reset line allows to hold the processor in a known disabled
// state, while the system is initialized. As the reset line goes high, the
// processor performs a start sequence of 7 cycles, at the end of which the
// program counter (PC) is read from the address provided in the 16-bit reset
// vector at $FFFC (LB-HB). Then, at the eighth cycle, the processor transfers
// control by performing a JMP to the provided address.
// Any other initializations are left to the thus executed program. (Notably,
// instructions exist for the initialization and loading of all registers, but
// for the program counter, which is provided by the reset vector at $FFFC.)
void CPU::reset() {
    // Reset program counter
    pc = read16BitData(RESET_VECTOR);

    // Stack pointer is decremented by 3 for some reason
    sp -= 3;

    // Set I flag
    setFlag(Flag::INTERRUPT, 1);

    // Reset takes 8 cycles
    remainingCycles = 8;
}

// Interrupts (descriptions from https://www.masswerk.at/6502/6502_instruction_set.html)
// A hardware interrupt (maskable IRQ and non-maskable NMI), will cause the processor to put first the address currently in the program counter onto the stack (in HB-LB order), followed by the value of the status register. (The stack will now contain, seen from the bottom or from the most recently added byte, SR PC-L PC-H with the stack pointer pointing to the address below the stored contents of status register.) Then, the processor will divert its control flow to the address provided in the two word-size interrupt vectors at $FFFA (IRQ) and $FFFE (NMI).
// A set interrupt disable flag will inhibit the execution of an IRQ, but not of a NMI, which will be executed anyways.
// The break instruction (BRK) behaves like a NMI, but will push the value of PC+2 onto the stack to be used as the return address. Also, as with any software initiated transfer of the status register to the stack, the break flag will be found set on the respective value pushed onto the stack. Then, control is transferred to the address in the NMI-vector at $FFFE.
// In any way, the interrupt disable flag is set to inhibit any further IRQ as control is transferred to the interrupt handler specified by the respective interrupt vector.
// The RTI instruction restores the status register from the stack and behaves otherwise like the JSR instruction. (The break flag is always ignored as the status is read from the stack, as it isn't a real processor flag anyway.)
bool CPU::IRQ() {
    if (!getFlag(Flag::INTERRUPT)) {
        setFlag(Flag::INTERRUPT, 1);
        setFlag(Flag::BREAK, 0);

        push16BitDataToStack(pc);
        push8BitDataToStack(sr);

        pc = read16BitData(IRQ_BRK_VECTOR);
        shouldAdvancePC = false;

        // IRQ takes 7 cycles
        remainingCycles = 7;
        return true;
    }

    return false;
}

void CPU::NMI() {
    setFlag(Flag::INTERRUPT, 1);
    setFlag(Flag::BREAK, 0);

    push16BitDataToStack(pc);
    push8BitDataToStack(sr);

    pc = read16BitData(NMI_VECTOR);
    shouldAdvancePC = false;

    // NMI takes 8 cycles
    remainingCycles = 8;
}

uint16_t CPU::getPC() const {
    return pc;
}
uint8_t CPU::getA() const {
    return a;
}
uint8_t CPU::getX() const {
    return x;
}
uint8_t CPU::getY() const {
    return y;
}
uint8_t CPU::getSR() const {
    return sr;
}
uint8_t CPU::getSP() const {
    return sp;
}
bool CPU::getFlag(Flag flag) const {
    return (sr >> static_cast<int>(flag)) & 1;
}
uint8_t CPU::getRemainingCycles() const {
    return remainingCycles;
}
int64_t CPU::getTotalCycles() const {
    return totalCycles;
}

std::array<CPU::Opcode, CPU::MAX_NUM_OPCODES> CPU::initLookup() {
    std::array<Opcode, MAX_NUM_OPCODES> lookup;

    // Define the addressing modes
    const AddressingMode modeACC{ 1, &CPU::ACC, &CPU::strACC };
    const AddressingMode modeABS{ 3, &CPU::ABS, &CPU::strABS };
    const AddressingMode modeABX{ 3, &CPU::ABX, &CPU::strABX };
    const AddressingMode modeABY{ 3, &CPU::ABY, &CPU::strABY };
    const AddressingMode modeIMM{ 2, &CPU::IMM, &CPU::strIMM };
    const AddressingMode modeIMP{ 1, &CPU::IMP, &CPU::strIMP };
    const AddressingMode modeIND{ 3, &CPU::IND, &CPU::strIND };
    const AddressingMode modeIZX{ 2, &CPU::IZX, &CPU::strIZX };
    const AddressingMode modeIZY{ 2, &CPU::IZY, &CPU::strIZY };
    const AddressingMode modeREL{ 2, &CPU::REL, &CPU::strREL };
    const AddressingMode modeZPG{ 2, &CPU::ZPG, &CPU::strZPG };
    const AddressingMode modeZPX{ 2, &CPU::ZPX, &CPU::strZPX };
    const AddressingMode modeZPY{ 2, &CPU::ZPY, &CPU::strZPY };

    // Define the instructions
    const Instruction instADC{ "ADC", &CPU::ADC, 1 };
    const Instruction instAND{ "AND", &CPU::AND, 1 };
    const Instruction instASL{ "ASL", &CPU::ASL, 0 };
    const Instruction instBCC{ "BCC", &CPU::BCC, 0 }; // Extra cycles for branching instructions are handled within the execute functions
    const Instruction instBCS{ "BCS", &CPU::BCS, 0 }; // Extra cycles for branching instructions are handled within the execute functions
    const Instruction instBEQ{ "BEQ", &CPU::BEQ, 0 }; // Extra cycles for branching instructions are handled within the execute functions
    const Instruction instBIT{ "BIT", &CPU::BIT, 0 };
    const Instruction instBMI{ "BMI", &CPU::BMI, 0 }; // Extra cycles for branching instructions are handled within the execute functions
    const Instruction instBNE{ "BNE", &CPU::BNE, 0 }; // Extra cycles for branching instructions are handled within the execute functions
    const Instruction instBPL{ "BPL", &CPU::BPL, 0 }; // Extra cycles for branching instructions are handled within the execute functions
    const Instruction instBRK{ "BRK", &CPU::BRK, 0 };
    const Instruction instBVC{ "BVC", &CPU::BVC, 0 }; // Extra cycles for branching instructions are handled within the execute functions
    const Instruction instBVS{ "BVS", &CPU::BVS, 0 }; // Extra cycles for branching instructions are handled within the execute functions
    const Instruction instCLC{ "CLC", &CPU::CLC, 0 };
    const Instruction instCLD{ "CLD", &CPU::CLD, 0 };
    const Instruction instCLI{ "CLI", &CPU::CLI, 0 };
    const Instruction instCLV{ "CLV", &CPU::CLV, 0 };
    const Instruction instCMP{ "CMP", &CPU::CMP, 1 };
    const Instruction instCPX{ "CPX", &CPU::CPX, 0 };
    const Instruction instCPY{ "CPY", &CPU::CPY, 0 };
    const Instruction instDEC{ "DEC", &CPU::DEC, 0 };
    const Instruction instDEX{ "DEX", &CPU::DEX, 0 };
    const Instruction instDEY{ "DEY", &CPU::DEY, 0 };
    const Instruction instEOR{ "EOR", &CPU::EOR, 1 };
    const Instruction instINC{ "INC", &CPU::INC, 0 };
    const Instruction instINX{ "INX", &CPU::INX, 0 };
    const Instruction instINY{ "INY", &CPU::INY, 0 };
    const Instruction instJMP{ "JMP", &CPU::JMP, 0 };
    const Instruction instJSR{ "JSR", &CPU::JSR, 0 };
    const Instruction instLDA{ "LDA", &CPU::LDA, 1 };
    const Instruction instLDX{ "LDX", &CPU::LDX, 1 };
    const Instruction instLDY{ "LDY", &CPU::LDY, 1 };
    const Instruction instLSR{ "LSR", &CPU::LSR, 0 };
    const Instruction instNOP{ "NOP", &CPU::NOP, 0 };
    const Instruction instORA{ "ORA", &CPU::ORA, 1 };
    const Instruction instPHA{ "PHA", &CPU::PHA, 0 };
    const Instruction instPHP{ "PHP", &CPU::PHP, 0 };
    const Instruction instPLA{ "PLA", &CPU::PLA, 0 };
    const Instruction instPLP{ "PLP", &CPU::PLP, 0 };
    const Instruction instROL{ "ROL", &CPU::ROL, 0 };
    const Instruction instROR{ "ROR", &CPU::ROR, 0 };
    const Instruction instRTI{ "RTI", &CPU::RTI, 0 };
    const Instruction instRTS{ "RTS", &CPU::RTS, 0 };
    const Instruction instSBC{ "SBC", &CPU::SBC, 1 };
    const Instruction instSEC{ "SEC", &CPU::SEC, 0 };
    const Instruction instSED{ "SED", &CPU::SED, 0 };
    const Instruction instSEI{ "SEI", &CPU::SEI, 0 };
    const Instruction instSTA{ "STA", &CPU::STA, 0 };
    const Instruction instSTX{ "STX", &CPU::STX, 0 };
    const Instruction instSTY{ "STY", &CPU::STY, 0 };
    const Instruction instTAX{ "TAX", &CPU::TAX, 0 };
    const Instruction instTAY{ "TAY", &CPU::TAY, 0 };
    const Instruction instTSX{ "TSX", &CPU::TSX, 0 };
    const Instruction instTXA{ "TXA", &CPU::TXA, 0 };
    const Instruction instTXS{ "TXS", &CPU::TXS, 0 };
    const Instruction instTYA{ "TYA", &CPU::TYA, 0 };
    const Instruction instUNI{ "???", &CPU::UNI, 0 };

    // Populate the lookup table
    lookup.fill(Opcode{ instUNI, modeIMP, 2 }); // Initialize all opcodes to unimplemented

    // Fill in the lookup table with the valid opcodes
    lookup[0x00] = Opcode{ instBRK, modeIMP, 7 };
    lookup[0x01] = Opcode{ instORA, modeIZX, 6 };
    lookup[0x05] = Opcode{ instORA, modeZPG, 3 };
    lookup[0x06] = Opcode{ instASL, modeZPG, 5 };
    lookup[0x08] = Opcode{ instPHP, modeIMP, 3 };
    lookup[0x09] = Opcode{ instORA, modeIMM, 2 };
    lookup[0x0A] = Opcode{ instASL, modeACC, 2 };
    lookup[0x0D] = Opcode{ instORA, modeABS, 4 };
    lookup[0x0E] = Opcode{ instASL, modeABS, 6 };

    lookup[0x10] = Opcode{ instBPL, modeREL, 2 };
    lookup[0x11] = Opcode{ instORA, modeIZY, 5 };
    lookup[0x15] = Opcode{ instORA, modeZPX, 4 };
    lookup[0x16] = Opcode{ instASL, modeZPX, 6 };
    lookup[0x18] = Opcode{ instCLC, modeIMP, 2 };
    lookup[0x19] = Opcode{ instORA, modeABY, 4 };
    lookup[0x1D] = Opcode{ instORA, modeABX, 4 };
    lookup[0x1E] = Opcode{ instASL, modeABX, 7 };

    lookup[0x20] = Opcode{ instJSR, modeABS, 6 };
    lookup[0x21] = Opcode{ instAND, modeIZX, 6 };
    lookup[0x24] = Opcode{ instBIT, modeZPG, 3 };
    lookup[0x25] = Opcode{ instAND, modeZPG, 3 };
    lookup[0x26] = Opcode{ instROL, modeZPG, 5 };
    lookup[0x28] = Opcode{ instPLP, modeIMP, 4 };
    lookup[0x29] = Opcode{ instAND, modeIMM, 2 };
    lookup[0x2A] = Opcode{ instROL, modeACC, 2 };
    lookup[0x2C] = Opcode{ instBIT, modeABS, 4 };
    lookup[0x2D] = Opcode{ instAND, modeABS, 4 };
    lookup[0x2E] = Opcode{ instROL, modeABS, 6 };

    lookup[0x30] = Opcode{ instBMI, modeREL, 2 };
    lookup[0x31] = Opcode{ instAND, modeIZY, 5 };
    lookup[0x35] = Opcode{ instAND, modeZPX, 4 };
    lookup[0x36] = Opcode{ instROL, modeZPX, 6 };
    lookup[0x38] = Opcode{ instSEC, modeIMP, 2 };
    lookup[0x39] = Opcode{ instAND, modeABY, 4 };
    lookup[0x3D] = Opcode{ instAND, modeABX, 4 };
    lookup[0x3E] = Opcode{ instROL, modeABX, 7 };

    lookup[0x40] = Opcode{ instRTI, modeIMP, 6 };
    lookup[0x41] = Opcode{ instEOR, modeIZX, 6 };
    lookup[0x45] = Opcode{ instEOR, modeZPG, 3 };
    lookup[0x46] = Opcode{ instLSR, modeZPG, 5 };
    lookup[0x48] = Opcode{ instPHA, modeIMP, 3 };
    lookup[0x49] = Opcode{ instEOR, modeIMM, 2 };
    lookup[0x4A] = Opcode{ instLSR, modeACC, 2 };
    lookup[0x4C] = Opcode{ instJMP, modeABS, 3 };
    lookup[0x4D] = Opcode{ instEOR, modeABS, 4 };
    lookup[0x4E] = Opcode{ instLSR, modeABS, 6 };

    lookup[0x50] = Opcode{ instBVC, modeREL, 2 };
    lookup[0x51] = Opcode{ instEOR, modeIZY, 5 };
    lookup[0x55] = Opcode{ instEOR, modeZPX, 4 };
    lookup[0x56] = Opcode{ instLSR, modeZPX, 6 };
    lookup[0x58] = Opcode{ instCLI, modeIMP, 2 };
    lookup[0x59] = Opcode{ instEOR, modeABY, 4 };
    lookup[0x5D] = Opcode{ instEOR, modeABX, 4 };
    lookup[0x5E] = Opcode{ instLSR, modeABX, 7 };

    lookup[0x60] = Opcode{ instRTS, modeIMP, 6 };
    lookup[0x61] = Opcode{ instADC, modeIZX, 6 };
    lookup[0x65] = Opcode{ instADC, modeZPG, 3 };
    lookup[0x66] = Opcode{ instROR, modeZPG, 5 };
    lookup[0x68] = Opcode{ instPLA, modeIMP, 4 };
    lookup[0x69] = Opcode{ instADC, modeIMM, 2 };
    lookup[0x6A] = Opcode{ instROR, modeACC, 2 };
    lookup[0x6C] = Opcode{ instJMP, modeIND, 5 };
    lookup[0x6D] = Opcode{ instADC, modeABS, 4 };
    lookup[0x6E] = Opcode{ instROR, modeABS, 6 };

    lookup[0x70] = Opcode{ instBVS, modeREL, 2 };
    lookup[0x71] = Opcode{ instADC, modeIZY, 5 };
    lookup[0x75] = Opcode{ instADC, modeZPX, 4 };
    lookup[0x76] = Opcode{ instROR, modeZPX, 6 };
    lookup[0x78] = Opcode{ instSEI, modeIMP, 2 };
    lookup[0x79] = Opcode{ instADC, modeABY, 4 };
    lookup[0x7D] = Opcode{ instADC, modeABX, 4 };
    lookup[0x7E] = Opcode{ instROR, modeABX, 7 };

    lookup[0x81] = Opcode{ instSTA, modeIZX, 6 };
    lookup[0x84] = Opcode{ instSTY, modeZPG, 3 };
    lookup[0x85] = Opcode{ instSTA, modeZPG, 3 };
    lookup[0x86] = Opcode{ instSTX, modeZPG, 3 };
    lookup[0x88] = Opcode{ instDEY, modeIMP, 2 };
    lookup[0x8A] = Opcode{ instTXA, modeIMP, 2 };
    lookup[0x8C] = Opcode{ instSTY, modeABS, 4 };
    lookup[0x8D] = Opcode{ instSTA, modeABS, 4 };
    lookup[0x8E] = Opcode{ instSTX, modeABS, 4 };

    lookup[0x90] = Opcode{ instBCC, modeREL, 2 };
    lookup[0x91] = Opcode{ instSTA, modeIZY, 6 };
    lookup[0x94] = Opcode{ instSTY, modeZPX, 4 };
    lookup[0x95] = Opcode{ instSTA, modeZPX, 4 };
    lookup[0x96] = Opcode{ instSTX, modeZPY, 4 };
    lookup[0x98] = Opcode{ instTYA, modeIMP, 2 };
    lookup[0x99] = Opcode{ instSTA, modeABY, 5 };
    lookup[0x9A] = Opcode{ instTXS, modeIMP, 2 };
    lookup[0x9D] = Opcode{ instSTA, modeABX, 5 };

    lookup[0xA0] = Opcode{ instLDY, modeIMM, 2 };
    lookup[0xA1] = Opcode{ instLDA, modeIZX, 6 };
    lookup[0xA2] = Opcode{ instLDX, modeIMM, 2 };
    lookup[0xA4] = Opcode{ instLDY, modeZPG, 3 };
    lookup[0xA5] = Opcode{ instLDA, modeZPG, 3 };
    lookup[0xA6] = Opcode{ instLDX, modeZPG, 3 };
    lookup[0xA8] = Opcode{ instTAY, modeIMP, 2 };
    lookup[0xA9] = Opcode{ instLDA, modeIMM, 2 };
    lookup[0xAA] = Opcode{ instTAX, modeIMP, 2 };
    lookup[0xAC] = Opcode{ instLDY, modeABS, 4 };
    lookup[0xAD] = Opcode{ instLDA, modeABS, 4 };
    lookup[0xAE] = Opcode{ instLDX, modeABS, 4 };

    lookup[0xB0] = Opcode{ instBCS, modeREL, 2 };
    lookup[0xB1] = Opcode{ instLDA, modeIZY, 5 };
    lookup[0xB4] = Opcode{ instLDY, modeZPX, 4 };
    lookup[0xB5] = Opcode{ instLDA, modeZPX, 4 };
    lookup[0xB6] = Opcode{ instLDX, modeZPY, 4 };
    lookup[0xB8] = Opcode{ instCLV, modeIMP, 2 };
    lookup[0xB9] = Opcode{ instLDA, modeABY, 4 };
    lookup[0xBA] = Opcode{ instTSX, modeIMP, 2 };
    lookup[0xBC] = Opcode{ instLDY, modeABX, 4 };
    lookup[0xBD] = Opcode{ instLDA, modeABX, 4 };
    lookup[0xBE] = Opcode{ instLDX, modeABY, 4 };

    lookup[0xC0] = Opcode{ instCPY, modeIMM, 2 };
    lookup[0xC1] = Opcode{ instCMP, modeIZX, 6 };
    lookup[0xC4] = Opcode{ instCPY, modeZPG, 3 };
    lookup[0xC5] = Opcode{ instCMP, modeZPG, 3 };
    lookup[0xC6] = Opcode{ instDEC, modeZPG, 5 };
    lookup[0xC8] = Opcode{ instINY, modeIMP, 2 };
    lookup[0xC9] = Opcode{ instCMP, modeIMM, 2 };
    lookup[0xCA] = Opcode{ instDEX, modeIMP, 2 };
    lookup[0xCC] = Opcode{ instCPY, modeABS, 4 };
    lookup[0xCD] = Opcode{ instCMP, modeABS, 4 };
    lookup[0xCE] = Opcode{ instDEC, modeABS, 6 };

    lookup[0xD0] = Opcode{ instBNE, modeREL, 2 };
    lookup[0xD1] = Opcode{ instCMP, modeIZY, 5 };
    lookup[0xD5] = Opcode{ instCMP, modeZPX, 4 };
    lookup[0xD6] = Opcode{ instDEC, modeZPX, 6 };
    lookup[0xD8] = Opcode{ instCLD, modeIMP, 2 };
    lookup[0xD9] = Opcode{ instCMP, modeABY, 4 };
    lookup[0xDD] = Opcode{ instCMP, modeABX, 4 };
    lookup[0xDE] = Opcode{ instDEC, modeABX, 7 };

    lookup[0xE0] = Opcode{ instCPX, modeIMM, 2 };
    lookup[0xE1] = Opcode{ instSBC, modeIZX, 6 };
    lookup[0xE4] = Opcode{ instCPX, modeZPG, 3 };
    lookup[0xE5] = Opcode{ instSBC, modeZPG, 3 };
    lookup[0xE6] = Opcode{ instINC, modeZPG, 5 };
    lookup[0xE8] = Opcode{ instINX, modeIMP, 2 };
    lookup[0xE9] = Opcode{ instSBC, modeIMM, 2 };
    lookup[0xEA] = Opcode{ instNOP, modeIMP, 2 };
    lookup[0xEC] = Opcode{ instCPX, modeABS, 4 };
    lookup[0xED] = Opcode{ instSBC, modeABS, 4 };
    lookup[0xEE] = Opcode{ instINC, modeABS, 6 };

    lookup[0xF0] = Opcode{ instBEQ, modeREL, 2 };
    lookup[0xF1] = Opcode{ instSBC, modeIZY, 5 };
    lookup[0xF5] = Opcode{ instSBC, modeZPX, 4 };
    lookup[0xF6] = Opcode{ instINC, modeZPX, 6 };
    lookup[0xF8] = Opcode{ instSED, modeIMP, 2 };
    lookup[0xF9] = Opcode{ instSBC, modeABY, 4 };
    lookup[0xFD] = Opcode{ instSBC, modeABX, 4 };
    lookup[0xFE] = Opcode{ instINC, modeABX, 7 };

    return lookup;
}

bool CPU::AddressingMode::ReturnType::hasAddress() const {
    const uint16_t* address = std::get_if<uint16_t>(&addressingModeOutput);
    return address != nullptr;
}

bool CPU::AddressingMode::ReturnType::hasData() const {
    const uint8_t* data = std::get_if<uint8_t>(&addressingModeOutput);
    return data != nullptr;
}

uint16_t CPU::getAddress(const AddressingMode::ReturnType& operand) const {
    // For instrutions that require an address, the addressing modes provide us with it directly.
    // So this std::get call should never throw an exception.
    return std::get<uint16_t>(operand.addressingModeOutput);
}

uint8_t CPU::getDataRead(const AddressingMode::ReturnType& operand) {
    // Some addressing modes (i.e. IMM) return us with data directly, while others (i.e. ZPX) give us the address that the data resides in.
    // So we first check if the data is present within operand, and if not, we read it from the bus.
    const uint8_t* data = std::get_if<uint8_t>(&operand.addressingModeOutput);
    if (data != nullptr) {
        return *data;
    }
    else {
        uint16_t address = std::get<uint16_t>(operand.addressingModeOutput);
        return bus->read(address);
    }
}

uint8_t CPU::getDataView(const AddressingMode::ReturnType& operand) const {
    // Same code as getDataRead(), but we call bus->view() instead of bus->read().
    // This function is used when we want to look under the hood of the console without changing its state (i.e. debugging information)
    const uint8_t* data = std::get_if<uint8_t>(&operand.addressingModeOutput);
    if (data != nullptr) {
        return *data;
    }
    else {
        uint16_t address = std::get<uint16_t>(operand.addressingModeOutput);
        return bus->view(address);
    }
}

void CPU::setFlag(Flag flag, bool value) {
    if (value) {
        sr |= (1 << static_cast<int>(flag));
    }
    else {
        sr &= ~(1 << static_cast<int>(flag));
    }
}

// The N and Z flags are often set alongside each other during instructions
void CPU::setNZFlags(uint8_t x) {
    setFlag(Flag::NEGATIVE, (x >> 7) & 1);
    setFlag(Flag::ZERO, x == 0);
}

uint16_t CPU::view16BitData(uint16_t address) const {
    uint8_t lo = bus->view(address);
    uint8_t hi = bus->view(address + 1);
    uint16_t data = (hi << 8) | lo;
    return data;
}

uint16_t CPU::read16BitData(uint16_t address) {
    uint8_t lo = bus->read(address);
    uint8_t hi = bus->read(address + 1);
    uint16_t data = (hi << 8) | lo;
    return data;
}

void CPU::write16BitData(uint16_t address, uint16_t data) {
    uint8_t lo = data & 0xFF;
    uint8_t hi = (data >> 8) & 0xFF;
    bus->write(address, lo);
    bus->write(address + 1, hi);
}

void CPU::push8BitDataToStack(uint8_t data) {
    bus->write(STACK_OFFSET + sp, data);
    sp--;
}

uint8_t CPU::pop8BitDataFromStack() {
    // Since the stack grows backwards, we need to read from sp + 1
    uint8_t data = bus->read(STACK_OFFSET + ((sp + 1) & 0xFF));
    sp++;
    return data;
}

void CPU::push16BitDataToStack(uint16_t data) {
    // Since the stack grows backwards, we need to write the least signifigant bit to sp - 1
    uint8_t lo = data & 0xFF;
    uint8_t hi = (data >> 8) & 0xFF;
    bus->write(STACK_OFFSET + ((sp - 1) & 0xFF), lo);
    bus->write(STACK_OFFSET + sp, hi);
    sp -= 2;
}

uint16_t CPU::pop16BitDataFromStack() {
    // Since the stack grows backwards, we need to read from sp + 1
    // uint16_t data = read16BitData(STACK_OFFSET + sp + 1);
    uint8_t lo = bus->read(STACK_OFFSET + ((sp + 1) & 0xFF));
    uint8_t hi = bus->read(STACK_OFFSET + ((sp + 2) & 0xFF));
    sp += 2;

    uint16_t data = (hi << 8) | lo;
    return data;
}

// Helper function for addressing modes
bool isPageChange(uint16_t address1, uint16_t address2) {
    return (address1 & 0xFF00) != (address2 & 0xFF00);
}

// 6502 addressing mode function definitions (descriptions from http://www.emulator101.com/6502-addressing-modes.html)

// Accumulator
// These instructions have register A (the accumulator) as the target. Examples are LSR A and ROL A.
CPU::AddressingMode::ReturnType CPU::ACC() {
    return { a, 0 };
}

// Absolute
// Absolute addressing specifies the memory location explicitly in the two bytes following the opcode. So JMP $4032 will set the PC to $4032. The hex for this is 4C 32 40. The 6502 is a little endian machine, so any 16 bit (2 byte) value is stored with the LSB first. All instructions that use absolute addressing are 3 bytes.
CPU::AddressingMode::ReturnType CPU::ABS() {
    uint16_t address = read16BitData(pc + 1);
    return { address, 0 };
}

// Absolute Indexed X
// This addressing mode makes the target address by adding the contents of the X or Y register to an absolute address. For example, this 6502 code can be used to fill 10 bytes with $FF starting at address $1009, counting down to address $1000.
CPU::AddressingMode::ReturnType CPU::ABX() {
    uint16_t oldAddress = read16BitData(pc + 1);
    uint16_t newAddress = oldAddress + x;
    return { newAddress, isPageChange(oldAddress, newAddress) };
}

// Absolute Indexed Y
// This addressing mode makes the target address by adding the contents of the X or Y register to an absolute address. For example, this 6502 code can be used to fill 10 bytes with $FF starting at address $1009, counting down to address $1000.
CPU::AddressingMode::ReturnType CPU::ABY() {
    uint16_t oldAddress = read16BitData(pc + 1);
    uint16_t newAddress = oldAddress + y;
    return { newAddress, isPageChange(oldAddress, newAddress) };
}

// Immediate
// These instructions have their data defined as the next byte after the opcode. ORA #$B2 will perform a logical (also called bitwise) of the value B2 with the accumulator. Remember that in assembly when you see a # sign, it indicates an immediate value. If $B2 was written without a #, it would indicate an address or offset.
CPU::AddressingMode::ReturnType CPU::IMM() {
    return { bus->read(pc + 1), 0 };
}

// Implied
// In an implied instruction, the data and/or destination is mandatory for the instruction. For example, the CLC instruction is implied, it is going to clear the processor's Carry flag.
CPU::AddressingMode::ReturnType CPU::IMP() {
    return { std::monostate{}, 0 };
}

// Indirect
// The JMP instruction is the only instruction that uses this addressing mode. It is a 3 byte instruction - the 2nd and 3rd bytes are an absolute address. The set the PC to the address stored at that address. So maybe this would be clearer.
CPU::AddressingMode::ReturnType CPU::IND() {
    uint16_t pointer = read16BitData(pc + 1);

    uint16_t address;

    // Due to a bug, indirect address reads cannot cross page boundaries and instead wrap around.
    // Because of this, we can't use read16BitData(pointer) when the bottom two bytes are FF.
    if ((pointer & 0xFF) == 0xFF) {
        uint8_t lo = bus->read(pointer);
        uint8_t hi = bus->read(pointer & 0xFF00);
        address = (hi << 8) | lo;
    }
    else {
        address = read16BitData(pointer);
    }

    return { address, 0 };
}

// Indexed Indirect X
// This mode is only used with the X register. Consider a situation where the instruction is LDA ($20,X), X contains $04, and memory at $24 contains 0024: 74 20, First, X is added to $20 to get $24. The target address will be fetched from $24 resulting in a target address of $2074. Register A will be loaded with the contents of memory at $2074.
// If X + the immediate byte will wrap around to a zero-page address. So you could code that like targetAddress = (X + opcode[1]) & 0xFF .
// Indexed Indirect instructions are 2 bytes - the second byte is the zero-page address - $20 in the example. Obviously the fetched address has to be stored in the zero page.
CPU::AddressingMode::ReturnType CPU::IZX() {
    uint8_t pointer = bus->read(pc + 1) + x;

    // We can't use read16BitData(pointer) because of zero page wrapping
    uint8_t lo = bus->read(pointer);
    uint8_t hi = bus->read((pointer + 1) & 0xFF);
    uint16_t address = (hi << 8) | lo;

    return { address, 0 };
}

// Indirect Indexed Y
// This mode is only used with the Y register. It differs in the order that Y is applied to the indirectly fetched address. An example instruction that uses indirect index addressing is LDA ($86),Y . To calculate the target address, the CPU will first fetch the address stored at zero page location $86. That address will be added to register Y to get the final target address. For LDA ($86),Y, if the address stored at $86 is $4028 (memory is 0086: 28 40, remember little endian) and register Y contains $10, then the final target address would be $4038. Register A will be loaded with the contents of memory at $4038.
// Indirect Indexed instructions are 2 bytes - the second byte is the zero-page address - $86 in the example. (So the fetched address has to be stored in the zero page.)
// While indexed indirect addressing will only generate a zero-page address, this mode's target address is not wrapped - it can be anywhere in the 16-bit address space.
CPU::AddressingMode::ReturnType CPU::IZY() {
    uint8_t pointer = bus->read(pc + 1);

    // We can't use read16BitData(pointer) because of zero page wrapping
    uint8_t lo = bus->read(pointer);
    uint8_t hi = bus->read((pointer + 1) & 0xFF);
    uint16_t oldAddress = (hi << 8) | lo;

    uint16_t newAddress = oldAddress + y;
    return { newAddress, isPageChange(oldAddress, newAddress) };
}

// Relative
// Relative addressing on the 6502 is only used for branch operations. The byte after the opcode is the branch offset. If the branch is taken, the new address will the the current PC plus the offset. The offset is a signed byte, so it can jump a maximum of 127 bytes forward, or 128 bytes backward. (For more info about signed numbers, check here.)
CPU::AddressingMode::ReturnType CPU::REL() {
    uint8_t offset = bus->read(pc + 1);

    // Relative addressing is done from the end of the instruction, so we need to add 2 to this address.
    uint16_t newAddress = pc + 2 + static_cast<int8_t>(offset);
    return { newAddress, isPageChange(pc + 2, newAddress) };
}

// Zero-Page
// Zero-Page is an addressing mode that is only capable of addressing the first 256 bytes of the CPU's memory map. You can think of it as absolute addressing for the first 256 bytes. The instruction LDA $35 will put the value stored in memory location $35 into A. The advantage of zero-page are two - the instruction takes one less byte to specify, and it executes in less CPU cycles. Most programs are written to store the most frequently used variables in the first 256 memory locations so they can take advantage of zero page addressing.
CPU::AddressingMode::ReturnType CPU::ZPG() {
    uint16_t address = bus->read(pc + 1);
    return { address, 0 };
}

// Zero-Page Indexed X
// This works just like absolute indexed, but the target address is limited to the first 0xFF bytes.
// The target address will wrap around and will always be in the zero page. If the instruction is LDA $C0,X, and X is $60, then the target address will be $20. $C0+$60 = $120, but the carry is discarded in the calculation of the target address.
CPU::AddressingMode::ReturnType CPU::ZPX() {
    uint16_t address = (bus->read(pc + 1) + x) & 0xFF;
    return { address, 0 };
}

// Zero-Page Indexed Y
// This works just like absolute indexed, but the target address is limited to the first 0xFF bytes.
// The target address will wrap around and will always be in the zero page. If the instruction is LDA $C0,X, and X is $60, then the target address will be $20. $C0+$60 = $120, but the carry is discarded in the calculation of the target address.
CPU::AddressingMode::ReturnType CPU::ZPY() {
    uint16_t address = (bus->read(pc + 1) + y) & 0xFF;
    return { address, 0 };
}


// Definitions for instruction functions
// Descriptions from https://www.masswerk.at/6502/6502_instruction_set.html

// ADC
// Add Memory to Accumulator with Carry
// A + M + C -> A, C
//  N	Z	C	I	D	V
//  +	+	+	-	-	+
void CPU::ADC(const AddressingMode::ReturnType& operand) {
    uint8_t data = getDataRead(operand);
    uint16_t fullSum = a + data + getFlag(Flag::CARRY);

    setFlag(Flag::CARRY, fullSum > 0xFF);

    // Set the overflow flag only when the two addends have the same sign, and result has a different sign
    setFlag(Flag::OVERFLOW_, ~(a ^ data) & (a ^ static_cast<uint8_t>(fullSum)) & 0x80);

    setNZFlags(static_cast<uint8_t>(fullSum));

    a = static_cast<uint8_t>(fullSum);
}

// AND
// AND Memory with Accumulator
// A AND M -> A
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::AND(const AddressingMode::ReturnType& operand) {
    uint8_t data = getDataRead(operand);
    a &= data;

    setNZFlags(a);
}

// ASL
// Shift Left One Bit (Memory or Accumulator)
// C <- [76543210] <- 0
//  N	Z	C	I	D	V
//  +	+	+	-	-	-
void CPU::ASL(const AddressingMode::ReturnType& operand) {
    uint16_t shift;
    if (operand.hasAddress()) {
        uint16_t addr = getAddress(operand);
        uint8_t data = getDataRead(operand);

        shift = data << 1;
        bus->write(addr, static_cast<uint8_t>(shift));
    }
    else {
        // If there is no address to write to, then we are in accumulator addressing mode
        shift = a << 1;
        a = static_cast<uint8_t>(shift);
    };

    setFlag(Flag::CARRY, shift > 0xFF);
    setNZFlags(static_cast<uint8_t>(shift));
}

// BCC
// Branch on Carry Clear
// Branch on C = 0
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::BCC(const AddressingMode::ReturnType& operand) {
    if (!getFlag(Flag::CARRY)) {
        pc = getAddress(operand);
        shouldAdvancePC = false;
        remainingCycles++;

        if (operand.mightNeedExtraCycle) {
            remainingCycles++;
        }
    }
}

// BCS
// Branch on Carry Set
// Branch on C = 1
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::BCS(const AddressingMode::ReturnType& operand) {
    if (getFlag(Flag::CARRY)) {
        pc = getAddress(operand);
        shouldAdvancePC = false;
        remainingCycles++;

        if (operand.mightNeedExtraCycle) {
            remainingCycles++;
        }
    }
}

// BEQ
// Branch on Result Zero
// Branch on Z = 1
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::BEQ(const AddressingMode::ReturnType& operand) {
    if (getFlag(Flag::ZERO)) {
        pc = getAddress(operand);
        shouldAdvancePC = false;
        remainingCycles++;

        if (operand.mightNeedExtraCycle) {
            remainingCycles++;
        }
    }
}

// BIT
// Test Bits in Memory with Accumulator
// Bits 7 and 6 of operand are transfered to bit 7 and 6 of SR (N,V);
// The zero-flag is set according to the result of the operand AND
// The accumulator (set, if the result is zero, unset otherwise).
// This allows a quick check of a few bits at once without affecting
// Any of the registers, other than the status register (SR).
// A AND M -> Z, M7 -> N, M6 -> V
//  N	Z	C	I	D	V
//  M7	+	-	-	-	M6
void CPU::BIT(const AddressingMode::ReturnType& operand) {
    uint8_t data = getDataRead(operand);

    setFlag(Flag::ZERO, (a & data) == 0);
    setFlag(Flag::NEGATIVE, (data >> 7) & 1);
    setFlag(Flag::OVERFLOW_, (data >> 6) & 1);
}

// BMI
// Branch on Result Minus
// Branch on N = 1
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::BMI(const AddressingMode::ReturnType& operand) {
    if (getFlag(Flag::NEGATIVE)) {
        pc = getAddress(operand);
        shouldAdvancePC = false;
        remainingCycles++;

        if (operand.mightNeedExtraCycle) {
            remainingCycles++;
        }
    }
}

// BNE
// Branch on Result not Zero
// Branch on Z = 0
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::BNE(const AddressingMode::ReturnType& operand) {
    if (!getFlag(Flag::ZERO)) {
        pc = getAddress(operand);
        shouldAdvancePC = false;
        remainingCycles++;

        if (operand.mightNeedExtraCycle) {
            remainingCycles++;
        }
    }
}

// BPL
// Branch on Result Plus
// Branch on N = 0
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::BPL(const AddressingMode::ReturnType& operand) {
    if (!getFlag(Flag::NEGATIVE)) {
        pc = getAddress(operand);
        shouldAdvancePC = false;
        remainingCycles++;

        if (operand.mightNeedExtraCycle) {
            remainingCycles++;
        }
    }
}

// BRK
// Force Break
// BRK initiates a software interrupt similar to a hardware
// Interrupt (IRQ). The return address pushed to the stack is
// PC+2, providing an extra byte of spacing for a break mark
// (identifying a reason for the break.)
// The status register will be pushed to the stack with the break
// Flag set to 1. However, when retrieved during RTI or by a PLP
// Instruction, the break flag will be ignored.
// The interrupt disable flag is not set automatically.
// Interrupt,
// Push PC+2, push SR
//  N	Z	C	I	D	V
//  -	-	-	1	-	-
void CPU::BRK(const AddressingMode::ReturnType& /*operand*/) {
    setFlag(Flag::INTERRUPT, 1);

    push16BitDataToStack(pc + 2);

    setFlag(Flag::BREAK, 1);
    push8BitDataToStack(sr);
    setFlag(Flag::BREAK, 0);

    pc = read16BitData(IRQ_BRK_VECTOR);
    shouldAdvancePC = false;
}

// BVC
// Branch on Overflow Clear
// Branch on V = 0
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::BVC(const AddressingMode::ReturnType& operand) {
    if (!getFlag(Flag::OVERFLOW_)) {
        pc = getAddress(operand);
        shouldAdvancePC = false;
        remainingCycles++;

        if (operand.mightNeedExtraCycle) {
            remainingCycles++;
        }
    }
}

// BVS
// Branch on Overflow Set
// Branch on V = 1
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::BVS(const AddressingMode::ReturnType& operand) {
    if (getFlag(Flag::OVERFLOW_)) {
        pc = getAddress(operand);
        shouldAdvancePC = false;
        remainingCycles++;

        if (operand.mightNeedExtraCycle) {
            remainingCycles++;
        }
    }
}

// CLC
// Clear Carry Flag
// 0 -> C
//  N	Z	C	I	D	V
//  -	-	0	-	-	-
void CPU::CLC(const AddressingMode::ReturnType& /*operand*/) {
    setFlag(Flag::CARRY, 0);
}

// CLD
// Clear Decimal Mode
// 0 -> D
//  N	Z	C	I	D	V
//  -	-	-	-	0	-
void CPU::CLD(const AddressingMode::ReturnType& /*operand*/) {
    setFlag(Flag::DECIMAL, 0);
}

// CLI
// Clear Interrupt Disable Bit
// 0 -> I
//  N	Z	C	I	D	V
//  -	-	-	0	-	-
void CPU::CLI(const AddressingMode::ReturnType& /*operand*/) {
    setFlag(Flag::INTERRUPT, 0);
}

// CLV
// Clear Overflow Flag
// 0 -> V
//  N	Z	C	I	D	V
//  -	-	-	-	-	0
void CPU::CLV(const AddressingMode::ReturnType& /*operand*/) {
    setFlag(Flag::OVERFLOW_, 0);
}

// Compare Memory with Accumulator
// A - M
//  N	Z	C	I	D	V
//  +	+	+	-	-	-
void CPU::CMP(const AddressingMode::ReturnType& operand) {
    uint8_t data = getDataRead(operand);
    uint16_t cmp = a - data;
    setFlag(Flag::CARRY, a >= data);
    setNZFlags(cmp);
}

// CPX
// Compare Memory and Index X
// X - M
//  N	Z	C	I	D	V
//  +	+	+	-	-	-
void CPU::CPX(const AddressingMode::ReturnType& operand) {
    uint8_t data = getDataRead(operand);
    uint16_t cmp = x - data;
    setFlag(Flag::CARRY, x >= data);
    setNZFlags(cmp);
}

// CPY
// Compare Memory and Index Y
// Y - M
//  N	Z	C	I	D	V
//  +	+	+	-	-	-
void CPU::CPY(const AddressingMode::ReturnType& operand) {
    uint8_t data = getDataRead(operand);
    uint16_t cmp = y - data;
    setFlag(Flag::CARRY, y >= data);
    setNZFlags(cmp);
}

// DEC
// Decrement Memory by One
// M - 1 -> M
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::DEC(const AddressingMode::ReturnType& operand) {
    uint16_t addr = getAddress(operand);
    uint8_t newData = bus->read(addr) - 1;
    bus->write(addr, newData);
    setNZFlags(newData);
}

// DEX
// Decrement Index X by One
// X - 1 -> X
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::DEX(const AddressingMode::ReturnType& /*operand*/) {
    x--;
    setNZFlags(x);
}

// DEY
// Decrement Index Y by One
// Y - 1 -> Y
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::DEY(const AddressingMode::ReturnType& /*operand*/) {
    y--;
    setNZFlags(y);
}

// EOR
// Exclusive-OR Memory with Accumulator
// A EOR M -> A
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::EOR(const AddressingMode::ReturnType& operand) {
    uint8_t data = getDataRead(operand);
    a ^= data;
    setNZFlags(a);
}

// INC
// Increment Memory by One
// M + 1 -> M
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::INC(const AddressingMode::ReturnType& operand) {
    uint16_t addr = getAddress(operand);
    uint8_t newData = bus->read(addr) + 1;
    bus->write(addr, newData);
    setNZFlags(newData);
}

// INX
// Increment Index X by One
// X + 1 -> X
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::INX(const AddressingMode::ReturnType& /*operand*/) {
    x++;
    setNZFlags(x);
}

// Increment Index Y by One
// Y + 1 -> Y
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::INY(const AddressingMode::ReturnType& /*operand*/) {
    y++;
    setNZFlags(y);
}

// JMP
// Jump to New Location
// Operand 1st byte -> PCL
// Operand 2nd byte -> PCH
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::JMP(const AddressingMode::ReturnType& operand) {
    uint16_t addr = getAddress(operand);
    pc = addr;
    shouldAdvancePC = false;
}

// JSR
// Jump to New Location Saving Return Address
// Push (PC+2),
// Operand 1st byte -> PCL
// Operand 2nd byte -> PCH
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::JSR(const AddressingMode::ReturnType& operand) {
    push16BitDataToStack(pc + 2);
    uint16_t addr = getAddress(operand);
    pc = addr;
    shouldAdvancePC = false;
}

// LDA
// Load Accumulator with Memory
// M -> A
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::LDA(const AddressingMode::ReturnType& operand) {
    uint8_t data = getDataRead(operand);
    a = data;
    setNZFlags(a);
}

// LDX
// Load Index X with Memory
// M -> X
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::LDX(const AddressingMode::ReturnType& operand) {
    uint8_t data = getDataRead(operand);
    x = data;
    setNZFlags(x);
}

// LDY
// Load Index Y with Memory
// M -> Y
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::LDY(const AddressingMode::ReturnType& operand) {
    uint8_t data = getDataRead(operand);
    y = data;
    setNZFlags(y);
}

// LSR
// Shift One Bit Right (Memory or Accumulator)
// 0 -> [76543210] -> C
//  N	Z	C	I	D	V
//  0	+	+	-	-	-
void CPU::LSR(const AddressingMode::ReturnType& operand) {
    if (operand.hasAddress()) {
        uint16_t addr = getAddress(operand);
        uint8_t data = getDataRead(operand);

        setFlag(Flag::CARRY, data & 1);

        data >>= 1;
        bus->write(addr, data);

        setNZFlags(data);
    }
    else {
        // If there is no address to write to, then we are in accumulator addressing mode
        setFlag(Flag::CARRY, a & 1);
        a >>= 1;
        setNZFlags(a);
    };



}

// NOP
// No Operation
// ---
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::NOP(const AddressingMode::ReturnType& /*operand*/) {
}

// ORA
// OR Memory with Accumulator
// A OR M -> A
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::ORA(const AddressingMode::ReturnType& operand) {
    uint8_t data = getDataRead(operand);
    a |= data;
    setNZFlags(a);
}

// PHA
// Push Accumulator on Stack
// Push A
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::PHA(const AddressingMode::ReturnType& /*operand*/) {
    push8BitDataToStack(a);
}

// PHP
// Push Processor Status on Stack
// The status register will be pushed with the break
// Flag and bit 5 set to 1.
// Push SR
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::PHP(const AddressingMode::ReturnType& /*operand*/) {
    setFlag(Flag::BREAK, 1);
    push8BitDataToStack(sr);
    setFlag(Flag::BREAK, 0);
}

// PLA
// Pull Accumulator from Stack
// Pull A
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::PLA(const AddressingMode::ReturnType& /*operand*/) {
    a = pop8BitDataFromStack();
    setNZFlags(a);
}

// PLP
// Pull Processor Status from Stack
// The status register will be pulled with the break
// Flag and bit 5 ignored.
// Pull SR
//  N	Z	C	I	D	V
//  from stack
void CPU::PLP(const AddressingMode::ReturnType& /*operand*/) {
    sr = pop8BitDataFromStack();
    setFlag(Flag::BREAK, 0);
    setFlag(Flag::UNUSED, 1);
}

// ROL
// Rotate One Bit Left (Memory or Accumulator)
// C <- [76543210] <- C
//  N	Z	C	I	D	V
//  +	+	+	-	-	-
void CPU::ROL(const AddressingMode::ReturnType& operand) {
    uint16_t shift;
    if (operand.hasAddress()) {
        uint16_t addr = getAddress(operand);
        uint8_t data = getDataRead(operand);

        shift = (data << 1) | static_cast<uint8_t>(getFlag(Flag::CARRY));
        bus->write(addr, static_cast<uint8_t>(shift));
    }
    else {
        // If there is no address to write to, then we are in accumulator addressing mode
        shift = (a << 1) | static_cast<uint8_t>(getFlag(Flag::CARRY));
        a = static_cast<uint8_t>(shift);
    }

    setFlag(Flag::CARRY, shift > 0xFF);
    setNZFlags(static_cast<uint8_t>(shift));
}


// ROR
// Rotate One Bit Right (Memory or Accumulator)
// C -> [76543210] -> C
//  N	Z	C	I	D	V
//  +	+	+	-	-	-
void CPU::ROR(const AddressingMode::ReturnType& operand) {
    uint8_t shift;
    if (operand.hasAddress()) {
        uint16_t addr = getAddress(operand);
        uint8_t data = getDataRead(operand);

        shift = (getFlag(Flag::CARRY) << 7) | (data >> 1);
        setFlag(Flag::CARRY, data & 1);
        bus->write(addr, shift);
    }
    else {
        // If there is no address to write to, then we are in accumulator addressing mode
        shift = (getFlag(Flag::CARRY) << 7) | (a >> 1);
        setFlag(Flag::CARRY, a & 1);

        a = shift;
    }

    setNZFlags(shift);
}

// RTI
// Return from Interrupt
// The status register is pulled with the break flag
// And bit 5 ignored. Then PC is pulled from the stack.
// Pull SR, pull PC
//  N	Z	C	I	D	V
//  from stack
void CPU::RTI(const AddressingMode::ReturnType& /*operand*/) {
    sr = pop8BitDataFromStack();
    setFlag(Flag::BREAK, 0);
    setFlag(Flag::UNUSED, 1);
    pc = pop16BitDataFromStack();
    shouldAdvancePC = false;
}

// RTS
// Return from Subroutine
// Pull PC, PC+1 -> PC
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::RTS(const AddressingMode::ReturnType& /*operand*/) {
    pc = pop16BitDataFromStack() + 1;
    shouldAdvancePC = false;
}

// SBC
// Subtract Memory from Accumulator with Borrow
// A - M - C̅ -> A
//  N	Z	C	I	D	V
//  +	+	+	-	-	+
void CPU::SBC(const AddressingMode::ReturnType& operand) {
    // SBC becomes equivalent to ADC after we flip the bits of data
    uint8_t data = getDataRead(operand) ^ 0xFF;
    uint16_t fullSum = a + data + getFlag(Flag::CARRY);

    setFlag(Flag::CARRY, fullSum > 0xFF);

    // Set the overflow flag only when the two addends have the same sign, and result has a different sign
    setFlag(Flag::OVERFLOW_, ~(a ^ data) & (a ^ static_cast<uint8_t>(fullSum)) & 0x80);

    setNZFlags(static_cast<uint8_t>(fullSum));

    a = static_cast<uint8_t>(fullSum);
}

// SEC
// Set Carry Flag
// 1 -> C
// N	Z	C	I	D	V
// -	-	1	-	-	-
void CPU::SEC(const AddressingMode::ReturnType& /*operand*/) {
    setFlag(Flag::CARRY, 1);
}

// SED
// Set Decimal Flag
// 1 -> D
// N	Z	C	I	D	V
// -	-	-	-	1	-
void CPU::SED(const AddressingMode::ReturnType& /*operand*/) {
    setFlag(Flag::DECIMAL, 1);
}

// SEI
// Set Interrupt Disable Status
// 1 -> I
// N	Z	C	I	D	V
// -	-	-	1	-	-
void CPU::SEI(const AddressingMode::ReturnType& /*operand*/) {
    setFlag(Flag::INTERRUPT, 1);
}

// STA
// Store Accumulator in Memory
// A -> M
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::STA(const AddressingMode::ReturnType& operand) {
    uint16_t addr = getAddress(operand);
    bus->write(addr, a);
}

// STX
// Store Index X in Memory
// X -> M
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::STX(const AddressingMode::ReturnType& operand) {
    uint16_t addr = getAddress(operand);
    bus->write(addr, x);
}

// STY
// Store Index Y in Memory
// Y -> M
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::STY(const AddressingMode::ReturnType& operand) {
    uint16_t addr = getAddress(operand);
    bus->write(addr, y);
}

// TAX
// Transfer Accumulator to Index X
// A -> X
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::TAX(const AddressingMode::ReturnType& /*operand*/) {
    x = a;
    setNZFlags(x);
}

// TAY
// Transfer Accumulator to Index Y
// A -> Y
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::TAY(const AddressingMode::ReturnType& /*operand*/) {
    y = a;
    setNZFlags(y);
}

// TSX
// Transfer Stack Pointer to Index X
// SP -> X
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::TSX(const AddressingMode::ReturnType& /*operand*/) {
    x = sp;
    setNZFlags(x);
}

// TXA
// Transfer Index X to Accumulator
// X -> A
//  N	Z	C	I	D	V
//  +	+	-	-	-	-
void CPU::TXA(const AddressingMode::ReturnType& /*operand*/) {
    a = x;
    setNZFlags(a);
}

// TXS
// Transfer Index X to Stack Register
// X -> SP
//  N	Z	C	I	D	V
//  -	-	-	-	-	-
void CPU::TXS(const AddressingMode::ReturnType& /*operand*/) {
    sp = x;
}

// TYA
// Transfer Index Y to Accumulator
// Y -> A
// N	Z	C	I	D	V
// +	+	-	-	-	-
void CPU::TYA(const AddressingMode::ReturnType& /*operand*/) {
    a = y;
    setNZFlags(a);
}

// UNI
// Unimplemented Instruction
void CPU::UNI(const AddressingMode::ReturnType& /*operand*/) {
    // TODO: handle illegal opcodes
}

// Addressing mode string function definitions
std::string CPU::strACC(uint16_t /*address*/) const {
    return "A";
}
std::string CPU::strABS(uint16_t address) const {
    return "$" + toHexString16(view16BitData(address + 1));
}
std::string CPU::strABX(uint16_t address) const {
    return "$" + toHexString16(view16BitData(address + 1)) + ",X";
}
std::string CPU::strABY(uint16_t address) const {
    return "$" + toHexString16(view16BitData(address + 1)) + ",Y";
}
std::string CPU::strIMM(uint16_t address) const {
    return "#$" + toHexString8(bus->view(address + 1));
}
std::string CPU::strIMP(uint16_t /*address*/) const {
    return "";
}
std::string CPU::strIND(uint16_t address) const {
    return "($" + toHexString16(view16BitData(address + 1)) + ")";
}
std::string CPU::strIZX(uint16_t address) const {
    return "($" + toHexString8(bus->view(address + 1)) + ",X)";
}
std::string CPU::strIZY(uint16_t address) const {
    return "($" + toHexString8(bus->view(address + 1)) + "),Y";
}
std::string CPU::strREL(uint16_t address) const {
    uint8_t relAddress = bus->view(address + 1);
    uint16_t newAddress = address + 2 + static_cast<int8_t>(relAddress);
    return "$" + toHexString16(newAddress);
}
std::string CPU::strZPG(uint16_t address) const {
    return "$" + toHexString8(bus->view(address + 1));
}
std::string CPU::strZPX(uint16_t address) const {
    return "$" + toHexString8(bus->view(address + 1)) + ",X";
}
std::string CPU::strZPY(uint16_t address) const {
    return "$" + toHexString8(bus->view(address + 1)) + ",Y";
}

std::string CPU::toString(uint16_t address) const {
    uint8_t index = bus->view(address);
    const Opcode& opcode = lookup[index];
    return opcode.instruction.name + " " + (this->*opcode.addressingMode.toString)(address);
}

const CPU::Opcode& CPU::getOpcode(uint16_t address) const {
    uint8_t index = bus->view(address);
    return lookup[index];
}

// Prints in a format that closely resembles the nestest.nes log found here: https://www.qmtpro.com/~nes/misc/nestest.log
void CPU::printDebug() const {
    uint8_t index = bus->view(pc);
    const Opcode& currentOpcode = lookup[index];
    const AddressingMode& mode = currentOpcode.addressingMode;

    std::cout << toHexString16(pc) << "  ";
    for (int i = 0; i < 3; i++) {
        if (i < mode.instructionSize) {
            std::cout << toHexString8(bus->view(pc + i)) << " ";
        }
        else {
            std::cout << "   ";
        }
    }
    std::string str = toString(pc);
    std::string gap(32 - static_cast<int>(str.length()), ' ');
    std::cout << " " << str << gap;

    std::cout
        << "A:" << toHexString8(a) \
        << " X:" << toHexString8(x) \
        << " Y:" << toHexString8(y) \
        << " P:" << toHexString8(sr) \
        << " SP:" << toHexString8(sp) \
        << " CYC:" << totalCycles - 1;

    std::cout << std::endl;
}