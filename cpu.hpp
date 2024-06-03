#ifndef CPU_HPP
#define CPU_HPP

#include "memory.hpp"

#include <stdint.h>
#include <optional>
#include <string>

class CPU{
public:
    CPU();
    void reset();
    void executeCycle();
    void executeNextInstruction();

    // Interrupts
    void IRQ();
    void NMI();

    bool load(const std::string& filePath, uint16_t location, uint16_t fileOffset);
    void printDebug(uint8_t index);

private:
    uint16_t pc; // program counter
    uint8_t a; // accumulator
    uint8_t x; // x register
    uint8_t y; // y register
    uint8_t sr; // status register
    uint8_t sp; // stack poitner

    static const uint16_t STACK_OFFSET = 0x100;

    static const uint16_t NMI_VECTOR = 0xFFFA;
    static const uint16_t RESET_VECTOR = 0xFFFC;
    static const uint16_t IRQ_BRK_VECTOR = 0xFFFE;

    void initCPU();

    Memory memory;

    enum Flags{
        CARRY,
        ZERO,
        INTERRUPT,
        DECIMAL, // Unimplemented
        BREAK,
        UNUSED,
        OVERFLOW,
        NEGATIVE
    };
    bool getFlag(Flags flag) const;
    void setFlag(Flags flag, bool value);
    void setNZFlags(uint8_t x);

    uint8_t remainingCycles;
    int64_t totalCycles;
    bool shouldAdvancePC;

    struct AddressingMode{
        struct ReturnType{
            std::optional<uint16_t> address;
            std::optional<uint8_t> data;
            bool mightNeedExtraCycle = 0;
        };
        uint8_t instructionSize;
        ReturnType (CPU::*getOperand)() const;
        std::string (CPU::*toString)(uint16_t address) const;
    };

    // Addressing mode functions
    AddressingMode::ReturnType ACC() const;
    AddressingMode::ReturnType ABS() const;
    AddressingMode::ReturnType ABX() const;
    AddressingMode::ReturnType ABY() const;
    AddressingMode::ReturnType IMM() const;
    AddressingMode::ReturnType IMP() const;
    AddressingMode::ReturnType IND() const;
    AddressingMode::ReturnType IZX() const;
    AddressingMode::ReturnType IZY() const;
    AddressingMode::ReturnType REL() const;
    AddressingMode::ReturnType ZPG() const;
    AddressingMode::ReturnType ZPX() const;
    AddressingMode::ReturnType ZPY() const;

    // Addressing mode string functions (used for disassembly)
    std::string strACC(uint16_t address) const;
    std::string strABS(uint16_t address) const;
    std::string strABX(uint16_t address) const;
    std::string strABY(uint16_t address) const;
    std::string strIMM(uint16_t address) const;
    std::string strIMP(uint16_t address) const;
    std::string strIND(uint16_t address) const;
    std::string strIZX(uint16_t address) const;
    std::string strIZY(uint16_t address) const;
    std::string strREL(uint16_t address) const;
    std::string strZPG(uint16_t address) const;
    std::string strZPX(uint16_t address) const;
    std::string strZPY(uint16_t address) const;
    
    struct Instruction{
        std::string name;
        void (CPU::*execute)(const AddressingMode::ReturnType& operand);
        bool mightNeedExtraCycle = 0;
    };

    // Instruction functions
    void ADC(const AddressingMode::ReturnType& operand);
    void AND(const AddressingMode::ReturnType& operand);
    void ASL(const AddressingMode::ReturnType& operand);
    void BCC(const AddressingMode::ReturnType& operand);
    void BCS(const AddressingMode::ReturnType& operand);
    void BEQ(const AddressingMode::ReturnType& operand);
    void BIT(const AddressingMode::ReturnType& operand);
    void BMI(const AddressingMode::ReturnType& operand);
    void BNE(const AddressingMode::ReturnType& operand);
    void BPL(const AddressingMode::ReturnType& operand);
    void BRK(const AddressingMode::ReturnType& operand);
    void BVC(const AddressingMode::ReturnType& operand);
    void BVS(const AddressingMode::ReturnType& operand);
    void CLC(const AddressingMode::ReturnType& operand);
    void CLD(const AddressingMode::ReturnType& operand);
    void CLI(const AddressingMode::ReturnType& operand);
    void CLV(const AddressingMode::ReturnType& operand);
    void CMP(const AddressingMode::ReturnType& operand);
    void CPX(const AddressingMode::ReturnType& operand);
    void CPY(const AddressingMode::ReturnType& operand);
    void DEC(const AddressingMode::ReturnType& operand);
    void DEX(const AddressingMode::ReturnType& operand);
    void DEY(const AddressingMode::ReturnType& operand);
    void EOR(const AddressingMode::ReturnType& operand);
    void INC(const AddressingMode::ReturnType& operand);
    void INX(const AddressingMode::ReturnType& operand);
    void INY(const AddressingMode::ReturnType& operand);
    void JMP(const AddressingMode::ReturnType& operand);
    void JSR(const AddressingMode::ReturnType& operand);
    void LDA(const AddressingMode::ReturnType& operand);
    void LDX(const AddressingMode::ReturnType& operand);
    void LDY(const AddressingMode::ReturnType& operand);
    void LSR(const AddressingMode::ReturnType& operand);
    void NOP(const AddressingMode::ReturnType& operand);
    void ORA(const AddressingMode::ReturnType& operand);
    void PHA(const AddressingMode::ReturnType& operand);
    void PHP(const AddressingMode::ReturnType& operand);
    void PLA(const AddressingMode::ReturnType& operand);
    void PLP(const AddressingMode::ReturnType& operand);
    void ROL(const AddressingMode::ReturnType& operand);
    void ROR(const AddressingMode::ReturnType& operand);
    void RTI(const AddressingMode::ReturnType& operand);
    void RTS(const AddressingMode::ReturnType& operand);
    void SBC(const AddressingMode::ReturnType& operand);
    void SEC(const AddressingMode::ReturnType& operand);
    void SED(const AddressingMode::ReturnType& operand);
    void SEI(const AddressingMode::ReturnType& operand);
    void STA(const AddressingMode::ReturnType& operand);
    void STX(const AddressingMode::ReturnType& operand);
    void STY(const AddressingMode::ReturnType& operand);
    void TAX(const AddressingMode::ReturnType& operand);
    void TAY(const AddressingMode::ReturnType& operand);
    void TSX(const AddressingMode::ReturnType& operand);
    void TXA(const AddressingMode::ReturnType& operand);
    void TXS(const AddressingMode::ReturnType& operand);
    void TYA(const AddressingMode::ReturnType& operand);
    void UNI(const AddressingMode::ReturnType& operand); // Handles unimplemented instructions

    struct Opcode{
        Instruction instruction;
        AddressingMode addressingMode;
        uint8_t numDefaultCycles;
    };
    std::string toString(const Opcode& opcode, uint16_t address) const;

    const static int MAX_NUM_OPCODES = 0x100;
    std::array<Opcode, MAX_NUM_OPCODES> lookup;
    void initLookup();

    uint16_t read16BitData(uint16_t address) const;
    void write16BitData(uint16_t address, uint16_t data);

    void push8BitDataToStack(uint8_t data);
    uint8_t pop8BitDataFromStack();

    void push16BitDataToStack(uint16_t data);
    uint16_t pop16BitDataFromStack();
};

#endif // CPU_HPP