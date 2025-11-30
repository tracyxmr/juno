#pragma once

#include <parser/ast.hpp>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

namespace codegen
{
    class codegen final : public Visitor
    {
    public:
        explicit codegen( const std::string& module_name = "juno_main" )
            : m_module { std::make_unique< llvm::Module >( module_name, m_context ) },
              m_ir_builder { m_context },
              m_last { nullptr },
              m_current_func { nullptr }
        { register_builtins( ); }

        void visit( const Number& n ) override;
        void visit( const String &s ) override;
        void visit( const BinaryExpression &b ) override;
        void visit( const CallExpression &c ) override;
        void visit( const IdentifierLit &i ) override;
        void visit( const FunctionPrototype &fp ) override;
        void visit( const FunctionExpression &fp ) override;

        void generate_stmt( Statement* s );
        void generate_expr_stmt( ExpressionStatement* e );
        void generate_var_decl( VariableDeclaration * v );
        void generate_assign( Assignment * a );
        void generate_comp_assign( CompoundAssignment * c );
        void generate_extern_proto( ExternalFunctionProto * e );
        void generate_block( BlockStmt* b );
        void generate_if_stmt( IfStatement* i );
        void generate_return( ReturnStatement* r );

        ///@brief Generates a LLVM value for an expression.
        llvm::Value* generate_expr( const Expression * expr );

        ///@brief Returns the module
        llvm::Module* get_module( ) const
        { return m_module.get(  ); }

        ///@brief Release the modules ownership from the CodeGen class
        std::unique_ptr< llvm::Module > release_module( )
        { return std::move( m_module ); }

        ///@brief Returns a reference to the context
        const llvm::LLVMContext& get_context( ) const
        { return m_context; }

        ///@brief Returns a reference to the IR builder
        const llvm::IRBuilder<>& get_builder( ) const
        { return m_ir_builder; }

        ///@brief Returns the last generated value
        llvm::Value* get_last( ) const { return m_last; }

        ///@brief Write the generated IR to a file
        void write( const std::string& file_name ) const;
    private:
        llvm::LLVMContext m_context;
        std::unique_ptr< llvm::Module > m_module;
        llvm::IRBuilder< > m_ir_builder;

        ///@brief Symbol table for variable names to LLVM values...
        std::unordered_map< std::string, llvm::Value* > m_symbol_table;
        ///@brief Holds the last generated value from the visitor
        llvm::Value* m_last;
        ///@brief The current function being generated.
        llvm::Function* m_current_func;

        ///@brief Helper function which returns the corresponding LLVM type for our type
        llvm::Type* get_type( const Type* type );
        ///@brief Helper function which either retrieves a function or creates one
        llvm::Function* get_function( const std::string& name ) const;

        ///@brief Register built ins
        void register_builtins( );

        static void error( const std::string& message );
    };
}