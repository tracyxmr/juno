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
    std::println("parsing primary with value = {}", m_current.value);

    switch ( m_current.token_type )
    {
        case token::NUMBER:
        {
            auto value { std::stod( m_current.value ) };
            auto expr { std::make_unique<Number>( value ) };
            eat(  );

            return expr;
        }

        case token::IDENTIFIER:
        {
            auto value { m_current.value };
            auto expr { std::make_unique< IdentifierLit >( value ) };
            eat(  );

            return expr;
        }

        case token::PRINT:
        {
            // std::println("PARSED PRINT");

            auto value { m_current.value };
            eat(  );

            if ( m_current.token_type == token::LPAREN )
            {
                eat(  );
                // std::println("about to parse args");
                auto args { parse_args( ) };
                // std::println("parsed args");
                auto expr { std::make_unique<CallExpression>( value, std::move( args ) ) };

                // std::println("Parsed print and created call expression");

                eat(  );

                return expr;
            }

            auto expr { std::make_unique< IdentifierLit >( value ) };
            return expr;
        }

        default: throw std::runtime_error( "unhandled expression type" );
    }
}

std::vector< std::unique_ptr< Expression > > parser::Parser::parse_args( )
{
    std::vector< std::unique_ptr< Expression > > args { };

    while ( m_current.token_type != token::RPAREN )
    {
        std::unique_ptr expr { parse_expr(  ) };
        args.push_back( std::move( expr ) );

        if ( m_current.token_type != token::RPAREN )
        {
            expect( token::COMMA, "Expected ',' after argument." );
        }
    }

    return args;
}

std::unique_ptr< Statement > parser::Parser::parse_stmt( )
{
    if (m_current.token_type == token::LET)
    {
        eat(  );
        auto name { expect( token::IDENTIFIER, "Expected a variable name after 'let'." ) };
        expect( token::EQUALS, "Expected '=' after variable name." );
        auto value { parse_expr(  ) };
        expect( token::SEMI, "Expected ';' after variable declaration statement." );
        return std::make_unique< VariableDeclaration >( std::move( name ) , std::move( value ) );
    }

    if ( m_current.token_type == token::LBRACE )
    {
        eat(  );
        auto block { parse_body(  ) };
        expect( token::RBRACE, "Expected '}' to close code block." );
        return block;
    }

    std::println("AT BEFORE ERR = {}", m_current.value);

    auto expr_stmt { std::make_unique< ExpressionStatement >( parse_expr( ) ) };
    expect( token::SEMI, "Expected ';' after expression statement." );
    return expr_stmt;
}

std::unique_ptr< Statement > parser::Parser::parse_body( )
{
    std::vector< std::unique_ptr< Statement > > stmts { };

    while ( m_current.token_type != token::RBRACE )
    {
        std::unique_ptr stmt { parse_stmt(  ) };
        stmts.push_back( std::move( stmt ) );
    }

    return std::make_unique< BlockStmt >( std::move( stmts ) );
}

void parser::Parser::eat( )
{
    if ( m_position < m_tokens.size(  ) - 1 )
        m_current = m_tokens[ ++m_position ];
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