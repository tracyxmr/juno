#include "constants.hpp"
#include "system_util.hpp"
#include "compiler/compiler.hpp"
#include "jnvm/machine.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <utility>

using namespace lexer;
using namespace parser;
using namespace jnvm::machine;

///@brief Load and parse a file.
std::vector< std::unique_ptr< Statement > > load_juno_file( const std::string& path )
{
    std::ifstream file { path };
    if ( !file ) throw std::runtime_error( std::format( "[juno::entry_error] failed to open file '{}'", path ) );

    std::string content;
    std::string line;
    while ( std::getline( file, line ) )
    {
        content += line;
        content.push_back( '\n' );
    }
    file.close(  );

    Lexer lexer { content };
    auto tokens { lexer.tokenize() };
    Parser parser { tokens };
    return parser.parse();
}

///@brief Load the standard libary
std::vector< std::unique_ptr< Statement > > load_std( )
{
    std::vector< std::unique_ptr< Statement > > ast;
    const std::filesystem::path path { "../../stdlib" };

    /* stdlib folder not found */
    if ( !std::filesystem::exists( path ) ) return ast;

    std::vector< std::string > modules { "core.jn" };

    for ( const auto& m : modules )
    {
        auto mpath { path / m };
        if ( !std::filesystem::exists( mpath ) ) continue;

        try
        {
            auto m_ast { load_juno_file( mpath.string( ) ) };
            ast.insert( ast.end(  ), std::make_move_iterator( m_ast.begin(  ) ), std::make_move_iterator( m_ast.end(  ) ) );
        } catch ( const std::exception& e )
        {
            throw std::runtime_error( e.what(  ) );
        }
    }

    return ast;
}

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

        const std::string test_path { "../../tests/main.jn" };
        auto file_ast { load_juno_file( test_path ) };

        Compiler compiler { std::move( file_ast ) };
        auto compile_result { compiler.compile(  ) };

        Machine machine;

        machine.load( compile_result.bytecode );
        machine.load_strings( compile_result.string_pool );
        auto result = machine.execute(  );
    }
    catch ( const std::exception& err )
    {
        std::cerr << err.what(  ) << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}