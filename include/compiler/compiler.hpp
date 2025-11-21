#pragma once

#include "jnvm/instruction.hpp"

#include <vector>
#include <parser/ast.hpp>

class Compiler
{
public:
    explicit Compiler(
        std::vector< std::unique_ptr< Statement > > &&_ast
    ) : ast { std::move(_ast) } {}

    ///@brief Compile the loaded AST into JNVM bytecode.
    std::vector< std::uint32_t > compile( );
private:
    std::vector< std::unique_ptr< Statement > > ast;
    std::vector< std::uint32_t > bytecode;
    std::uint8_t nx_register { 0 }; /// The next register to be allocated.

    ///@brief Emits the instruction into it's underlying integer value.
    void emit( const jnvm::inst::Instruction& inst );

    ///@brief This method will compile a statement.
    void comp_statement( const Statement& stmt );

    ///@brief This method will compile an expression and returns a register index.
    std::uint8_t comp_expression( const Expression* expr );
};