#include "constants.hpp"
#include "system_util.hpp"
#include "compiler/compiler.hpp"
#include "evaluator/eval_visitor.hpp"
#include "jnvm/machine.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <print>

using namespace lexer;
using namespace parser;
using namespace jnvm::machine;

// @brief The entry point of the application.
std::int32_t main( )
{
    std::println( "{} {} ({} {} on {}) {}",
        constants::APP_NAME,
        constants::APP_VERSION,
        constants::APP_COMMIT,
        constants::COMPILER_INFO,
        constants::BUILD_ARCH,
        system_util::get_system_platform(  )
    );

    constexpr std::string_view test_source = "1 + 1";

    Lexer lexer { test_source };
    Parser parser { lexer.tokenize(  ) };
    Compiler compiler { parser.parse(  ) };
    Machine machine;

    auto bytecode { compiler.compile(  ) };

    std::println("bytecode: {}", bytecode);

    machine.load( bytecode );
    auto result { machine.execute(  ) };
    std::println( "Machine Result (returns first reg, register 3 holds the value): {}", result );

    return EXIT_SUCCESS;
}