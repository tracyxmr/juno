#pragma once
#include "instruction.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <print>
#include <span>
#include <stack>
#include <stdexcept>
#include <vector>

namespace jnvm::machine
{
    using namespace jnvm::inst;

    constexpr std::size_t REGISTER_COUNT { 256 };
    constexpr std::size_t STACK_SIZE { 4096 };

    static void error( const std::string& message )
    {
        throw std::runtime_error( std::format( "[jnvm::error] {}", message ) );
    }

    ///@brief This structure represents a stack frame which is essential for tracking function calls.
    struct StackFrame
    {
        ///@brief Where the machine should return to after calling
        std::size_t return_addr;
        ///@brief The base register for the frames local variables
        std::uint8_t frame_ptr;
        ///@brief The amount of parameters passed.
        std::uint8_t param_count;
        ///@brief The amount of Local variables.
        std::uint8_t local_count;
        ///@brief The register to which the result will be stored in.
        std::uint8_t result_reg;
        ///@brief An array of registers which the frame must save
        std::array< std::uint32_t, REGISTER_COUNT > saved_regs { };

        StackFrame( const std::size_t return_addr, const std::uint8_t frame_ptr, const std::uint8_t param_count,
                    const std::uint8_t local_count, const std::uint8_t result_reg, const std::array< std::uint32_t, REGISTER_COUNT > &saved_regs ) :
            return_addr { return_addr },
            frame_ptr { frame_ptr },
            param_count { param_count },
            local_count { local_count },
            result_reg { result_reg },
            saved_regs { saved_regs }
        {
        }
    };

    class Machine
    {
    public:
        explicit Machine() = default;

        ///@brief Load a new program into the machine.
        ///@details Clears all registers and resets the program counter, and the call stack.
        void load( const std::vector< std::uint32_t > &_bytecode )
        {
            bytecode = _bytecode;
            pc = 0;

            std::fill( std::begin( registers ), std::end( registers ), 0 );
            while ( !call_stack.empty(  ) ) call_stack.pop(  );
        }

