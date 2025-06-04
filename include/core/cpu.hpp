#ifndef CPU_HPP
#define CPU_HPP

#include "util/serializer.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <variant>

class Bus;

class CPU {
public:
    CPU(Bus& bus);
    void initCPU();

    void executeCycle();

    // Reset/interrupts
    void reset();
    bool IRQ();
    void NMI();

    // Data structures
    enum class Flag {
        CARRY,
        ZERO,
        INTERRUPT,
        DECIMAL, // Unimplemented - not used on the NES
        BREAK,
        UNUSED,
        OVERFLOW_, // Underscore to avoid naming conflict with a Windows library
        NEGATIVE
    };
    struct AddressingMode {
        struct ReturnType {
            // This is the variable that will be populated after we determine the instruction's addressing mode.
            // Addressing modes can either return a 16-bit address, an 8-bit data value, or nothing (in the case of IMP).
            // These three options are captured in the below std::variant
            const std::variant<uint16_t, uint8_t, std::monostate> addressingModeOutput;

            bool hasAddress() const;
            bool hasData() const;

            bool mightNeedExtraCycle = 0;
        };
        uint8_t instructionSize;
        ReturnType(CPU::* getOperand)();
        std::string(CPU::* toString)(uint16_t address) const;
    };
    struct Instruction {
        std::string name;
        void (CPU::* execute)(const AddressingMode::ReturnType& operand);
        bool mightNeedExtraCycle = 0;
    };
    struct Opcode {
        Instruction instruction;
        AddressingMode addressingMode;
        uint8_t numDefaultCycles;
    };

    uint16_t getAddress(const AddressingMode::ReturnType& operand) const;
    uint8_t getDataView(const AddressingMode::ReturnType& operand) const;
    uint8_t getDataRead(const AddressingMode::ReturnType& operand);

    // Getters for internal variables
    uint16_t getPC() const;
    uint8_t getA() const;
    uint8_t getX() const;
    uint8_t getY() const;
    uint8_t getSP() const;
    uint8_t getSR() const;
    bool getFlag(Flag flag) const;
    uint8_t getFlagMask(Flag flag) const;
    uint8_t getRemainingCycles() const;

    // Debugging
    std::string toString(uint16_t address) const;
    const Opcode& getOpcode(uint16_t address) const;

    // Serialization
    void serialize(Serializer& s) const;
    void deserialize(Deserializer& d);

private:
    // Constants
    static constexpr uint16_t NMI_VECTOR = 0xFFFA;
    static constexpr uint16_t RESET_VECTOR = 0xFFFC;
    static constexpr uint16_t IRQ_BRK_VECTOR = 0xFFFE;
    static constexpr uint16_t STACK_OFFSET = 0x100;
    static constexpr uint16_t MAX_NUM_OPCODES = 0x100;

    // CPU state variables
    uint16_t pc; // program counter
    uint8_t a; // accumulator
    uint8_t x; // x register
    uint8_t y; // y register
    uint8_t sr; // status register
    uint8_t sp; // stack pointer

    // Helper variables
    uint8_t remainingCycles;
    bool shouldAdvancePC;
    static const std::array<Opcode, MAX_NUM_OPCODES> lookup;

    Bus& bus;

    // Initialization
    static std::array<Opcode, MAX_NUM_OPCODES> initLookup();

    // Flags
    void setFlag(Flag flag, bool value);
    void setNZFlags(uint8_t x);

    // Reading/writing data
    uint16_t view16BitData(uint16_t address) const;
    uint16_t read16BitData(uint16_t address);
    void write16BitData(uint16_t address, uint16_t data);

    void push8BitDataToStack(uint8_t data);
    uint8_t pop8BitDataFromStack();
    void push16BitDataToStack(uint16_t data);
    uint16_t pop16BitDataFromStack();

    // Addressing mode functions
    AddressingMode::ReturnType ACC();
    AddressingMode::ReturnType ABS();
    AddressingMode::ReturnType ABX();
    AddressingMode::ReturnType ABY();
    AddressingMode::ReturnType IMM();
    AddressingMode::ReturnType IMP();
    AddressingMode::ReturnType IND();
    AddressingMode::ReturnType IZX();
    AddressingMode::ReturnType IZY();
    AddressingMode::ReturnType REL();
    AddressingMode::ReturnType ZPG();
    AddressingMode::ReturnType ZPX();
    AddressingMode::ReturnType ZPY();

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
};

#endif // CPU_HPP