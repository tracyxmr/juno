#include "constants.hpp"
#include "system_util.hpp"
#include "compiler/compiler.hpp"
#include "evaluator/eval_visitor.hpp"
#include "jnvm/machine.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "solver/solver.hpp"

#include <fstream>
#include <iostream>
#include <print>
#include <utility>

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

        const std::string test_path { "../../tests/if.jn" };

        std::ifstream file { test_path };
        std::string line;
        std::string content;
        while ( std::getline( file, line ) )
        {
            content += line;
            content.push_back( '\n' );
        }
        file.close(  );

        Lexer lexer { content };
        auto tokens { lexer.tokenize( ) };
        Parser parser { tokens };
        std::vector ast { parser.parse(  ) };
        Solver type_solver;
        type_solver.solve( ast );
        Compiler compiler { std::move( ast ) };
        Machine  machine;

        auto compiler_result { compiler.compile(  ) };

        machine.load( compiler_result.bytecode );
        machine.load_strings( compiler_result.string_pool );

        const auto first { machine.execute( ) };
    }
    catch ( const std::exception& err )
    {
        std::cerr << err.what(  ) << std::endl;
    }

    return EXIT_SUCCESS;
}