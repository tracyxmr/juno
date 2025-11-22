//
// Created by margo on 15/11/2025.
//

#pragma once

#include <string>
#include <utility>

namespace token
{
    ///@brief Definition of token types which are part of the language.
    enum TokenType
    {
        IDENTIFIER,
        NUMBER,
        LPAREN,
        RPAREN,
        LBRACE,
        RBRACE,
        PLUS,
        MINUS,
        ASTERISK,
        SLASH,
        EQUALS,
        COMMA,
        PRINT,
        SEMI,
        LET,
        END_OF_FILE
    };

    ///@brief A simple structure which represents a token.
    struct Token
    {
        TokenType token_type { END_OF_FILE };
        std::string value;
        std::size_t line { 0 };
        std::size_t col { 0 };

        Token(
            const TokenType type,
            const std::string_view value,
            const std::size_t line,
            const std::size_t col
        ) :
            token_type { type },
            value { value },
            line { line },
            col { col }
        {}

        Token(
            const TokenType type,
            std::string value,
            const std::size_t line,
            const std::size_t col
        ) :
            token_type { type },
            value { std::move( value ) },
            line { line },
            col { col }
        {
        }

        Token( ) = default;
    };
}