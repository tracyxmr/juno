#include <print>
#include <compiler/compiler.hpp>

const std::unordered_map< std::string_view, jnvm::inst::VMNativeID > Compiler::m_natives_map = {
    { "print", jnvm::inst::VMNativeID::PRINT }
};

Compiler::CompilerResult Compiler::compile( )
{
    reset(  );
    /* Create a global scope */
    enter_scope(  );

    /* Temporary: reserve some space for a jump instruction at position 0 */
    const auto jmp_addr { current_addr(  ) };
    emit( jnvm::inst::Instruction( jnvm::inst::Opcode::JMP, jmp_addr ) );
    /* Run the passes */
    pass_collect_prototypes(  );
    /* Path the jump to skip "global" */
    const auto start { current_addr(  ) };
    m_bytecode[ jmp_addr ] = jnvm::inst::Instruction( jnvm::inst::Opcode::JMP, start).data(  );
    compile_global_stmts(  );

    /* ALWAYS emit hlt :3 */
    emit( jnvm::inst::Instruction(jnvm::inst::Opcode::HLT) );
    return CompilerResult {
        .bytecode = std::move( m_bytecode ),
        .string_pool = std::move( m_string_pool ),
        .functions = std::move( m_functions )
    };
}

void Compiler::reset( )
{
    /* Simply just reset everything to an original state */
    m_bytecode.clear(  );
    m_string_pool.clear(  );
    m_functions.clear(  );
    m_scopes.clear(  );
    m_next_register = 0;
}

void Compiler::pass_collect_prototypes( )
{
    for ( const auto& stmt : m_ast )
    {
        if ( const auto* proto { dynamic_cast< const FunctionPrototype* >( stmt.get(  ) ) } )
        {
            m_functions[ std::string( proto->get_name(  ) ) ] = current_addr(  );
            comp_proto_stmt( *proto );
        }
    }
}

void Compiler::compile_global_stmts( )
{
    for ( const auto& stmt : m_ast )
    {
        /* don't recompile protypes, skip them */
        if ( dynamic_cast< const FunctionPrototype* >( stmt.get(  ) ) ) continue;

        comp_statement( *stmt );
    }
}

void Compiler::emit( const jnvm::inst::Instruction &instruction )
{
    m_bytecode.push_back( instruction.data(  ) );
}

void Compiler::enter_scope( )
{
    m_scopes.emplace_back( m_next_register );
}

void Compiler::exit_scope( )
{
    if ( m_scopes.empty(  ) ) throw RuntimeError( "No scopes to exit." );

    const auto scope_start_reg { m_scopes.back(  ).get_start_register(  ) };
    m_scopes.pop_back(  );

    m_next_register = scope_start_reg;
}

std::optional< std::uint8_t > Compiler::find_variable( std::string_view name ) const
{
    for ( auto iter { m_scopes.rbegin(  ) }; iter != m_scopes.rend(); ++iter )
    {
        if ( const auto reg { iter->find( name ) } ) return reg;
    }

    return std::nullopt;
}

std::uint8_t Compiler::alloc_register( )
{
    if ( m_next_register >= 255 ) throw RuntimeError( "Register exhaustion" );
    return m_next_register++;
}

std::uint8_t Compiler::add_to_pool( const std::string &str )
{
    /* Simply return any existing strings in the pool, simple. */
    for ( auto idx { 0 }; idx < m_string_pool.size(  ); idx++ )
    {
        if ( m_string_pool[ idx ] == str ) return idx;
    }

    if ( m_string_pool.size(  ) >= 255 ) throw RuntimeError( "String pool exhaustion" );

    m_string_pool.push_back( str );
    return m_string_pool.size(  ) - 1;
}

