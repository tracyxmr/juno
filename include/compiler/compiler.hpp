#pragma once

#include "evaluator/eval_visitor.hpp"
#include "jnvm/instruction.hpp"

#include <vector>
#include <parser/ast.hpp>

class Compiler
{
public:
    explicit Compiler(
        std::vector< std::unique_ptr< Statement > > &&_ast
    ) : ast { std::move(_ast) } {}

    struct Scope
    {
        std::uint8_t start_reg { 0 };
        std::unordered_map< std::string_view, std::uint8_t > variables { };

        explicit Scope( const std::uint8_t _start_reg ) : start_reg { _start_reg } { }
    };

    ///@brief Compile the loaded AST into JNVM bytecode.
    std::vector< std::uint32_t > compile( );
private:
    std::vector< std::unique_ptr< Statement > > ast;
    std::vector< std::uint32_t > bytecode;
    std::uint8_t nx_register { 0 }; /// The next register to be allocated.
    std::vector< Scope > scopes { };

    std::unordered_map< std::string, std::size_t > function_addrs;

    std::unordered_map< std::string_view, jnvm::inst::VMNative > native_map {
        { "print", jnvm::inst::VMNative::PRINT }
    };

    ///@brief Emits the instruction into it's underlying integer value.
    void emit( const jnvm::inst::Instruction& inst );

    ///@brief Enters the compiler into the top scope.
    void enter_scope( );

    ///@brief Exists the top scope and sets the next register to be the start register
    ///of the scope so they can be overwritten.
    void exit_scope( );

    ///@brief This method will compile a statement.
    void comp_statement( const Statement& stmt );

    ///@brief This method will compile a function prototype.
    void comp_prototype( const FunctionPrototype& proto );

    ///@brief This method will compile an expression and returns a register index.
    std::uint8_t comp_expression( const Expression* expr );
};