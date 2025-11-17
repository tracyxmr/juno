#pragma once
#include "ast.hpp"
#include "lexer/token.hpp"

#include <memory>
#include <vector>

namespace parser
{
    class Parser
    {
    public:
        explicit Parser( const std::vector< token::Token >& tokens )
            : m_tokens { tokens }
        { }

        ///@brief Parse the vector of tokens into a vector of statements.
        std::vector< std::unique_ptr< Statement > > parse();
    private:
        std::vector< token::Token > m_tokens { };
        token::Token m_current { };

        ///@brief Parse a statement.
        std::unique_ptr< Statement > parse_stmt();
        ///@brief Parse an expression.
        std::unique_ptr< Expression > parse_expr();

        std::size_t m_position { 0 };

        ///@brief Advance onto the next token in the tokens vector.
        void eat( );
        ///@brief Expect the current token type to be an expected
        ///type, if not an error will be thrown.
        void expect( token::TokenType type, std::string_view error_message );
    };
}
