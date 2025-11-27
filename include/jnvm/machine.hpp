#pragma once
#include "instruction.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <iostream>
#include <print>
#include <span>
#include <stack>
#include <stdexcept>
#include <vector>

namespace jnvm::machine
{
    using namespace jnvm::inst;

    constexpr std::size_t REGISTER_COUNT { 256 };
    constexpr std::size_t MAX_CALL_DEPTH { 1024 };

    /*
     * Refactor message:
     *
     * I noticed I did not implement proper error handling before
     * so I've decided to implement a RuntimeError class.
     *
     */
    class RuntimeError final : public std::runtime_error
    {
    public:
        explicit RuntimeError( const std::string& msg )
            : std::runtime_error { std::format( "[jnvm::runtime_error] {}", msg ) }
        {}
    };

    ///@brief This structure represents a stack frame which is essential for tracking function calls.
    struct StackFrame
    {
        ///@brief Where the machine should return to after calling
        std::size_t return_addr;
        ///@brief The base register for the frames local variables
        std::uint8_t frame_ptr;
        ///@brief The amount of parameters passed.
        std::uint8_t param_count;
        ///@brief The register to which the result will be stored in.
        std::uint8_t result_reg;
        ///@brief An array of registers which the frame must save
        std::array< std::uint32_t, REGISTER_COUNT > saved_regs { };

        StackFrame(
            const std::size_t return_addr,
            const std::uint8_t frame_ptr,
            const std::uint8_t param_count,
            const std::uint8_t result_reg,
            const std::array< std::uint32_t, REGISTER_COUNT > &saved_regs
        ) :
            return_addr { return_addr },
            frame_ptr { frame_ptr },
            param_count { param_count },
            result_reg { result_reg },
            saved_regs { saved_regs }
        {}
    };

    /// First Argument: registers
    /// Second Argument: base_reg
    /// Third Argument: arg_count
    /// Fourth Argument: string_pool
    using VMNative = std::function< void(
        std::array< std::uint32_t, REGISTER_COUNT >&,
        std::uint32_t,
        std::uint8_t,
        const std::vector< std::string >&
    ) >;

    class Machine
    {
    public:
        explicit Machine()
        {
            load_natives(  );
        }

        explicit Machine( const bool debug ) : m_dbg { debug }
        {
            load_natives(  );
        }

        ///@brief Load a new program into the machine.
        ///@details Clears all registers and resets the program counter, and the call stack.
        void load( const std::vector< std::uint32_t > &_bytecode )
        {
            m_bytecode = _bytecode;
            reset(  );
        }

        ///@brief Load a vector of strings to the string pool in the machine.
        void load_strings( const std::vector< std::string >& string_pool_ )
        {
            m_string_pool = string_pool_;
        }

        ///@brief Register a native function into the virtual machine
        void register_native( const VMNativeID id, VMNative func )
        {
            m_natives[ id ] = std::move( func );
        }

        ///@brief Execute the bytecode.
        ///@return The result of the first register.
        [[nodiscard]]
        std::uint32_t execute( )
        {
            if ( m_bytecode.empty(  ) ) throw RuntimeError( "No bytecode to execute." );

            while ( m_pc < m_bytecode.size(  ) )
            {
                execute_one(  );
                if ( m_halted )
                {
                    if ( m_dbg ) std::println("Registers: {}", m_registers);

                    return m_registers[ 0 ];
                };
            }

            throw RuntimeError( "Program was aborted without a HLT instruction, please check your compiler." );
        }

    private:
        std::array< std::uint32_t, REGISTER_COUNT > m_registers { };
        std::vector< std::uint32_t > m_bytecode;
        std::vector< std::string > m_string_pool;
        std::size_t m_pc { 0 };     /// Program counter
        std::size_t m_fp { 0 };     /// Frame pointer
        bool m_halted { false };
        bool m_dbg { false };