void Compiler::comp_statement( const Statement &stmt )
{
    if ( const auto* e { dynamic_cast< const ExpressionStatement* >( &stmt ) } )
    {
        comp_expr_stmt( *e );
    }
    else if ( const auto* v { dynamic_cast< const VariableDeclaration* >( &stmt ) } )
    {
        comp_var_decl_stmt( *v );
    }
    else if ( const auto* c { dynamic_cast< const CompoundAssignment* >( &stmt ) } )
    {
        comp_compound_assign( *c );
    }
    else if ( const auto* a { dynamic_cast< const Assignment* >( &stmt ) } )
    {
        comp_assignment( *a );
    }
    else if ( const auto* b { dynamic_cast< const BlockStmt* >( &stmt ) } )
    {
        comp_block_stmt( *b );
    }
    else if ( const auto* i { dynamic_cast< const IfStatement* >( &stmt ) } )
    {
        comp_if_stmt( *i );
    }
    else if ( const auto* r { dynamic_cast< const ReturnStatement* >( &stmt ) } )
    {
        comp_ret_stmt( *r );
    }
    else if ( dynamic_cast< const FunctionPrototype* >( &stmt ) )
    {
    }
    else
    {
        throw RuntimeError("Unknown statement type");
    }
}

void Compiler::comp_expr_stmt( const ExpressionStatement &stmt )
{
    comp_expression( stmt.get_expression(  ) );
}

void Compiler::comp_var_decl_stmt( const VariableDeclaration &var_decl )
{
    if ( m_scopes.empty(  ) ) throw RuntimeError( "Somehow, variable declaration is outside scope" );

    std::uint8_t var_register;
    if ( var_decl.is_comptime(  ) )
    {
        if ( const auto reg { try_comptime( var_decl.get_value(  ).get(  ) ) } )
        {
            var_register = *reg;
        }
        else
        {
            var_register = comp_expression( var_decl.get_value(  ).get(  ) );
        }
    } else
    {
        var_register = comp_expression( var_decl.get_value(  ).get(  ) );
    }

    /* Bind the variable to the current scope */
    m_scopes.back(  ).declare( var_decl.get_name(  ), var_register );
}

void Compiler::comp_assignment( const Assignment &ass )
{
    const auto var_reg { find_variable( ass.get_name(  ) ) };

    /*
     *  Clean up only temporary values e.g x = 2;
     *  And NOT identifier / variables e.g x = y;
     */
    const auto saved_reg { m_next_register };
    const auto value_reg { comp_expression( ass.get_value(  ).get(  ) ) };
    if ( value_reg != *var_reg )
        emit( jnvm::inst::Instruction(jnvm::inst::Opcode::COPY, *var_reg, value_reg) );

    /* If new registers were allocated now we clean up */
    /* This will restore the register if we found a constant like 42 or a complex expr */
    if ( value_reg >= saved_reg ) m_next_register = saved_reg;
}

void Compiler::comp_compound_assign( const CompoundAssignment &cass )
{
    const auto var_reg { find_variable( cass.get_name(  ) ) };

    /*
     *  Clean up only temporary values e.g x = 2;
     *  And NOT identifier / variables e.g x = y;
     */
    const auto saved_reg { m_next_register };
    const auto value_reg { comp_expression( cass.get_value(  ).get(  ) ) };

    /* TODO: Implement other compound operators */
    emit( jnvm::inst::Instruction(jnvm::inst::Opcode::ADD, *var_reg, value_reg, *var_reg) );

    /* If new registers were allocated now we clean up */
    /* This will restore the register if we found a constant like 42 or a complex expr */
    if ( value_reg >= saved_reg ) m_next_register = saved_reg;
}

void Compiler::comp_block_stmt( const BlockStmt &block )
{
    /* If the block is prefixed with @profile, emit it */
    if ( block.is_profiled(  ) ) emit( jnvm::inst::Instruction(jnvm::inst::Opcode::PRF) );

    enter_scope(  );
    for ( const auto& stmt : block.get_body(  ) )
        comp_statement( *stmt );
    exit_scope(  );

    if ( block.is_profiled(  ) ) emit( jnvm::inst::Instruction(jnvm::inst::Opcode::PRFE) );
}

void Compiler::comp_ret_stmt( const ReturnStatement &ret )
{
    if ( ret.has_value(  ) )
    {
        if ( const auto result_register { comp_expression( ret.get_value( ).get( ) ) }; result_register != 0 )
        {
            emit( jnvm::inst::Instruction( jnvm::inst::Opcode::COPY, 0, result_register ) );
        }
    }

    emit( jnvm::inst::Instruction( jnvm::inst::Opcode::RET ) );
}

