#pragma once
#include "instruction.hpp"

#include <array>
#include <cstdint>
#include <format>
#include <functional>
#include <print>
#include <span>
#include <stdexcept>
#include <vector>

namespace jnvm::machine
{
    using namespace jnvm::inst;

    constexpr std::size_t REGISTER_COUNT { 256 };

    static void error( const std::string& message )
    {
        throw std::runtime_error( std::format( "[jnvm::error] {}", message ) );
    }

    class Machine
    {
    public:
        explicit Machine() = default;

        ///@brief Load a new program into the machine.
        ///@details Clears all registers and resets the program counter.
        void load( const std::vector< std::uint32_t > &_bytecode )
        {
            bytecode = _bytecode;
            pc = 0;

            std::fill( std::begin( registers ), std::end( registers ), 0 );
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
                        }
                        break;
                    }

                    case Opcode::JMP:
                    {
                        pc = inst.op1(  );
                        break;
                    }

                    case Opcode::DBG:
                    {
                        const auto reg { inst.op1(  ) };
                        std::println( "[reg@{},pc@{}] {}", reg, pc, registers[ reg ] );
                        break;
                    }

                    case Opcode::HLT:
                    {
                        /// Terminate the machine and return the first register.
                        break;
                    }

                    case Opcode::CALL:
                    {
                        const VMNative native { inst.op1(  ) };
                        const auto base_reg { inst.op2(  ) };
                        const auto arg_count { inst.op3(  ) };

                        if ( natives.contains( native ) )
                        {
                            const auto fn { natives[ native ] };
                            fn( base_reg, arg_count );
                        }

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
        std::size_t pc { 0 };

        std::unordered_map< VMNative, std::function< void( std::uint8_t base_reg, std::size_t arg_count ) > > natives {
            /// print
            {
                    VMNative::PRINT, // index
                    [ this ]( const std::uint8_t base_reg, const std::size_t arg_count ) // function
                    {
                        for ( auto idx { base_reg }; idx < base_reg + arg_count; ++idx )
                        {
                            std::print("{}", this->registers[ idx ]);
                        }

                        std::println("");
                    }
            }
        };
    };
}