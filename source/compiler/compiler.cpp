#include <print>
#include <compiler/compiler.hpp>

std::vector< std::uint32_t > Compiler::compile( )
{
    /// Reset any state in the compiler.
    bytecode.clear(  );
    nx_register = 0;

    /// Compile all statements in the AST.
    for ( const auto& s : ast )
        comp_statement( *s );

    emit(jnvm::inst::Instruction(jnvm::inst::Opcode::HLT));
    return bytecode;
}

void Compiler::emit( const jnvm::inst::Instruction &inst )
{
    std::println( "{}", jnvm::inst::opcode_to_string( inst.opcode(  ) ) );
    std::println( "next reg: {}", nx_register );

    bytecode.push_back( inst.data(  ) );

    std::println("bytecode at emission: {}", bytecode);
}

void Compiler::comp_statement( const Statement &stmt )
{
    /// Downcast the Statement into an ExpressionStatement
    const auto* expr_stmt { dynamic_cast< const ExpressionStatement* >( &stmt ) };

    if ( !expr_stmt ) throw std::runtime_error("[juno::compile_error] unknown statement type.");

    /// Get the Expression of the ExpressionStatement.
    const Expression* expr { expr_stmt->get_expression(  ) };
    /// Compile the expression.
    comp_expression( expr );
}

std::uint8_t Compiler::comp_expression( const Expression *expr )
{
    /// If the expression is a Number, emit it's value to the bytecode.
    /// And allocate it to a register.
    if ( const auto* n { dynamic_cast< const Number* >( expr ) } )
    {
        std::println("COMPILING NUMBER");
        const auto reg { nx_register++ };

        emit( jnvm::inst::Instruction(
            jnvm::inst::Opcode::MOV,
            reg,
            static_cast< std::uint8_t >( n->get_value(  ) )
        ) );

        return reg;
    }

    /// If the expression is a BinaryExpression, emit it's structure to the bytecode.
    if ( const auto* bin { dynamic_cast< const BinaryExpression* >( expr ) } )
    {
        /// Compile LHS and RHS of the expression
        const auto lhs_register { comp_expression( bin->get_lhs(  ).get(  ) ) };
        const auto rhs_register { comp_expression( bin->get_rhs(  ).get(  ) ) };
        const auto result_register { nx_register++ };

        std::println("at binary expr");

        jnvm::inst::Opcode operation;
        switch ( bin->get_op(  ).op )
        {
            case BinaryOp::ADD: operation = jnvm::inst::Opcode::ADD; break;
            case BinaryOp::SUB: operation = jnvm::inst::Opcode::SUB; break;
            case BinaryOp::MUL: operation = jnvm::inst::Opcode::MUL; break;
            case BinaryOp::DIV: operation = jnvm::inst::Opcode::DIV; break;
            default: throw std::runtime_error( "[juno::compile_error] unknown binary operator used." );
        }

        /// Emit the instruction.
        emit( jnvm::inst::Instruction(
            operation,
            lhs_register,
            rhs_register,
            result_register
        ) );

        return result_register;
    }

    throw std::runtime_error( "[juno::compile_error] unknown expression type." );
}
