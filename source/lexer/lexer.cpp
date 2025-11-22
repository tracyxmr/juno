#include <print>
#include <lexer/lexer.hpp>

std::vector<token::Token> lexer::Lexer::tokenize()
{
    std::vector<token::Token> tokens;

    while (pos < source.size()) {
        if ( auto current = source[ pos ]; current == ' '  ||
            current == '\r' ||
            current == '\t' ||
            current == '\n'
        )
        {
            advance(  );
        } else if ( token_character_map.contains( current ) ) /// Process single character tokens such as + - * /
        {
            auto token_type { token_character_map.at( current ) };
            tokens.emplace_back(
                token_type,
                std::string { current },
                line,
                col
            );

            advance(  );
        } else if ( std::isalpha( current ) ) /// Process multiple alphabetical characters as an identifier or keyword.
        {
            const std::size_t start { pos };
            std::size_t start_col { col };
            while ( pos < source.size( ) && std::isalpha( source.at( pos ) )) advance(  );

            const std::string value { source.substr( start, pos - start ) };
            token::TokenType token_type { token::IDENTIFIER };
            if ( token_keywords_map.contains( value ) )
            {
                token_type = token_keywords_map.at( value );
            }

            tokens.emplace_back(
                token_type,
                value,
                line,
                start_col
            );
        } else if ( std::isdigit( current ) ) /// Process multiple numerical characters as a digit
        {
            const std::size_t start { pos };
            std::size_t start_col { col };
            bool is_float { false };

            while (pos < source.size(  ))
            {
                if (std::isdigit( source.at( pos ) ))
                {
                    advance(  );
                } else if ( source.at( pos )== '.' && !is_float && std::isdigit( *peek_ahead(  ) ) )
                {
                    is_float = true;
                    advance(  );
                } else
                {
                    break;
                }
            }

            tokens.emplace_back(
                token::TokenType::NUMBER,
                std::string { source.substr( start, pos - start ) },
                line,
                start_col
            );
        }
    }

    tokens.emplace_back(token::TokenType::END_OF_FILE, std::string(""), line, col);

    return tokens;
}

void lexer::Lexer::advance()
{
    if (pos < source.length()) {
        if (source[pos] == '\n') {
            ++line;
            col = 1;
        } else {
            ++col;
        }

        ++pos;
    }
}

char *lexer::Lexer::peek_ahead( )
{
    return &source.at( pos + 1 );
}