void Compiler::comp_proto_stmt( const FunctionPrototype &proto )
{
    /* We need to save the next register state for compiling protos */
    const auto saved { save_register(  ) };
    restore_register( 0 );

    enter_scope(  );

    /*
     * Refactor message:
     *
     * I totally forgot to this in the older version,
     * i forgot to declare the params in the scope of
     * the prototypes body, but here i have done it so
     * using params in bodies work!
     *
     */
    for ( const auto& param : proto.get_params(  ) )
    {
        const auto param_register { alloc_register(  ) };
        m_scopes.back(  ).declare( param.name, param_register );
    }

    comp_statement( *proto.get_body(  ) );

    /* protos must always return! */
    if ( m_bytecode.empty(  ) || jnvm::inst::Instruction( m_bytecode.back(  ) ).opcode(  ) != jnvm::inst::Opcode::RET )
        emit( jnvm::inst::Instruction( jnvm::inst::Opcode::RET ) );

    exit_scope(  );
    restore_register( saved );
}

void Compiler::comp_if_stmt( const IfStatement &ifs )
{
    const auto condition_reg { comp_expression( ifs.get_condition(  ).get(  ) ) };
    /* emit jz if condition is false (zero) */
    const auto jz_addr { current_addr(  ) };
    emit( jnvm::inst::Instruction( jnvm::inst::Opcode::JZ, condition_reg, 0 ) );

    /* first body compilation (then) */
    comp_statement( *ifs.get_body(  ) );

    /* unconditional jump if the IfStatement has an else or else if block */
    std::optional< std::size_t > jmp_addr;
    if ( ifs.has_else(  ) || ifs.has_else_if(  ) )
    {
        jmp_addr = current_addr(  );
        emit( jnvm::inst::Instruction( jnvm::inst::Opcode::JMP, 0 ) );
    }

    /* patch the previous jz address after the then block */
    const auto else_start_addr { current_addr(  ) };
    m_bytecode[ jz_addr ] = jnvm::inst::Instruction( jnvm::inst::Opcode::JZ, condition_reg, static_cast< std::uint8_t >( else_start_addr & 0xFF ) ).data(  );

    /* else ifs */
    if ( ifs.has_else_if(  ) ) comp_if_stmt( *ifs.get_else_if(  ).value(  ) );
    else if ( ifs.has_else(  ) ) comp_statement( *ifs.get_else_body(  ).value(  ) );

    /* patch the unconditonal jump made after compiling then body */
    if ( jmp_addr.has_value(  ) )
    {
        const auto end_addr { current_addr(  ) };
        m_bytecode[ *jmp_addr ] = jnvm::inst::Instruction( jnvm::inst::Opcode::JMP, static_cast< std::uint8_t >( end_addr & 0xFF ) ).data(  );
    }
}

std::uint8_t Compiler::comp_expression( const Expression *expr )
{
    if ( !expr ) throw RuntimeError( "Null expression" );

    if ( const auto* n { dynamic_cast< const Number* >( expr ) } ) return comp_number( *n );
    if ( const auto* s { dynamic_cast< const String* >( expr ) } ) return comp_string( *s );
    if ( const auto* i { dynamic_cast< const IdentifierLit* >( expr ) } ) return comp_identifier( *i );
    if ( const auto* b { dynamic_cast< const BinaryExpression* >( expr ) } ) return comp_binary_expr( *b );
    if ( const auto* c { dynamic_cast< const CallExpression* >( expr ) } ) return comp_call( *c );

    throw RuntimeError( "Unknown expression type" );
}

std::uint8_t Compiler::comp_number( const Number &num )
{
    const auto n_register { alloc_register(  ) };
    emit( jnvm::inst::Instruction(
        jnvm::inst::Opcode::MOV,
        n_register,
        static_cast< std::uint8_t >( num.get_value(  ) )
    ));
    return n_register;
}

