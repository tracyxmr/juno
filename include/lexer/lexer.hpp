//
// Created by margo on 15/11/2025.
//

#pragma once

#include <lexer/token.hpp>
#include <vector>
#include <unordered_map>

namespace lexer
{
    const std::unordered_map<
        char,
        token::TokenType
    > token_character_map
    {
        { '(', token::TokenType::LPAREN, },
        { ')', token::TokenType::RPAREN },
        { '{', token::TokenType::LBRACE },
        { '}', token::TokenType::RBRACE },
        { '*', token::TokenType::ASTERISK },
        { '+', token::TokenType::PLUS },
        { '-', token::TokenType::MINUS },
        { '/', token::TokenType::SLASH },
        { '=', token::TokenType::EQUALS },
        { ',', token::TokenType::COMMA },
        { ';', token::TokenType::SEMI },
        { ':', token::TokenType::COLON },
    };

    const std::unordered_map<
        std::string_view,
        token::TokenType
    > token_keywords_map
    {
        { "let", token::TokenType::LET },
        { "print", token::TokenType::PRINT },
        { "with", token::TokenType::WITH },
        { "fn", token::TokenType::FN },
        { "return", token::TokenType::RETURN },
        { "@profile", token::TokenType::SPECIAL },
        { "@comptime", token::TokenType::SPECIAL },
    };

    class Lexer
    {
    public:
        explicit Lexer( const std::string_view source )
            : source { source } { }

        ///@brief Iterate over the source code and categorise words and characters into tokens.
        [[nodiscard]]
        std::vector< token::Token > tokenize( );
    private:
        std::string source;
        std::size_t line { 1 };
        std::size_t col { 1 };
        std::size_t pos { 0 };

        ///@brief Advance the position to the next character safely, updating line and column position.
        void advance( );

        ///@brief Look at the character ahead one position of the source code
        char *peek_ahead( );
    };
}