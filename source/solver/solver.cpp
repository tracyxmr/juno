#include <format>
#include <solver/solver.hpp>

void Solver::solve( const std::vector< std::unique_ptr< Statement > > &ast )
{
    for ( const auto& s : ast )
    {
        /// Obviously, visit each statement type
        if ( const auto* var { dynamic_cast< const VariableDeclaration* >( s.get(  ) ) } )
        {
            visit( *var );
        }
        else if ( const auto* ex { dynamic_cast< const ExpressionStatement* >( s.get(  ) ) } )
        {
            visit( *ex );
        }
        else if ( const auto* b { dynamic_cast< const BlockStmt* >( s.get(  ) ) } )
        {
            visit( *b );
        }
        else if ( const auto* f { dynamic_cast< const FunctionPrototype* >( s.get(  ) ) } )
        {
            visit( *f );
        }
        else if ( const auto* r { dynamic_cast< const ReturnStatement* >( s.get(  ) ) } )
        {
            visit( *r );
        }
    }
}

void Solver::visit( const BinaryExpression &b )
{
    /// Visit both LHS and RHS
    b.get_lhs(  )->accept( *this );
    /// The type of LHS will be m_last, as it has been previously accepted and visited.
    const TypeExtended lhs_t { m_last };

    b.get_rhs(  )->accept( *this );
    /// The type of RHS will be m_last, as it has been previously accepted and visited.
    const TypeExtended rhs_t { m_last };

    /// Now check the compatability of both operands
    ensure_types_compatibility(
        lhs_t,
        rhs_t,
        std::format(
            "Binary operation '{}' requires compatible operand types, but got '{}' and '{}'",
            b.get_op(  ).to_string(  ),
            lhs_t.to_string(  ),
            rhs_t.to_string(  )
        )
    );

    if ( lhs_t.name == "double" && rhs_t.name == "double" )
    {
        m_last = TypeExtended( TypeKind::Simple, "double" );
    }
    else
    {
        throw std::runtime_error( std::format(  "[juno::solver_error] Binary operation not supported for types '{}' and '{}'", lhs_t.to_string(  ), rhs_t.to_string(  ) ) );
    }
}

void Solver::visit( const Number &n )
{
    m_last = TypeExtended { TypeKind::Simple, "double" };
}

void Solver::visit( const CallExpression &c )
{
    /// Find the function return type.
    const TypeExtended return_type { lookup_return_type( c.get_callee(  ) ) };
    /// TODO: compare argument types against parameter types.
    m_last = return_type;
}

void Solver::visit( const IdentifierLit &i )
{
    /// Since an identifier is refering to a symbol, look it up.
    m_last = lookup_variable_type( i.get_value(  ) );
}

void Solver::visit( const FunctionPrototype &f )
{
    /// Register if it's not a lambda, because lambdas dont have a name... duh.
    if ( !f.is_lambda(  ) )
    {
        TypeExtended return_type { *f.get_return_type(  ) };
        register_func( std::string( f.get_name(  ) ), return_type );
        m_cur_fn_ret_type = return_type;
    }

    /// Register each parameter in synmbol table
    for ( const auto& p : f.get_params(  ) )
    {
        TypeExtended p_type { *p.type };
        register_var( p_type.name, p_type );
    }

    /// Visit the function body block
    if ( f.get_body(  ) )
        visit( *f.get_body(  ) );

    m_cur_fn_ret_type = std::nullopt;

    /// Clean up params in symbol table for scope management, but later need to do this properly.
    for ( const auto& p : f.get_params(  ) )
    {
        m_symbols_tab.erase( p.name );
    }
}

void Solver::visit( const FunctionExpression &f )
{
    if ( f.get_proto(  ) ) visit( *f.get_proto(  ) );
}

void Solver::visit( const VariableDeclaration &v )
{
    /// Infer the type from the init expression
    v.get_value(  )->accept( *this );
    const TypeExtended inferred_t { m_last }; /// Conveniently, it's in m_last

    /// Does the var decl have a type annotation? if it does check compatability.
    /// If it does have an annotation, use it, else use the inferred.
    if ( v.get_type(  ) )
    {
        const TypeExtended annotation_t { *v.get_type(  ) };
        ensure_types_compatibility(
            annotation_t,
            inferred_t,
            std::format( "variable '{}' declaration", v.get_name(  ) )
        );
        register_var( std::string( v.get_name(  ) ), annotation_t );
    } else
    {
        register_var( std::string( v.get_name(  ) ), inferred_t );
    }
}

void Solver::visit( const ReturnStatement &r )
{
    if ( !m_cur_fn_ret_type.has_value(  ) )
        throw std::runtime_error( "[juno::solver_error] Return statement outside of function context" );

    if ( r.has_value(  ) )
    {
        /// SOLVE IT
        r.get_value(  )->accept( *this );
        TypeExtended return_t { m_last };

        ensure_types_compatibility(
            m_cur_fn_ret_type.value(  ),
            return_t,
            "Return statement"
        );
    } else
    {
        /// return nothing, but the function must expects void
        if ( m_cur_fn_ret_type.value(  ).name != "void" )
        {
            throw std::runtime_error(
                std::format(
                    "[juno::solver_error] Function expects return type '{}' but got no return (void)",
                    m_cur_fn_ret_type.value(  ).to_string(  )
                )
            );
        }
    }
}

void Solver::visit( const BlockStmt &b )
{
    /// Solve the body of the block
    solve( b.get_body(  ) );
}

void Solver::visit( const ExpressionStatement &e )
{
    /// This is just a wrapped expression
    if ( e.get_expression(  ) ) e.get_expression(  )->accept( *this );
}

void Solver::ensure_types_compatibility( const TypeExtended &expected, const TypeExtended &got, const std::string &ctx )
{
    if ( expected == got ) return;

    throw std::runtime_error(
        std::format(
            "[juno::solver_error] Type mismatch in {}: expected '{}', but got '{}'",
            ctx,
            expected.to_string(  ),
            got.to_string(  )
        )
    );
}

void Solver::register_var( const std::string &name, const TypeExtended &type )
{
    m_symbols_tab[ name ] = type;
}

void Solver::register_func( const std::string &name, const TypeExtended &ret_type )
{
    m_func_tab[ name ] = ret_type;
}

TypeExtended Solver::lookup_variable_type( const std::string &name )
{
    const auto iter { m_symbols_tab.find( name ) };
    if ( iter == m_symbols_tab.end( ) )
        throw std::runtime_error( std::format( "[juno::solver_error] Undefined variable '{}'", name ) );

    return iter->second;
}

TypeExtended Solver::lookup_return_type( const std::string &name )
{
    /// TODO: Implement a proper way to check built in function return types
    if ( name == "print" ) return TypeExtended { TypeKind::Simple, "void" };

    const auto iter { m_func_tab.find( name ) };
    if ( iter == m_func_tab.end( ) )
        throw std::runtime_error( std::format( "[juno::solver_error] Undefined function '{}'", name ) );

    return iter->second;
}

TypeExtended Solver::infer_type( const Expression *expr )
{
    if ( !expr ) return TypeExtended { };

    expr->accept( *this );
    return m_last;
}
