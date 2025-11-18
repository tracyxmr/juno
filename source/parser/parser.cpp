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
        case token::NUMBER:
        {
            auto value { std::stod( m_current.value ) };
            auto expr { std::make_unique<Number>( value ) };
            eat(  );

            return expr;
        }

        default: throw std::runtime_error( "unhandled expression type" );
    }
}

std::unique_ptr< Statement > parser::Parser::parse_stmt( )
{
    return std::make_unique< ExpressionStatement >( parse_expr(  ) );
}

void parser::Parser::eat( )
{
    if ( m_position < m_tokens.size(  ) - 1 )
        m_current = m_tokens[ ++m_position ];
}

void parser::Parser::expect( const token::TokenType type, const std::string_view error_message )
{
    if ( m_current.token_type != type )
        throw std::runtime_error(
            std::format( "[juno::parse_error] {}",
                std::string { error_message }
            ));

    eat(  );
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