        std::stack< StackFrame > m_call_stack;
        std::unordered_map< VMNativeID, VMNative > m_natives;

        std::chrono::steady_clock::time_point m_profile_start;
        std::size_t m_profile_instructions_count { 0 };

        void reset()
        {
            m_registers.fill(  0 );
            m_pc = 0;
            m_fp = 0;
            m_halted = false;
            m_profile_instructions_count = 0;

            while ( !m_call_stack.empty(  ) ) m_call_stack.pop(  );
        }

        ///@brief Will execute the move instruction
        ///@details Move the immediate value (operand 2) into the register (operand 1)
        void execute_mov( const Instruction instruction )
        {
            m_registers[ instruction.op1(  ) ] = instruction.op2(  );
        }

        ///@brief Will execute the copy instruction.
        ///@details Copy the value of a source register (operand 2) into a target register (operand 1)
        void execute_copy( const Instruction instruction )
        {
            m_registers[ instruction.op1() ] = m_registers[ instruction.op2(  ) ];
        }

        ///@brief Will execute the load string instruction.
        ///@details Will check if the index (operand 1) of the string exceeds the string pool size
        ///if it does it'll error, else it'll mark the index with a flag and place it into
        ///the register (operand 1)
        void execute_loads( const Instruction instruction )
        {
            const auto str_idx { instruction.op2( ) };
            if ( str_idx >= m_string_pool.size(  ) ) throw RuntimeError( "String pool index is out of bounds.");

            m_registers[ instruction.op1( ) ] = make_idx_for_string( str_idx );
        }

        ///@brief Will execute the add instruction.
        ///@details Add the value of register one (operand 1) and register two (operand 2) and place
        ///the result in register three (operand 3)
        void execute_add( const Instruction instruction )
        {
            m_registers[ instruction.op3( ) ] = m_registers[ instruction.op1( ) ] + m_registers[ instruction.op2( ) ];
        }

        ///@brief Will execute the subtract instruction.
        ///@details Subtract the value of register one (operand 1) and register two (operand 2) and place
        ///the result in register three (operand 3)
        void execute_sub( const Instruction instruction )
        {
            m_registers[ instruction.op3( ) ] = m_registers[ instruction.op1( ) ] - m_registers[ instruction.op2( ) ];
        }

        ///@brief Will execute the multiply instruction.
        ///@details Multiply the value of register one (operand 1) and register two (operand 2) and place
        ///the result in register three (operand 3)
        void execute_mul( const Instruction instruction )
        {
            m_registers[ instruction.op3( ) ] = m_registers[ instruction.op1( ) ] * m_registers[ instruction.op2( ) ];
        }

        ///@brief Will execute the divide instruction.
        ///@details Divide the value of register one (operand 1) and register two (operand 2) and place
        ///the result in register three (operand 3)
        void execute_div( const Instruction instruction )
        {
            const auto rhs { m_registers[ instruction.op2(  ) ] };
            if ( rhs == 0 ) throw RuntimeError( "Division by zero." );
            m_registers[ instruction.op3( ) ] = m_registers[ instruction.op1( ) ] / rhs;
        }

        ///@brief Will execute the increment instruction.
        ///@details Increment the register's (operand 1) value.
        void execute_inc( const Instruction instruction )
        {
            m_registers[ instruction.op1(  ) ]++;
        }

        ///@brief Will execute the unconditional jump instruction.
        ///@details Set the program counter value to the jump instructions desired place / address.
        void execute_jmp( const Instruction instruction )
        {
            m_pc = instruction.op1( );
        }

        ///@brief Will execute the jump-if-not-zero instruction.
        ///@details Set the program counter value to the jump-if-not-zero instructions desired place / address.
        void execute_jnz( const Instruction instruction )
        {
            if ( m_registers[ instruction.op1(  ) ] != 0 ) m_pc = instruction.op2(  );
            else m_pc++;
        }

