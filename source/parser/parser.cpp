#include <format>
#include <print>
#include <stdexcept>
#include <parser/parser.hpp>

std::vector< std::unique_ptr< Statement > > parser::Parser::parse( )
{
    std::vector< std::unique_ptr< Statement > > ast { };

    /// Retrieve the first token
    if ( !m_tokens.empty(  ) )
        m_current = m_tokens[ 0 ];

    while ( m_current.token_type != token::TokenType::END_OF_FILE )
    {
        if ( auto stmt = parse_stmt( ) )
            ast.push_back( std::move( stmt ) );
    }

    return ast;
}

std::unique_ptr< Expression > parser::Parser::parse_expr( )
{
    return parse_precedence( 0 );
}

std::unique_ptr< Expression > parser::Parser::parse_precedence( const int min_precedence )
{
    auto left { parse_prim(  ) };

    while ( true )
    {
        auto op { binary_op(  ) };
        if ( !op.has_value(  ) ) break;

        const auto precedence { op->precedence(  ) };

        if ( precedence < min_precedence ) break;

        eat(  );

        auto rhs { parse_precedence( precedence + 1 ) };

        left = std::make_unique<BinaryExpression>(
            std::move(left),
            std::move(rhs),
            op.value()
        );
    }

    return left;
}

std::unique_ptr< Expression > parser::Parser::parse_prim( )
{
    switch ( m_current.token_type )
    {
        case token::NUMBER: return parse_number();
        case token::PRINT:
        case token::IDENTIFIER:
            return parse_identifier();
        case token::LPAREN: return parse_group(  );
        case token::FN:
        {
            auto lambda { parse_lambda() };
            auto* proto_r { dynamic_cast< FunctionPrototype * >( lambda.get(  ) ) };
            if ( !proto_r ) throw std::runtime_error( "[juno::parse_error] Failed to parse lambda expression." );

            lambda.release( );

            return std::make_unique< FunctionExpression >( std::unique_ptr< FunctionPrototype >( proto_r ) );
        }
        default: throw std::runtime_error(
                std::format( "[juno::parse_error] Unexpected token '{}' in expression",
                    m_current.value )
            );
    }
}

std::unique_ptr< ReturnStatement > parser::Parser::parse_return( )
{
    eat(  );
    auto value { parse_expr(  ) };
    expect( token::SEMI, "Expected ';' after return value." );
    return std::make_unique< ReturnStatement >( std::move( value ) );
}

std::unique_ptr< Expression > parser::Parser::parse_number( )
{
    const double value { std::stod( m_current.value ) };
    eat(  );

    return std::make_unique< Number >( value );
}

std::unique_ptr< Expression > parser::Parser::parse_identifier( )
{
    const std::string value { m_current.value };
    eat(  );

    /// Check if the current token is a left parenthesis
    /// If it is, then it's a function call.
    if ( match( token::LPAREN ) )
    {
        auto args { parse_args(  ) };
        expect( token::RPAREN, "Expected ')' after function arguments." );

        return std::make_unique< CallExpression >( value, std::move( args ) );
    }

    return std::make_unique< IdentifierLit >( value );
}

std::unique_ptr< Expression > parser::Parser::parse_group( )
{
    eat(  );
    auto expr { parse_expr(  ) };
    expect( token::RPAREN, "Expected ')' after grouped expression." );

    return expr;
}

std::vector< std::unique_ptr< Expression > > parser::Parser::parse_args( )
{
    std::vector< std::unique_ptr< Expression > > args { };

    /// I did not handle empty args before, but now I have.
    if ( check( token::RPAREN ) ) return args;

    /// Parse the first argument, then the remaining.
    args.push_back( parse_expr(  ) );
    while ( match( token::COMMA ) ) args.push_back( parse_expr(  ) );

    return args;
}

std::unique_ptr< Statement > parser::Parser::parse_stmt( )
{
    switch ( m_current.token_type )
    {
        case token::SPECIAL:
        case token::LET: return parse_var_decl(  );
        case token::LBRACE:
        case token::FN: return parse_prototype(  );
        case token::RETURN: return parse_return(  );
        default: return parse_expr_stmt(  );
    }
}

std::unique_ptr< Statement > parser::Parser::parse_prototype( )
{
    expect( token::FN, "Expected 'fn' keyword." );
    auto name { expect( token::IDENTIFIER, "Expected function name after 'fn'." ) };
    expect( token::LPAREN, "Expected '(' after 'fn'"  );
    auto params { parse_params(  ) };
    expect( token::RPAREN, "Expected ')' after parameters." );
    expect( token::ARROW, "Expected '->' after enclosed parameters." );
    auto ret_type_name { expect( token::IDENTIFIER, "Expected return type after '->' in function prototype." ) };
    auto ret_type { std::make_unique< Type >( TypeKind::Simple, ret_type_name, std::nullopt ) };

    /// Parse the function prototype body.
    auto body { parse_block(  ) };
    auto block_stmt { std::unique_ptr< BlockStmt >( dynamic_cast< BlockStmt * >( body.release(  ) ) ) };
    /// Ensure the block exists
    if ( !block_stmt ) throw std::runtime_error( "[juno::parse_error] Expected block statement for function body." );

    return std::make_unique< FunctionPrototype >(
        name,
        std::move( params ),
        std::move( ret_type ),
        std::move( block_stmt )
    );
}

