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
        { '<', token::TokenType::LT },
        { '>', token::TokenType::GT },
    };

    const std::unordered_map<
        std::string_view,
        token::TokenType
    > token_keywords_map
    {
        { "let", token::TokenType::LET },
        { "with", token::TokenType::WITH },
        { "fn", token::TokenType::FN },
        { "if", token::TokenType::IF },
        { "else", token::TokenType::ELSE },
        { "extern", token::TokenType::EXTERN },
        { "return", token::TokenType::RETURN },
        { "true", token::TokenType::TRUE },
        { "false", token::TokenType::FALSE },
        { "@profile", token::TokenType::SPECIAL },
        { "@comptime", token::TokenType::SPECIAL },
    };

    const std::unordered_map<
        std::string_view,
        token::TokenType
    > token_compound_map
    {
        { "+=", token::TokenType::ADD_EQ },
        { "-=", token::TokenType::SUB_EQ },
        { "*=", token::TokenType::MUL_EQ },
        { "/=", token::TokenType::DIV_EQ },
        { "->", token::TokenType::ARROW },
        { "<=", token::TokenType::LTE },
        { ">=", token::TokenType::GTE },
        { "==", token::TokenType::EQ },
        { "!=", token::TokenType::NEQ },
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

        ///@brief Will try to tokenize a compound token.
        std::optional< token::Token > try_compound( char first, char second );
    };
}