        ///@brief Executes a VM native function
        void execute_vm_native( std::uint8_t addr, const std::uint8_t base_reg, const std::uint8_t arg_count )
        {
            const auto fn { static_cast< VMNativeID >( addr ) };
            if ( !m_natives.contains( fn ) ) throw RuntimeError( "Unknown native function." );

            m_natives[ fn ]( m_registers, base_reg, arg_count, m_string_pool );
        }

        ///@brief Executes a user defined function
        void execute_user_func( const std::uint8_t addr, std::uint8_t base_reg, std::uint8_t arg_count )
        {
            if ( m_call_stack.size(  ) >= MAX_CALL_DEPTH ) throw RuntimeError( "Call stack overflow." );

            /* Save state */
            m_call_stack.emplace( m_pc + 1, m_fp, arg_count, base_reg, m_registers );
            /* Setup */
            m_fp = base_reg;
            m_pc = addr;
        }

        ///@brief Will execute the call instruction.
        /// First Operand: the functions address
        /// Second Operand: the start register for the arguments
        /// Third Operand: the amount of arguments
        void execute_call( const Instruction instruction )
        {
            const auto addr { instruction.op1(  ) };
            const auto base_reg { instruction.op2(  ) };
            const auto arg_count { instruction.op3(  ) };

            /* Check if the function address maps to a VM native function */
            if ( is_vm_native( addr ) )
            {
                execute_vm_native( addr, base_reg, arg_count );
                m_pc++;
            } else
            {
                execute_user_func( addr, base_reg, arg_count );
            }
        }

        ///@brief Will execute the return instruction.
        ///If RET is called from the main program, it'll be halted.
        ///Else
        ///The value in the first register is for the return value,
        ///so it'll be saved, then the VM will restore the caller state,
        ///which includes resetting the registers that were saved, the program counter
        ///and the frame pointer, and the first register (reg 0) will be restored, and
        ///the call stack will be popped.
        void execute_ret( const Instruction instruction )
        {
            /* This is the first check in the return instruction */
            if ( m_call_stack.empty(  ) ) { m_halted = true; return; }

            /* Save the value in register zero, it will be restored */
            const auto saved_value { m_registers[ 0 ] };

            /* Restore */
            const auto& f { m_call_stack.top(  ) }; /// Top stack frame
            m_registers = f.saved_regs;
            m_pc = f.return_addr;
            m_fp = f.frame_ptr;

            m_registers[ f.result_reg ] = saved_value;
            m_call_stack.pop(  );
        }

        ///@brief Will execute the profile instruction
        ///Will be started with a steady clock of ::now( )
        void execute_prf( const Instruction instruction )
        {
            m_profile_start = std::chrono::steady_clock::now(  );
        }

        ///@brief Will execute the profile end instruction
        ///Will automatically print the amount of instructions executed.
        void execute_prfe( const Instruction instruction )
        {
            const auto end { std::chrono::steady_clock::now(  ) };
            const auto dur { std::chrono::duration_cast< std::chrono::milliseconds >( end - m_profile_start ) };
            std::println("Block executed in {} (processed {} instructions)", dur, m_profile_instructions_count);
        }

        ///@brief Will execute the halt instruction
        ///Simply sets the halted flag to true.
        void execute_hlt( const Instruction instruction )
        {
            m_halted = true;
        }

        void execute_jz( const Instruction instruction )
        {
            if ( m_registers[ instruction.op1(  ) ] == 0 ) m_pc = instruction.op2(  );
            else m_pc++;
        }

        void execute_eq( const Instruction instruction )
        {
            m_registers[ instruction.op3(  ) ] = ( m_registers[ instruction.op1(  ) ] == m_registers[ instruction.op2(  ) ] ) ? 1 : 0;
        }

        void execute_neq( const Instruction instruction )
        {
            m_registers[ instruction.op3(  ) ] = ( m_registers[ instruction.op1(  ) ] != m_registers[ instruction.op2(  ) ] ) ? 1 : 0;
        }

