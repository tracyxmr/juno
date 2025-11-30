#include "constants.hpp"
#include "system_util.hpp"
#include "codegen/codegen.hpp"
#include "compiler/compiler.hpp"
#include "evaluator/eval_visitor.hpp"
#include "jnvm/machine.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "solver/solver.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <utility>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>

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

///@brief Run the module with LLVM JIT Execution
void jit_module( std::unique_ptr< llvm::Module > mod )
{
    llvm::InitializeNativeTarget(  );
    llvm::InitializeNativeTargetAsmPrinter(  );
    llvm::InitializeNativeTargetAsmParser(  );

    auto result { llvm::orc::LLJITBuilder( ).create(  ) };
    if ( !result ) throw std::runtime_error( std::format( "[juno::jit_error] Failed to create JIT builder, error: {}", llvm::toString( result.takeError(  ) ) ) );

    const auto jit { std::move( *result ) };

    if ( auto err { jit->addIRModule( llvm::orc::ThreadSafeModule( std::move( mod ), std::make_unique< llvm::LLVMContext >( ) ) ) } )
        throw std::runtime_error( std::format( "[juno::jit_error] Failed to add module to JIT, error: {}", llvm::toString( std::move( err ) ) ) );

    auto main { jit->lookup( "main" ) };
    if ( !main )
        throw std::runtime_error( std::format( "[juno::jit_error] Failed to find 'main' function, error: {}", llvm::toString( main.takeError(  ) ) ) );

    const auto fn { main->toPtr< double( * )( ) >(  ) };
    auto fn_result { fn( ) };
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
        const auto file_ast { load_juno_file( test_path ) };

        codegen::codegen codegen;

        for ( std::size_t i { 0 }; i < file_ast.size(); ++i )
            codegen.generate_stmt( file_ast[i].get() );

        codegen.write( "out.ll" );
        jit_module( codegen.release_module(  ) );
    }
    catch ( const std::exception& err )
    {
        std::cerr << err.what(  ) << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}