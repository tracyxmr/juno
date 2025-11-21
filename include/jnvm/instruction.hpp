#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace jnvm::inst
{
    ///@brief An enum of opcodes for the Virtual Machine.
    ///@details Each opcode is exactly one byte.
    enum class Opcode : std::uint8_t
    {
        MOV,    /// Move an immediate value to a register.
        ADD,    /// Add two registers and store it in another register.
        SUB,    /// Subtract two registers and store it in another register.
        MUL,    /// Multiply two registers and store it in another register.
        DIV,    /// Divide two registers and store it in another register.
        HLT,    /// Halt the machine.
        INC,    /// Increment a register by one.
        EQ,     /// Compare two registers, if they equal set the equal flag to true.
        JMP,    /// Unconditional jump
        JNZ,    /// Jump if not zero
        CALL,   /// Call a function or native function
        DBG     /// A DEBUG INSTRUCTION
    };

    ///@brief An enum of native functions ordered by their index.
    enum class VMNative : std::size_t
    {
        PRINT,
    };

    ///@brief A helper function which returns a string_view which contains the opcodes name.
    static std::string_view opcode_to_string( const Opcode opcode )
    {
        static std::unordered_map< Opcode, std::string_view > opcode_To_string {
            { Opcode::MOV, "MOV" },
            { Opcode::ADD, "ADD" },
            { Opcode::SUB, "SUB" },
            { Opcode::MUL, "MUL" },
            { Opcode::DIV, "DIV" },
            { Opcode::HLT, "HLT" },
            { Opcode::INC, "INC" },
            { Opcode::EQ, "EQ" },
            { Opcode::JMP, "JMP" },
            { Opcode::JNZ, "JNZ" },
            { Opcode::DBG, "DBG" },
        };

        return opcode_To_string.contains( opcode ) ? opcode_To_string[ opcode ] : "?";
    }

    ///@brief An abstract representation of a tightly packed instruction.
    ///@details The packed instruction follows this format: `[opcode][op1][op2][op3]` or `[8bits][8bits][8bits][8bits]`
    struct Instruction
    {
        std::uint32_t inst; /// A 32-bit integer where the data will be packed into.

        ///@brief Create an instruction.
        explicit Instruction(
            const Opcode opcode,
            const std::uint8_t o1 = 0,
            const std::uint8_t o2 = 0,
            const std::uint8_t o3 = 0
        ) : inst {
            static_cast< std::uint32_t >( opcode ) << 24 |
            static_cast< std::uint32_t >( o1 ) << 16 |
            static_cast< std::uint32_t >( o2 ) << 8 |
            static_cast< std::uint32_t >( o3 )
        } {}

        ///@brief Create an instruction from the data given.
        explicit Instruction(
            const std::uint32_t data
        ) : inst { data } {}

        ///@brief Returns the 32-bit integer.
        [[nodiscard]]
        std::uint32_t data() const { return inst; }

        ///@brief A helper function which unpacks to the position in the data which holds the opcode.
        [[nodiscard]]
        Opcode opcode() const { return static_cast< Opcode >( inst >> 24 ); }

        ///@brief A helper function which unpacks to the position in the data which holds the first operand.
        [[nodiscard]]
        std::uint8_t op1() const { return (inst >> 16) & 0xFF; }

        ///@brief A helper function which unpacks to the position in the data which holds the second operand.
        [[nodiscard]]
        std::uint8_t op2() const { return (inst >> 8) & 0xFF; }

        ///@brief A helper function which unpacks to the position in the data which holds the third operand.
        [[nodiscard]]
        std::uint8_t op3() const { return inst & 0xFF; }
    };
}