#include <print>
#include <juno/lexer/lexer.hpp>

using namespace juno;

std::vector< token > lexer::tokenize( )
{
    std::vector< token > tokens;

    while ( m_pos < m_source.length(  ) )
    {
        if ( const auto current = m_source.at( m_pos );
            current == ' ' ||
            current == '\r' ||
            current == '\t' ||
            current == '\n'
        )
        {
            if ( current == '\n' )
            {
                m_line++;
                m_column = 1;
            }

            advance( );
        } else if ( current == '/' && peek_ahead(  ) && *peek_ahead(  ) == '/' )
        {
            advance(  ); advance(  );
            while ( m_pos < m_source.length(  ) && m_source.at( m_pos ) != '\n' ) advance(  );
        } else if ( token_character_map.contains( current ) )
        {
            auto type = token_character_map.at( current );
            tokens.emplace_back( type, std::string { current }, m_line, m_column );
            advance(  );
        } else if ( std::isalpha( current ) || current == '_' )
        {
            const std::size_t start_pos = m_pos;
            const std::size_t start_col = m_column;

            while ( m_pos < m_source.length(  ) && std::isalnum( m_source.at( m_pos ) ) ) advance(  );

            const std::string value = m_source.substr( start_pos, m_pos - start_pos );
            auto type { token_type::IDENTIFIER };

            if ( token_keyword_map.contains( value ) )
                type = token_keyword_map.at( value );

            tokens.emplace_back( type, value, m_line, start_col );
        } else if ( std::isdigit( current ) )
        {
            const std::size_t start_pos = m_pos;
            const std::size_t start_col = m_column;
            bool is_float = false;

            while ( m_pos < m_source.length(  ) )
            {
                if ( std::isdigit( m_source.at( m_pos ) ) ) advance(  );
                else if ( m_source.at( m_pos ) == '.' && !is_float && std::isdigit( *peek_ahead(  ) ) )
                {
                    is_float = true;
                    advance(  );
                } else break;
            }

            tokens.emplace_back(
                token_type::NUMBER,
                std::string { m_source.substr( start_pos, m_pos - start_pos ) },
                m_line,
                m_column
            );
        } else
        {
            m_pos++;
        }
    }

    tokens.emplace_back( token_type::T_EOF, "", m_line, m_column );

    return tokens;
}

void lexer::advance( )
{
    if ( m_pos < m_source.length(  ) )
    {
        m_column++;
        m_pos++;
    }
}

char *lexer::peek_ahead( )
{
    return &m_source.at( m_pos + 1 );
}
