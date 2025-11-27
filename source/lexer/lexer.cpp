#include <optional>
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
        } else if ( current == '/' && peek_ahead(  ) && *peek_ahead(  ) == '/') {
            advance(  ); advance(  );
            while ( pos < source.size(  ) && source.at( pos ) != '\n' ) advance(  );
        } else if ( const auto token_value { try_compound( current, *peek_ahead(  ) ) }; peek_ahead(  ) && token_value.has_value(  ) )
        {
            tokens.emplace_back( token_value.value(  ) );
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
        } else if ( std::isalpha( current ) || current == '_' || current == '@' ) /// Process multiple alphabetical characters as an identifier or keyword.
        {
            const std::size_t start { pos };
            std::size_t start_col { col };
            if (current == '@') advance(  );

            while ( pos < source.size( ) && std::isalnum( source.at( pos ) )) advance(  );

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
        } else if ( current == '"' ) {
            advance(  );
            const std::size_t start { pos };
            std::size_t start_col { col };
            while ( pos < source.size(  ) && source[ pos ] != '"' )
            {
                if ( source[ pos ] == '\n' ) { ++line; col = 0; }
                advance(  );
            }

            if ( pos >= source.size( ) ) {
                throw std::runtime_error(
                    "[juno::error] Unterminated string at line " +
                    std::to_string(line)
                );
            }

            tokens.emplace_back(
                token::TokenType::STRING,
                std::string { source.substr( start, pos - start ) },
                line,
                start_col
            );

            advance(  );
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

    tokens.emplace_back(
        token::TokenType::END_OF_FILE,
        std::string(),
        line,
        col
    );

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

std::optional< token::Token > lexer::Lexer::try_compound( char first, char second )
{
    if ( pos + 1 >= source.size(  ) ) return std::nullopt;
    if ( source[ pos + 1 ] != second ) return std::nullopt;

    const std::string_view map_key { source.data(  ) + pos, 2 };
    if ( const auto iter { token_compound_map.find( map_key ) }; iter != token_compound_map.end() )
    {
        token::Token token { iter->second, std::string { map_key }, line, col };
        advance(  ); advance(  );
        return token;
    }

    return std::nullopt;
}
