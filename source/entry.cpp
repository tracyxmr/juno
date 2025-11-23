#include "constants.hpp"
#include "system_util.hpp"
#include "compiler/compiler.hpp"
#include "evaluator/eval_visitor.hpp"
#include "jnvm/machine.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <iostream>
#include <print>

using namespace lexer;
using namespace parser;
using namespace jnvm::machine;

///@brief The entry point of the application.
std::int32_t main( )
{
    try
    {
        std::println( "{} {} ({} {} on {}) {}",
            constants::APP_NAME,
            constants::APP_VERSION,
            constants::APP_COMMIT,
            constants::COMPILER_INFO,
            constants::BUILD_ARCH,
            system_util::get_system_platform(  )
        );

        constexpr std::string_view test_source = "@profile { let x = 234 * 34562; let y = x / 23; print(y); }";

        Lexer lexer { test_source };
        auto tokens { lexer.tokenize( ) };
        Parser parser { tokens };
        Compiler compiler { parser.parse( ) };
        Machine  machine;

        auto bytecode { compiler.compile( ) };

        // std::println( "bytecode: {}", bytecode );

        machine.load( bytecode );
        machine.execute( );
    }
    catch ( const std::exception& err )
    {
        std::cerr << err.what(  ) << std::endl;
    }

    return EXIT_SUCCESS;
}