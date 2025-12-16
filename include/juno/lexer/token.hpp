#pragma once

#include <string>

namespace juno
{
    ///@brief An enum for all token types in Juno
    enum class token_type
    {
        IDENTIFIER,
        STRING,
        NUMBER,
        LET,
        MUT,
        EQUALS,
        ADD,
        SUB,
        MUL,
        DIV,
        SEMI,
        COLON,
        T_EOF,
    };

    ///@brief A structure which represents a token
    struct token
    {
        token_type type { token_type::T_EOF };
        std::string value;
        std::size_t line { 0 };
        std::size_t column { 0 };

        token(
            const token_type type,
            std::string value,
            const std::size_t line,
            const std::size_t column
        ) :
            type( type ),
            value( std::move( value ) ),
            line( line ),
            column( column )
        { }

        token( ) = default;
    };
}