#include "constants.hpp"
#include "system_util.hpp"
#include "evaluator/evaluator.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <print>

using namespace lexer;
using namespace parser;

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

    EvalVisitor eval_visitor;
    for (
        const auto ast { parser.parse( ) };
        const auto & stmt : ast )
    {
        if ( const auto * expr_stmt { dynamic_cast<const ExpressionStatement*>( stmt.get( ) ) } )
        {
            const auto * expr { expr_stmt->get_expression( ) };
            if ( auto * b { dynamic_cast<const BinaryExpression*>( expr ) } )
            {
                b->accept( eval_visitor );
                break;
            }
        }
    }

    std::println( "EvalVisitor result: {}", eval_visitor.get_result(  ) );

    return EXIT_SUCCESS;
}