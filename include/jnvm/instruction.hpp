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
        MOV,    /// Move an immediate value to a register
        COPY,   /// Copy a register to another register
        LOADS,  /// Load a string constant from the pool

        ADD,    /// Add two register values into a result register
        SUB,    /// Subtract two register values into a result register
        MUL,    /// Multiply two register values into a result register
        DIV,    /// Divide two register values into a result register

        JMP,    /// An unconditional jump
        JNZ,    /// Will jump if not zero
        JZ,     /// Will jump if zero
        CALL,   /// Call a native or user defined function
        RET,    /// Return from a function

        EQ,     /// Two registers are equal
        NEQ,    /// Two registers are not equal
        LT,     /// If register 1 is less than register 2, store in result register.
        GT,     /// If register 1 is greater than register 2, store in result register.
        LTE,    /// If register 1 is less than or equal to register 2, store in result register.
        GTE,    /// If register 1 is greater than or equal to register 2, store in result register.

        INC,    /// Increment a registers value
        DEC,    /// Decrement a registers value
        PRF,    /// Start the profiler
        PRFE,   /// End the profiler
        HLT,    /// Halt execution of the virtual machine
    };

    ///@brief An enum of native functions ordered by their index.
    enum class VMNativeID : std::size_t
    {
        PRINT = 128,
    };

    ///@brief A helper function which returns a string_view which contains the opcodes name.
    static std::string_view opcode_to_string( const Opcode opcode )
    {
        /*
         *  I don't know why I used a map for this before, decided to switch back to a switch statement.
         */

        switch ( opcode )
        {
            case Opcode::MOV:   return "MOV";
            case Opcode::COPY:  return "COPY";
            case Opcode::LOADS: return "LOADS";
            case Opcode::ADD:   return "ADD";
            case Opcode::SUB:   return "SUB";
            case Opcode::MUL:   return "MUL";
            case Opcode::DIV:   return "DIV";
            case Opcode::JMP:   return "JMP";
            case Opcode::JNZ:   return "JNZ";
            case Opcode::CALL:  return "CALL";
            case Opcode::RET:   return "RET";
            case Opcode::INC:   return "INC";
            case Opcode::EQ:    return "EQ";
            case Opcode::PRF:   return "PRF";
            case Opcode::PRFE:  return "PRFE";
            case Opcode::HLT:   return "HLT";
            default:            return "UNKNOWN";
        }
    }

    ///@brief An abstract representation of a tightly packed instruction.
    ///@details The packed instruction follows this format: `[opcode][op1][op2][op3]` or `[8bits][8bits][8bits][8bits]`
    struct Instruction
    {
        ///@brief Create an instruction.
        explicit constexpr Instruction(
            const Opcode opcode,
            const std::uint8_t o1 = 0,
            const std::uint8_t o2 = 0,
            const std::uint8_t o3 = 0
        ) noexcept : inst {
            static_cast< std::uint32_t >( opcode ) << 24 |
            static_cast< std::uint32_t >( o1 ) << 16 |
            static_cast< std::uint32_t >( o2 ) << 8 |
            static_cast< std::uint32_t >( o3 )
        } {}

        ///@brief Create an instruction from the data given.
        explicit constexpr Instruction(
            const std::uint32_t data
        ) noexcept : inst { data } {}

        ///@brief Returns the 32-bit integer.
        [[nodiscard]]
        constexpr std::uint32_t data() const noexcept { return inst; }

        ///@brief A helper function which unpacks to the position in the data which holds the opcode.
        [[nodiscard]]
        constexpr Opcode opcode() const noexcept { return static_cast< Opcode >( inst >> 24 ); }

        ///@brief A helper function which unpacks to the position in the data which holds the first operand.
        [[nodiscard]]
        constexpr std::uint8_t op1() const noexcept { return (inst >> 16) & 0xFF; }

        ///@brief A helper function which unpacks to the position in the data which holds the second operand.
        [[nodiscard]]
        constexpr std::uint8_t op2() const noexcept { return (inst >> 8) & 0xFF; }

        ///@brief A helper function which unpacks to the position in the data which holds the third operand.
        [[nodiscard]]
        constexpr std::uint8_t op3() const noexcept { return inst & 0xFF; }

    private:
        std::uint32_t inst; /// A 32-bit integer where the data will be packed into.
    };

    ///@brief Returns true if the function is a VM native function
    [[nodiscard]]
    constexpr bool is_vm_native( const std::uint8_t addr ) noexcept { return addr >= 128; }

    ///@brief Returns true if the value of the register passed is a string pool index by checking for the high bit set flag
    [[nodiscard]]
    constexpr bool is_string_value( const std::uint32_t value ) noexcept { return ( value & 0x80000000 ) != 0; }

    ///@brief Extract the index for the string pool from a register value
    [[nodiscard]]
    constexpr std::uint32_t get_string_idx( const std::uint32_t value ) noexcept { return value & ~0x80000000; }

    ///@brief A helper function which creates a string pool index value
    [[nodiscard]]
    constexpr std::uint32_t make_idx_for_string( const std::uint32_t idx ) noexcept { return idx | 0x80000000; }
}