#include <print>
#include <compiler/compiler.hpp>

std::vector< std::uint32_t > Compiler::compile( )
{
    /// Reset any state in the compiler.
    bytecode.clear(  );
    nx_register = 0;

    /// Enter the global scope
    enter_scope(  );

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

void Compiler::enter_scope( )
{
    const auto scope { Scope { nx_register } };
    scopes.emplace_back( scope );
}

void Compiler::exit_scope( )
{
    const Scope scope { scopes.back(  ) };
    scopes.pop_back(  );

    nx_register = scope.start_reg;
}

void Compiler::comp_statement( const Statement &stmt )
{
    /// Downcast the Statement into an ExpressionStatement

    if ( const auto *expr_stmt { dynamic_cast< const ExpressionStatement * >( &stmt ) } )
    {
        /// Get the Expression of the ExpressionStatement.
        const Expression* expr { expr_stmt->get_expression(  ) };
        /// Compile the expression.
        comp_expression( expr );
    } else if ( const auto *var_stmt { dynamic_cast< const VariableDeclaration * >( &stmt) })
    {
        if (scopes.empty(  )) throw std::runtime_error("[juno::compile_error] Somehow, there are no scopes left.");

        const auto var_reg { comp_expression( var_stmt->get_value( ).get( ) ) };
        scopes.back(  ).variables[ var_stmt->get_name(  ) ] = var_reg;
    } else if ( const auto *block_stmt { dynamic_cast< const BlockStmt * >( &stmt ) } )
    {
        enter_scope(  );
        for ( const auto& s : block_stmt->get_body( ) )
            comp_statement( *s );
        exit_scope(  );
    } else
    {
        throw std::runtime_error("[juno::compile_error] unknown statement type.");
    }
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

        std::println("Number reg = {}", reg);

        return reg;
    }

    /// If the expression is an IndentifierLiteral, assume it's referring to a variable
    /// so search through all scopes until it's found.
    if ( const auto* id { dynamic_cast< const IdentifierLit* >( expr )})
    {
        for ( auto& s : scopes )
            if ( s.variables.contains( id->get_value(  ) ) )
                return s.variables[ id->get_value(  ) ];
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

    /// If the expression is a CallExpression, emit it's structure to the bytecode.
    /// For now, we'll directly call natives
    if ( const auto* call { dynamic_cast< const CallExpression* >( expr ) })
    {
        const auto& args { call->get_args(  ) };
        const auto& callee { call->get_callee(  ) };

        const auto first_reg { comp_expression( args[ 0 ].get( ) ) };
        const auto arg_count { args.size(  ) };

        /// 22/11/2025 - I subtracted 1 from arg_count because I wanted to exclude the first arg
        ///              now it has been removed and now the args are compiled.
        for ( int idx { 1 }; idx < arg_count; idx++ )
            comp_expression( args[ idx ].get( ) );

        emit(jnvm::inst::Instruction(
            jnvm::inst::Opcode::CALL,
            static_cast< std::uint8_t >( native_map[ callee ] ),
            first_reg,
            arg_count
        ));

        return first_reg;
    }

    throw std::runtime_error( "[juno::compile_error] unknown expression type." );
}
