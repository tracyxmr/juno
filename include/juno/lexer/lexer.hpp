#pragma once

#include <juno/lexer/token.hpp>
#include <vector>
#include <unordered_map>
#include <format>

namespace juno
{
    static const std::unordered_map< char, token_type > token_character_map {
        { ';', token_type::SEMI },
        { ':', token_type::COLON },
        { '=', token_type::EQUALS },
        { '+', token_type::ADD },
        { '-', token_type::SUB },
        { '*', token_type::MUL },
        { '/', token_type::DIV }
    };

    static const std::unordered_map< std::string_view, token_type > token_keyword_map {
        { "let", token_type::LET },
        { "mut", token_type::MUT }
    };

    class juno_error final : std::runtime_error
    {
        explicit juno_error( const std::string& message )
            : runtime_error { std::format( "[juno::error] {}", message ) } { }
    };

    class lexer
    {
    public:
        explicit lexer(
            // ReSharper disable once CppParameterMayBeConst
            std::string_view source
        ) : m_source { source } { }

        ///@brief Process the source code into a vector of tokens.
        [[nodiscard]]
        std::vector< token > tokenize( );

    private:
        std::string m_source;
        std::size_t m_line { 1 };
        std::size_t m_column { 1 };
        std::size_t m_pos { 0 };

        ///@brief Advance the position of the lexer to the next character.
        void advance( );

        ///@brief Look ahead one position from the current position in the source code.
        char* peek_ahead( );
    };
}