        void execute_lt( const Instruction instruction )
        {
            m_registers[ instruction.op3(  ) ] = ( m_registers[ instruction.op1(  ) ] < m_registers[ instruction.op2(  ) ] ) ? 1 : 0;
        }

        void execute_gt( const Instruction instruction )
        {
            m_registers[ instruction.op3(  ) ] = ( m_registers[ instruction.op1(  ) ] > m_registers[ instruction.op2(  ) ] ) ? 1 : 0;
        }

        void execute_lte( const Instruction instruction )
        {
            m_registers[ instruction.op3(  ) ] = ( m_registers[ instruction.op1(  ) ] <= m_registers[ instruction.op2(  ) ] ) ? 1 : 0;
        }

        void execute_gte( const Instruction instruction )
        {
            m_registers[ instruction.op3(  ) ] = ( m_registers[ instruction.op1(  ) ] >= m_registers[ instruction.op2(  ) ] ) ? 1 : 0;
        }

        ///@brief Execute a single instruction.
        void execute_one()
        {
            /* Check if the program counter exceeds the bytecode capacity */
            if ( m_pc >= m_bytecode.size(  ) ) throw RuntimeError( "Program counter is out of bounds." );

            /* Encode the byte into an instruction */
            const Instruction i { m_bytecode[ m_pc ] };

            /* Increment the profiler instruction count */
            m_profile_instructions_count++;

            switch ( i.opcode(  ) )
            {
                case Opcode::MOV:   execute_mov( i );   m_pc++; break;
                case Opcode::COPY:  execute_copy( i );  m_pc++; break;
                case Opcode::LOADS: execute_loads( i ); m_pc++; break;
                case Opcode::ADD:   execute_add( i );   m_pc++; break;
                case Opcode::SUB:   execute_sub( i );   m_pc++; break;
                case Opcode::MUL:   execute_mul( i );   m_pc++; break;
                case Opcode::DIV:   execute_div( i );   m_pc++; break;
                case Opcode::INC:   execute_inc( i );   m_pc++; break;
                case Opcode::JMP:   execute_jmp( i );   break;
                case Opcode::JNZ:   execute_jnz( i );   break;
                case Opcode::JZ:    execute_jz(i);      break;
                case Opcode::EQ:    execute_eq(i);      m_pc++; break;
                case Opcode::NEQ:   execute_neq(i);     m_pc++; break;
                case Opcode::LT:    execute_lt(i);      m_pc++; break;
                case Opcode::GT:    execute_gt(i);      m_pc++; break;
                case Opcode::LTE:   execute_lte(i);     m_pc++; break;
                case Opcode::GTE:   execute_gte(i);     m_pc++; break;
                case Opcode::CALL:  execute_call( i );  break;
                case Opcode::RET:   execute_ret( i );   break;
                case Opcode::PRF:   execute_prf( i );   m_pc++; break;
                case Opcode::PRFE:  execute_prfe( i );  m_pc++; break;
                case Opcode::HLT:   execute_hlt( i );   return;
                default: throw RuntimeError("I have just encountered an unknown opcode...");
            }
        }

        ///@brief Register native functions
        void load_natives( )
        {
            register_native(
                VMNativeID::PRINT,
                []( auto& registers, const std::uint8_t base_reg, const std::uint8_t arg_count, const auto& string_pool )
                {
                    for ( std::uint8_t offset { 0 }; offset < arg_count; offset++ )
                    {
                        if ( const auto value { registers[ base_reg + offset ] }; is_string_value( value ) )
                        {
                            const auto idx { get_string_idx( value ) };
                            if ( idx < string_pool.size(  ) ) std::print("{} ", string_pool[ idx ]);
                        } else
                        {
                            std::print("{} ", value );
                        }
                    }

                    std::cout << std::endl;
                }
            );
        }
    };
}