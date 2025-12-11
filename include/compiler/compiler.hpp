#pragma once

#include "evaluator/eval_visitor.hpp"
#include "jnvm/instruction.hpp"
#include "parser/ast.hpp"

#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Compiler
{
public:
    explicit Compiler(
        std::vector< std::unique_ptr< Statement > >&& ast
    ) : m_ast { std::move( ast ) } {}

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
            : std::runtime_error { std::format( "[jnvm::compiler_error] {}", msg ) }
        {}
    };

    /* Return a bundle of the bytecode, string pool and function addresses */
    struct CompilerResult
    {
        std::vector< std::uint32_t > bytecode;
        std::vector< std::string > string_pool;
        std::unordered_map< std::string, std::size_t > functions;
    };

    /* Added better variable bindings for scopes */
    class Scope
    {
    public:
        explicit Scope(
            const std::uint8_t _start_reg
        ) noexcept : start_reg { _start_reg } { }

        ///@brief Bind a variable identifier to a register index.
        void declare( std::string_view name, const std::uint8_t register_idx )
        {
            variables[ name ] = register_idx;
        }

        ///@brief Find a variable within the scope.
        ///@return A std::optional< std::uint8_t > which represents the register index of the variables value.
        [[nodiscard]]
        std::optional< std::uint8_t > find( std::string_view name ) const
        {
            const auto iter { variables.find( name ) };
            return iter != variables.end( ) ? std::optional { iter->second } : std::nullopt;
        }

        ///@brief Helper for getting the start register index for cleaning up
        [[nodiscard]]
        std::uint8_t get_start_register() const noexcept { return start_reg; }

    private:
        std::uint8_t start_reg { 0 };
        std::unordered_map< std::string_view, std::uint8_t > variables { };
    };

    ///@brief Compile the loaded AST into a packed bundle of bytecode, strings and functions.
    [[nodiscard]]
    CompilerResult compile( );

private:
    std::vector< std::unique_ptr< Statement > > m_ast;
    std::vector< std::uint32_t > m_bytecode;
    std::vector< std::string > m_string_pool;
    std::uint8_t m_next_register { 0 };
    std::vector< Scope > m_scopes;
    std::unordered_map< std::string, std::size_t > m_functions;
    EvalVisitor m_eval_visitor;

    ///@brief VM native function mapping
    static const std::unordered_map< std::string_view, jnvm::inst::VMNativeID > m_natives_map;

    ///@brief Reset the state of the compiler;
    void reset();
    ///@brief This is a pass in the compiler which collects all prototypes in the AST.
    void pass_collect_prototypes();
    ///@brief Compile all global statements
    void compile_global_stmts();

    ///@brief Helper function which decodes an instruction into it's raw format.
    void emit( const jnvm::inst::Instruction& instruction );

    ///@brief Helper function which returns the current address (size of bytecode when calling)
    [[nodiscard]]
    std::size_t current_addr( ) const noexcept { return m_bytecode.size(  ); }

    ///@brief Enter a scope.
    void enter_scope();
    ///@brief Exit a scope.
    void exit_scope();

    ///@brief Find a variable in the available scopes.
    [[nodiscard]]
    std::optional< std::uint8_t > find_variable( std::string_view name ) const;

    ///@brief Helper function to allocate a register and get it's index.
    [[maybe_unused]]
    std::uint8_t alloc_register( );

    ///@brief Helper fucntion to save the current register state
    [[nodiscard]]
    std::uint8_t save_register( ) const noexcept { return m_next_register; }

    ///@brief Helper function to restore the register state
    void restore_register( const std::uint8_t other ) noexcept { m_next_register = other; }

    ///@brief Helper function which adds a string to the pool.
    ///@return The string's index in the pool.
    [[nodiscard]]
    std::uint8_t add_to_pool( const std::string& str );

    ///@brief Compile a statement.
    void comp_statement( const Statement& stmt );
    ///@brief Compile an expression statement.
    void comp_expr_stmt( const ExpressionStatement& stmt );
    ///@brief Compile a variable declaration statement.
    void comp_var_decl_stmt( const VariableDeclaration& var_decl );
    ///@brief Compile an assignment statement.
    void comp_assignment( const Assignment& ass );
    ///@brief Compile a compound assignment statement.
    void comp_compound_assign( const CompoundAssignment& cass );
    ///@brief Compile a block statement.
    void comp_block_stmt( const BlockStmt& block );
    ///@brief Compile a return statement.
    void comp_ret_stmt( const ReturnStatement& ret );
    ///@brief Compile a prototype statement.
    void comp_proto_stmt( const FunctionPrototype& proto );
    ///@brief Compile an if statement.
    void comp_if_stmt( const IfStatement& ifs );

    ///@brief Compile an expression.
    [[maybe_unused]] std::uint8_t comp_expression( const Expression* expr );
    ///@brief Compile a number.
    [[nodiscard]] std::uint8_t comp_number( const Number& num );
    ///@brief Compile a string.
    [[nodiscard]] std::uint8_t comp_string( const String& str );
    ///@brief Compile an identifier.
    [[nodiscard]] std::uint8_t comp_identifier( const IdentifierLit& id ) const;
    ///@brief Compile a binary expresison.
    [[nodiscard]] std::uint8_t comp_binary_expr( const BinaryExpression& bin );
    ///@brief Compile a call expression.
    [[nodiscard]] std::uint8_t comp_call( const CallExpression& call );

    ///@brief Helper function which attempts to evaluate an expression decorated with @comptime
    [[nodiscard]] std::optional< std::uint8_t > try_comptime( const Expression* expr );
    ///@brief Helper to get opcode from binary operator
    static jnvm::inst::Opcode binop_to_opcode( BinaryOp::Type op );
};