        ///@brief Execute the bytecode.
        ///@return The result of the first register.
        std::uint32_t execute( )
        {
            while ( pc < bytecode.size(  ) )
            {
                Instruction inst { bytecode[ pc ] };

                switch ( const auto opcode { inst.opcode( ) } )
                {
                    case Opcode::MOV:
                    {
                        const auto dest_reg { inst.op1(  ) };
                        const auto imm { inst.op2(  ) };

                        registers[ dest_reg ] = imm;
                        break;
                    }

                    case Opcode::MOVR:
                    {
                        const auto dest_reg { inst.op1(  ) };
                        const auto src_reg { inst.op2(  ) };

                        registers[ dest_reg ] = registers[ src_reg ];
                        break;
                    }

                    case Opcode::ADD:
                    {
                        const auto first_reg { inst.op1(  ) };
                        const auto second_reg { inst.op2(  ) };
                        const auto dest_reg { inst.op3(  ) };

                        registers[ dest_reg ] = registers[ first_reg ] + registers[ second_reg ];
                        break;
                    }

                    case Opcode::SUB:
                    {
                        const auto first_reg { inst.op1(  ) };
                        const auto second_reg { inst.op2(  ) };
                        const auto dest_reg { inst.op3(  ) };

                        registers[ dest_reg ] = registers[ first_reg ] - registers[ second_reg ];
                        break;
                    }

                    case Opcode::MUL:
                    {
                        const auto first_reg { inst.op1(  ) };
                        const auto second_reg { inst.op2(  ) };
                        const auto dest_reg { inst.op3(  ) };

                        registers[ dest_reg ] = registers[ first_reg ] * registers[ second_reg ];
                        break;
                    }

                    case Opcode::DIV:
                    {
                        const auto first_reg { inst.op1(  ) };
                        const auto second_reg { inst.op2(  ) };
                        const auto dest_reg { inst.op3(  ) };

                        if ( first_reg == 0 || second_reg == 0 ) error( "attempted zero-division!" );

                        registers[ dest_reg ] = registers[ first_reg ] / registers[ second_reg ];
                        break;
                    }

                    case Opcode::INC:
                    {
                        const auto reg { inst.op1(  ) };
                        registers[ reg ]++;
                        break;
                    }

                    case Opcode::JNZ:
                    {
                        if ( registers[ inst.op1(  ) ] != 0 )
                        {
                            pc = inst.op2(  );
                            continue; /// Without this, the pc++ at the end would still happen
                        }
                        break;
                    }

                    case Opcode::JMP:
                    {
                        pc = inst.op1(  );
                        continue;
                    }

                    case Opcode::HLT:
                    {
                        return registers[ 0 ];
                    }

                    case Opcode::PRF:
                    {
                        profiler_start_time = std::chrono::steady_clock::now(  );
                        break;
                    }

                    case Opcode::PRFE:
                    {
                        auto end { std::chrono::steady_clock::now(  ) };
                        auto dur { std::chrono::duration_cast< std::chrono::microseconds >( end - profiler_start_time ) };
                        std::println( "Block took {}, processed {} instructions.", dur, pc+1 );
                        break;
                    }

                    case Opcode::CALL:
                    {
                        const auto func_addr{ inst.op1(  ) };
                        const auto base_reg { inst.op2(  ) };
                        const auto arg_count { inst.op3(  ) };

                        /// The virtual machine is expected to call both native and user functions now.
                        if ( func_addr >= 128 ) /// IS it a native?
                        {
                            if ( const auto id { static_cast< VMNative >( func_addr ) }; natives.contains( id ) )
                            {
                                const auto fn { natives[ id ] };
                                fn( base_reg, arg_count );
                            } else
                            {
                                error( std::format( "Unknown native function '{}'", func_addr ) );
                            }
                        } else
                        {
                            call_fn( func_addr, base_reg, arg_count );
                        }

                        break;
                    }

                    case Opcode::RET:
                    {
                        return_from_fn();
                        break;
                    }

                    default: error( std::format( "unknown opcode {} was caught!", opcode_to_string( opcode ) ) );
                }

                pc++;
            }

            std::println( "Registers: {}", registers );

            return registers[ 0 ];
        }

    private:
        std::array< std::uint32_t, REGISTER_COUNT > registers { };
        std::vector< std::uint32_t > bytecode { };
        ///@brief Program counter.
        std::size_t pc { 0 };
        ///@brief Frame pointer.
        std::uint8_t fp { 0 };

        std::stack< StackFrame > call_stack;
        ///@brief Functions ID mapped to it's bytecode address (position).
        std::unordered_map< std::uint8_t, std::size_t > user_funcs;
        std::chrono::steady_clock::time_point profiler_start_time;

        ///@brief This method will call a user defined function.
        void call_fn( const std::uint8_t fn_addr, const std::uint8_t base_reg, const std::uint8_t arg_count )
        {
            const StackFrame frame( pc + 1, fp, arg_count, 0, base_reg, registers );
            call_stack.push( frame );
            fp = base_reg;
            pc = fn_addr;
            pc--; /// DO NOT increment, i found this out because execute() already handles it but we'll end up skipped ahead if we inc
        }

        ///@brief Return from a user defined function
        void return_from_fn()
        {
            if ( call_stack.empty(  ) ) error( "Return called with empty call stack" );

            const auto return_val { registers[ 0 ] };
            const auto frame { call_stack.top(  ) };
            call_stack.pop(  );

            /// Restore
            registers = frame.saved_regs;
            pc = frame.return_addr;
            fp = frame.frame_ptr;
            /// Store in register 0, the callers register.
            registers[ frame.result_reg ] = return_val;
        }

        std::unordered_map< VMNative, std::function< void( std::uint8_t base_reg, std::size_t arg_count ) > > natives {
            /// print
                {
                    VMNative::PRINT, // index
                    [ this ]( const std::uint8_t base_reg, const std::size_t arg_count ) // function
                    {
                        for ( auto idx { base_reg }; idx < base_reg + arg_count; ++idx )
                        {
                            std::print("{} ", this->registers[ idx ]);
                        }

                        std::println("");
                    }
                }
        };
    };
}