std::unique_ptr< Statement > parser::Parser::parse_lambda( )
{
    expect( token::FN, "Expected 'fn' keyword." );
    expect( token::LPAREN, "Expected '(' after 'fn'"  );
    auto params { parse_params(  ) };
    expect( token::RPAREN, "Expected ')' after parameters." );
    expect( token::ARROW, "Expected '->' after enclosed parameters." );
    auto ret_type_name { expect( token::IDENTIFIER, "Expected return type after '->' in function prototype." ) };
    auto ret_type { std::make_unique< Type >( TypeKind::Simple, ret_type_name, std::nullopt ) };

    /// Parse the function prototype body.
    auto body { parse_block(  ) };
    auto block_stmt { std::unique_ptr< BlockStmt >( dynamic_cast< BlockStmt * >( body.release(  ) ) ) };
    /// Ensure the block exists
    if ( !block_stmt ) throw std::runtime_error( "[juno::parse_error] Expected block statement for function body." );

    return std::make_unique< FunctionPrototype >(
        std::move( params ),
        std::move( ret_type ),
        std::move( block_stmt )
    );
}

std::vector< Parameter > parser::Parser::parse_params( )
{
    std::vector< Parameter > params;

    if ( check( token::RPAREN ) ) return params;

    /// Parse the first parameter right here
    auto param_name { expect( token::IDENTIFIER, "Expected identifier name." ) };
    expect( token::COLON, std::format( "Expected ':' after parameter name '{}'.", param_name ) );
    auto param_type_name { expect( token::IDENTIFIER, "Expected parameter type" ) };

    auto param_type { std::make_unique< Type >( TypeKind::Simple, param_type_name, std::nullopt )};
    params.emplace_back( param_name, std::move( param_type ) );

    /// Parse the remaining parameters
    while ( match(token::COMMA) )
    {
        auto n_param_name { expect( token::IDENTIFIER, "Expected identifier name." ) };
        expect( token::COLON, std::format( "Expected ':' after parameter name '{}'.", n_param_name ) );
        auto n_param_type_name { expect( token::IDENTIFIER, "Expected parameter type" ) };

        auto n_param_type { std::make_unique< Type >( TypeKind::Simple, n_param_type_name, std::nullopt )};
        params.emplace_back( n_param_name, std::move( n_param_type ) );
    }

    return params;
}

std::unique_ptr< Statement > parser::Parser::parse_var_decl( )
{
    const auto special_kw { check( token::SPECIAL ) };
    auto comptime { false };

    if ( special_kw && m_current.value == "@comptime" )
    {
        comptime = true;
        eat(  );
    } else if ( m_current.value == "@profile" )
    {
        return parse_block(  );
    }

    expect( token::LET, "Expected 'let' after @comptime" );

    auto name { expect( token::IDENTIFIER, "Expected variable name after 'let'." ) };

    /// Parse the optional type annotation, will be inferred later.
    std::unique_ptr< Type > type { nullptr };
    if ( match( token::COLON ) )
    {
        const auto type_name { expect( token::IDENTIFIER, std::format( "Expected type name after '{}:'", name ) ) };
        type = std::make_unique< Type >( TypeKind::Simple, type_name, std::nullopt );
    }

    expect( token::EQUALS, "Expected '=' in variable declaration." );
    auto value { parse_expr(  ) };
    expect( token::SEMI, "Expected ';' after variable declaration." );

    return std::make_unique< VariableDeclaration >(
        std::move( name ),
        std::move( value ),
        std::move( type ),
        comptime
    );
}

std::unique_ptr< Statement > parser::Parser::parse_expr_stmt( )
{
    auto expr { parse_expr(  ) };
    expect( token::SEMI, "Expected ';' after expression statement." );

    return std::make_unique< ExpressionStatement >( std::move( expr ) );
}

std::unique_ptr< Statement > parser::Parser::parse_block( )
{
    bool is_profiled { false };
    if ( m_current.token_type == token::SPECIAL )
    {
        if ( m_current.value == "@profile" )
        {
            is_profiled = true;
            eat(  );
        } else
        {
            throw std::runtime_error(
                std::format( "[juno::parse_error] Unknown annotation '{}'. Only @profile is supported.",
                    m_current.value )
            );
        }
    }

    expect( token::LBRACE, "Expected '{' to start block." );

    std::vector< std::unique_ptr< Statement > > stmts { };

    while ( !check( token::RBRACE ) && m_current.token_type != token::TokenType::END_OF_FILE )
    {
        std::unique_ptr stmt { parse_stmt(  ) };
        stmts.push_back( std::move( stmt ) );
    }

    expect( token::RBRACE , "Expected '}' to close block." );

    return std::make_unique< BlockStmt >( std::move( stmts ), is_profiled );
}

void parser::Parser::eat( )
{
    if ( m_position < m_tokens.size(  ) - 1 )
        m_current = m_tokens[ ++m_position ];
}

bool parser::Parser::check( const token::TokenType type ) const
{
    return m_current.token_type == type;
}

bool parser::Parser::match( token::TokenType type )
{
    if ( check( type ) )
    {
        eat(  );
        return true;
    }

    return false;
}

std::string parser::Parser::expect( const token::TokenType type, const std::string_view error_message )
{
    if ( m_current.token_type != type )
        throw std::runtime_error(
            std::format( "[juno::parse_error] {}",
                std::string { error_message }
            ));

    auto value { m_current.value };
    eat(  );

    return value;
}

std::optional< BinaryOp > parser::Parser::binary_op( ) const
{
    switch ( m_current.token_type )
    {
        case token::PLUS: return BinaryOp { BinaryOp::ADD };
        case token::MINUS: return BinaryOp { BinaryOp::SUB };
        case token::ASTERISK: return BinaryOp { BinaryOp::MUL };
        case token::SLASH: return BinaryOp { BinaryOp::DIV };
        default: return std::nullopt;
    }
}