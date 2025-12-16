#include <juno/lexer/lexer.hpp>
#include <print>

std::int32_t main( )
{
    const std::string test_source = "let x = 12;";

    try
    {
        juno::lexer lexer { test_source };

        for (
            const auto tokens = lexer.tokenize( );
            const auto& token : tokens
        )
            std::println("token value = {}", token.value);

    } catch ( std::exception& err )
    {
        std::println( "runtime error: {}", err.what(  ) );
    }

    return 0;
}