std::uint8_t Compiler::comp_string( const String &str )
{
    const auto n_register { alloc_register(  ) };
    const auto str_idx { add_to_pool( str.get_value(  ) ) };
    emit( jnvm::inst::Instruction(
        jnvm::inst::Opcode::LOADS,
        n_register,
        str_idx
    ));
    return n_register;
}

std::uint8_t Compiler::comp_identifier( const IdentifierLit &id ) const
{
    /* Identifier = referring to a variable */
    if ( const auto n_reg { find_variable( id.get_value(  ) ) } )
        return *n_reg;

    throw RuntimeError( std::format( "Undefined variable '{}'", id.get_value(  ) ) );
}

std::uint8_t Compiler::comp_binary_expr( const BinaryExpression &bin )
{
    const auto lhs { comp_expression( bin.get_lhs(  ).get(  ) ) };
    const auto rhs { comp_expression( bin.get_rhs(  ).get(  ) ) };
    const auto res_reg { alloc_register(  ) };
    const auto opcode { binop_to_opcode( bin.get_op(  ).op ) };
    emit( jnvm::inst::Instruction( opcode, lhs, rhs, res_reg ) );
    return res_reg;
}

std::uint8_t Compiler::comp_call( const CallExpression &call )
{
    const auto& args { call.get_args(  ) };
    const auto& callee { call.get_callee(  ) };
    const auto arg_count { args.size(  ) };

    /*
     *
     * We need to define the first register because we have a base
     * register which the function needs to resolve the arguments.
     *
     */
    std::uint8_t first_reg;
    if ( arg_count == 0 ) first_reg = alloc_register(  );
    else
    {
        first_reg = comp_expression( args[ 0 ].get(  ) );
        for ( auto idx { 1 }; idx < args.size(  ); ++idx )
        {
            const auto reg { comp_expression( args[ idx ].get(  ) ) };

            if ( const auto need_consecutive_reg { first_reg + idx }; reg != need_consecutive_reg )
            {
                emit( jnvm::inst::Instruction( jnvm::inst::Opcode::COPY, need_consecutive_reg, reg ));
                alloc_register(  );
            }
        }
    }

    std::uint8_t fn_addr;
    if ( m_functions.contains( callee ) )
    {
        /* user defined are masked to lower bytes! */
        fn_addr = static_cast< std::uint8_t >( m_functions[ callee ] & 0xFF );
    }
    else if ( m_natives_map.contains( callee ) )
    {
        fn_addr = static_cast< std::uint8_t >( m_natives_map.at( callee ) );
    }
    else
    {
        throw RuntimeError( std::format( "Unknown function '{}'", callee ) );
    }

    emit( jnvm::inst::Instruction(
        jnvm::inst::Opcode::CALL,
        fn_addr,
        first_reg,
        arg_count
    ) );

    return first_reg;
}

std::optional< std::uint8_t > Compiler::try_comptime( const Expression *expr )
{
    /* attempt the comptime evaluation for binary expressions */
    if ( const auto* bin { dynamic_cast< const BinaryExpression* >( expr ) })
    {
        try
        {
            m_eval_visitor.visit( *bin );
            const Number result { m_eval_visitor.get_result(  ) };
            return comp_number( result );
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

jnvm::inst::Opcode Compiler::binop_to_opcode( const BinaryOp::Type op )
{
    switch (op)
    {
        case BinaryOp::ADD: return jnvm::inst::Opcode::ADD;
        case BinaryOp::SUB: return jnvm::inst::Opcode::SUB;
        case BinaryOp::MUL: return jnvm::inst::Opcode::MUL;
        case BinaryOp::DIV: return jnvm::inst::Opcode::DIV;
        case BinaryOp::NEQ: return jnvm::inst::Opcode::NEQ;
        case BinaryOp::EQ:  return jnvm::inst::Opcode::EQ;
        case BinaryOp::LT:  return jnvm::inst::Opcode::LT;
        case BinaryOp::GT:  return jnvm::inst::Opcode::GT;
        case BinaryOp::LTE: return jnvm::inst::Opcode::LTE;
        case BinaryOp::GTE: return jnvm::inst::Opcode::GTE;
        default: throw RuntimeError( "Unknown binary operator" );
    }
}
