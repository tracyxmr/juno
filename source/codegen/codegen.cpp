#include <format>
#include <codegen/codegen.hpp>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <fstream>
#include <stdexcept>

namespace codegen
{
    void codegen::visit( const Number &n )
    {
        m_last = llvm::ConstantFP::get( m_context, llvm::APFloat( n.get_value(  ) ) );
    }

    void codegen::visit( const String &s )
    {
        m_last = m_ir_builder.CreateGlobalStringPtr( s.get_value(  ) );
    }

    void codegen::visit( const BinaryExpression &b )
    {
        auto* lhs { generate_expr( b.get_lhs(  ).get(  ) ) };
        auto* rhs { generate_expr( b.get_rhs(  ).get(  ) ) };

        if ( !lhs || !rhs ) error( "Invalid operands in binary expression." );

        switch ( b.get_op(  ).op )
        {
            case BinaryOp::ADD: m_last = m_ir_builder.CreateFAdd( lhs, rhs, "addtmp" ); break;
            case BinaryOp::SUB: m_last = m_ir_builder.CreateFSub( lhs, rhs, "subtmp" ); break;
            case BinaryOp::MUL: m_last = m_ir_builder.CreateFMul( lhs, rhs, "multmp" ); break;
            case BinaryOp::DIV: m_last = m_ir_builder.CreateFDiv( lhs, rhs, "divtmp" ); break;
            case BinaryOp::LT:
            {
                m_last = m_ir_builder.CreateFCmpULT( lhs, rhs, "cmptmp" );
                m_last = m_ir_builder.CreateUIToFP( m_last, llvm::Type::getDoubleTy( m_context ), "booltmp" );
                break;
            }
            case BinaryOp::GT:
            {
                m_last = m_ir_builder.CreateFCmpUGT( lhs, rhs, "cmptmp" );
                m_last = m_ir_builder.CreateUIToFP( m_last, llvm::Type::getDoubleTy( m_context ), "booltmp" );
                break;
            }
            case BinaryOp::LTE:
            {
                m_last = m_ir_builder.CreateFCmpULE( lhs, rhs, "cmptmp" );
                m_last = m_ir_builder.CreateUIToFP( m_last, llvm::Type::getDoubleTy( m_context ), "booltmp" );
                break;
            }
            case BinaryOp::GTE:
            {
                m_last = m_ir_builder.CreateFCmpUGE( lhs, rhs, "cmptmp" );
                m_last = m_ir_builder.CreateUIToFP( m_last, llvm::Type::getDoubleTy( m_context ), "booltmp" );
                break;
            }
            case BinaryOp::EQ:
            {
                m_last = m_ir_builder.CreateFCmpUEQ( lhs, rhs, "cmptmp" );
                m_last = m_ir_builder.CreateUIToFP( m_last, llvm::Type::getDoubleTy( m_context ), "booltmp" );
                break;
            }
            case BinaryOp::NEQ:
            {
                m_last = m_ir_builder.CreateFCmpUNE( lhs, rhs, "cmptmp" );
                m_last = m_ir_builder.CreateUIToFP( m_last, llvm::Type::getDoubleTy( m_context ), "booltmp" );
                break;
            }
            default: error("Unknown binary operator.");
        }
    }

    void codegen::visit( const CallExpression &c )
    {
        /* Try to find to the function name in the module */
        auto * c_func { m_module->getFunction( c.get_callee(  ) ) };
        if ( !c_func ) error( std::format( "Unknown function '{}'", c.get_callee(  ) ) );

        /* Resolve the amount of args passed */
        const auto need_arg_count { c_func->arg_size(  ) };
        if ( const auto got_arg_count { c.get_args( ).size( ) }; need_arg_count != got_arg_count )
            error( std::format( "Function '{}' has gotten too many arguments, wanted {} but got {}",
                c.get_callee(  ), need_arg_count, got_arg_count  ) );

        /* generate Each argument */
        std::vector< llvm::Value* > args_ { };
        for ( const auto& a : c.get_args(  ) )
        {
            args_.emplace_back( generate_expr( a.get(  ) ) );
            if ( !args_.back(  ) ) error( "Failed to generate argument in function call" );
        }

        if ( c_func->getReturnType(  )->isVoidTy(  ) ) m_last = m_ir_builder.CreateCall( c_func, args_ );
        else m_last = m_ir_builder.CreateCall( c_func, args_, "calltmp" );
    }

    void codegen::visit( const IdentifierLit &i )
    {
        /* Ensure the variable exists in the symbol table */
        const auto value { i.get_value(  ) };
        auto iter { m_symbol_table.find( value ) };
        if ( iter == m_symbol_table.end( ) ) error( std::format( "Unknown variable '{}'", value ) );

        m_last = m_ir_builder.CreateLoad(
            llvm::Type::getDoubleTy( m_context ),
            iter->second,
            value
        );
    }

    void codegen::visit( const FunctionPrototype &fp )
    {
        /* Initialize a vector of llvm Type* for the param types */
        const std::vector proto_param_types { fp.get_params(  ).size(  ), llvm::Type::getDoubleTy( m_context ) };
        /* Get the corresponding LLVMs type for Juno type for the return type of the proto */
        auto* proto_return_type { get_type( fp.get_return_type(  ).get(  ) ) };
        /* Get the FunctionType from the return type and params types */
        auto* fn_type { llvm::FunctionType::get( proto_return_type, proto_param_types, false ) };

        /* Finally create the function. */
        auto fn_name { fp.is_lambda(  ) ? "" : std::string { fp.get_name(  ) } };
        /* If it's a lambda create a special name for it */
        if ( fn_name.empty(  ) ) fn_name = std::format( "lm_{}", reinterpret_cast< std::uintptr_t >( &fp ) );

        auto* fn { llvm::Function::Create( fn_type, llvm::Function::ExternalLinkage, fn_name, m_module.get(  ) ) };

        /* Name the parameters in the function */
        std::size_t a_idx { 0 };
        for ( auto& a : fn->args(  ) )
            a.setName( fp.get_params(  )[ a_idx++ ].name );

        /* Give the function an entry block */
        auto* entry_block { llvm::BasicBlock::Create( m_context, "entry", fn ) };
        m_ir_builder.SetInsertPoint( entry_block );

        /* We need to save the function context before, so do it */
        auto* before_fn { m_current_func };
        const auto before_symbol_table { m_symbol_table };

        m_current_func = fn;
        m_symbol_table.clear( );

        /* Give the parameters enough space and place them into the symbol table */
        for ( auto& a : fn->args(  ) )
        {
            auto* alloc { m_ir_builder.CreateAlloca( llvm::Type::getDoubleTy( m_context ), nullptr, a.getName(  ) ) };
            m_ir_builder.CreateStore( &a, alloc );
            m_symbol_table[ std::string( a.getName(  ) ) ] = alloc;
        }

        /* Okay now generate the function body :sob: */
        generate_block( fp.get_body(  ).get(  ) );

        /* Add an implicit return if the function is the main function */
        if ( !m_ir_builder.GetInsertBlock()->getTerminator() && fp.get_name(  ) == "main" )
        {
            m_ir_builder.CreateRet( llvm::ConstantInt::get( proto_return_type, 0 ) );
        }

        /* Ensure the function is okay */
        if ( llvm::verifyFunction( *fn, &llvm::errs( ) ) )
        {
            fn->eraseFromParent(  );
            error( "LLVM failed to verify function" );
        }

        m_last = fn;
        m_symbol_table = before_symbol_table;
        m_current_func = before_fn;
    }

    void codegen::visit( const FunctionExpression &fp )
    {
        if ( fp.get_proto(  ) ) visit( *fp.get_proto(  ) );
    }

    void codegen::generate_stmt( Statement *s )
    {
        if ( !s ) return;

        if ( auto* v { dynamic_cast< VariableDeclaration* >( s ) })
        {
            generate_var_decl( v );
        }
        else if ( auto* a { dynamic_cast< Assignment* >( s ) })
        {
            generate_assign( a );
        }
        else if ( auto* c { dynamic_cast< CompoundAssignment* >( s ) })
        {
            generate_comp_assign( c );
        }
        else if ( auto* b { dynamic_cast< BlockStmt* >( s ) })
        {
            generate_block( b );
        }
        else if ( auto* i { dynamic_cast< IfStatement* >( s ) })
        {
            generate_if_stmt( i );
        }
        else if ( auto* r { dynamic_cast< ReturnStatement* >( s ) })
        {
            generate_return( r );
        }
        else if ( auto* e { dynamic_cast< ExpressionStatement* >( s ) })
        {
            generate_expr_stmt( e );
        }
        else if ( auto* f { dynamic_cast< FunctionPrototype* >( s ) })
        {
            visit(*f);
        }
        else if ( auto* ex { dynamic_cast< ExternalFunctionProto* >( s ) })
        {
            generate_extern_proto( ex );
        }
    }

    void codegen::generate_expr_stmt( ExpressionStatement *e )
    {
        if ( e && e->get_expression( ) ) generate_expr( e->get_expression( ) );
    }

    void codegen::generate_var_decl( VariableDeclaration *v )
    {
        /* generate the declaration value for the variable */
        auto* initializer { generate_expr( v->get_value(  ).get(  ) ) };
        if ( !initializer ) error( std::format( "Failed to generate initializer for variable '{}'", v->get_name(  ) ) );

        /* Give some stack space for the variable */
        auto* alloc { m_ir_builder.CreateAlloca( initializer->getType(  ), nullptr, std::string( v->get_name(  ) ) ) };
        /* Store it */
        m_ir_builder.CreateStore( initializer, alloc );
        m_symbol_table[ std::string( v->get_name(  ) ) ] = alloc;
    }

    void codegen::generate_assign( Assignment *a )
    {
        /* Find the variable */
        const auto value { a->get_name(  ) };
        const auto iter { m_symbol_table.find( value ) };
        if ( iter == m_symbol_table.end( ) ) error( std::format( "Unknown variable '{}'", value ) );

        auto* assign_value { generate_expr( a->get_value(  ).get(  ) ) };
        if ( !assign_value ) error( std::format( "Failed to generate assign value for variable '{}'", value ) );

        m_ir_builder.CreateStore( assign_value, iter->second );
    }

    void codegen::generate_comp_assign( CompoundAssignment *c )
    {
        /* Find the variable */
        const auto value { c->get_name(  ) };
        const auto iter { m_symbol_table.find( value ) };
        if ( iter == m_symbol_table.end( ) ) error( std::format( "Unknown variable '{}'", value ) );

        auto* var_current_val { m_ir_builder.CreateLoad( llvm::Type::getDoubleTy( m_context ), iter->second, value ) };
        auto* rhs_op { generate_expr( c->get_value(  ).get(  ) ) };
        if ( !rhs_op ) error( std::format( "Failed to generate rvalue operand for compound assignment for variable '{}'", value ) );

        llvm::Value* result { nullptr };
        switch ( c->get_op(  ) )
        {
            case CompoundOperator::ADD: result = m_ir_builder.CreateFAdd( var_current_val, rhs_op, "addtmp" ); break;
            case CompoundOperator::SUB: result = m_ir_builder.CreateFSub( var_current_val, rhs_op, "subtmp" ); break;
            case CompoundOperator::MUL: result = m_ir_builder.CreateFMul( var_current_val, rhs_op, "multmp" ); break;
            case CompoundOperator::DIV: result = m_ir_builder.CreateFDiv( var_current_val, rhs_op, "divtmp" ); break;
        }

        m_ir_builder.CreateStore( result, iter->second );
    }

    void codegen::generate_block( BlockStmt *b )
    {
        for ( auto& s : b->get_body(  ) )
            generate_stmt( s.get(  ) );
    }

    void codegen::generate_if_stmt( IfStatement *i )
    {
        auto* condition { generate_expr( i->get_condition(  ).get(  ) ) };
        if ( !condition ) error( "Failed to generate condition in 'if' statement." );

        /* Condition must be truthy or falsey, so convert to bool */
        condition = m_ir_builder.CreateFCmpONE( condition, llvm::ConstantFP::get( m_context, llvm::APFloat(0.0) ), "ifcond" );
        auto* fn { m_ir_builder.GetInsertBlock(  )->getParent(  ) };

        auto* then_block { llvm::BasicBlock::Create( m_context, "then", fn ) };
        auto* else_block { llvm::BasicBlock::Create( m_context, "else" ) };
        auto* merge_block { llvm::BasicBlock::Create( m_context, "merge" ) };

        m_ir_builder.CreateCondBr( condition, then_block, else_block );

        /* Generate the then block */
        m_ir_builder.SetInsertPoint( then_block );
        generate_block( i->get_body(  ).get(  ) );
        m_ir_builder.CreateBr( merge_block );

        /* Else or Else if */
        fn->insert( fn->end(  ), else_block );
        m_ir_builder.SetInsertPoint( else_block );

        if ( i->has_else_if(  ) )
        {
            generate_if_stmt( i->get_else_if(  )->get(  ) );
        }
        else if ( i->has_else(  ) )
        {
            generate_block( i->get_else_body(  )->get(  ) );
        }

        m_ir_builder.CreateBr( merge_block );

        fn->insert( fn->end(  ), merge_block );
        m_ir_builder.SetInsertPoint( merge_block );
    }

    void codegen::generate_return( ReturnStatement *r )
    {
        if ( r->has_value(  ) )
        {
            auto* val { generate_expr( r->get_value(  ).get(  ) ) };
            if ( !val ) error( "Failed to generate return value" );
            m_ir_builder.CreateRet( val );
        }
        else
        {
            m_ir_builder.CreateRetVoid(  );
        }
    }

    void codegen::generate_extern_proto( ExternalFunctionProto *e )
    {
        /* buuild param types */
        std::vector< llvm::Type* > p_types;
        for ( const auto& p : e->get_params(  ) )
            p_types.push_back( get_type( p.type.get(  ) ) );

        /* return type */
        auto* ret_type { get_type( e->get_return_type(  ).get(  ) ) };

        /* var arg function? */
        bool vararg { false };
        auto name { e->get_name(  ) };

        /* TODO: add a proper way to ensure a function is vararg or not */
        if ( name == "printf" ) vararg = true;

        auto* fn_type { llvm::FunctionType::get(
            ret_type,
            p_types,
            vararg
        ) };

        auto* fn { llvm::Function::Create(
            fn_type,
            llvm::Function::ExternalLinkage,
            name,
            m_module.get(  )
        ) };

        std::size_t p_idx { 0 };
        for ( auto& a : fn->args(  ) )
            a.setName( e->get_params(  )[ p_idx++ ].name );

        m_last = fn;
    }

    llvm::Value *codegen::generate_expr( const Expression *expr )
    {
        if ( !expr ) return nullptr;
        expr->accept( *this );
        return m_last;
    }

    void codegen::write( const std::string &file_name ) const
    {
        std::error_code error_code;
        llvm::raw_fd_ostream output { file_name, error_code };

        if ( error_code )
            throw std::runtime_error( std::format( "Failed to open file {}, error: {}", file_name, error_code.message(  ) ) );

        m_module->print( output, nullptr );
        // m_module->print(llvm::outs(), nullptr);
    }

    llvm::Type *codegen::get_type( const Type *type )
    {
        if ( !type ) return llvm::Type::getVoidTy( m_context );

        if ( type->kind == TypeKind::Simple )
        {
            auto& name { type->name };
            if ( name == "double" ) return llvm::Type::getDoubleTy( m_context );
            if ( name == "void" ) return llvm::Type::getVoidTy( m_context );
            if ( name == "int" ) return llvm::Type::getInt32Ty( m_context );
            if ( name == "bool" ) return llvm::Type::getInt1Ty( m_context );
            if ( name == "string" ) return llvm::PointerType::get( m_context, 0 );
        }

        return llvm::Type::getDoubleTy( m_context );
    }

    llvm::Function *codegen::get_function( const std::string &name ) const
    {
        return m_module->getFunction( name );
    }

    void codegen::error( const std::string &message )
    {
        throw std::runtime_error("[juno::codegen_error] " + message);
    }

    void codegen::register_builtins( )
    {
        {
            auto* i8_ptr = llvm::PointerType::get(m_context, 0);
            const std::vector<llvm::Type*> printf_args = { i8_ptr };
            auto* printf_type = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(m_context),
                printf_args,
                true
            );
            llvm::Function::Create(
                printf_type,
                llvm::Function::ExternalLinkage,
                "printf",
                m_module.get()
            );
        }

        {
            auto* i8_ptr = llvm::PointerType::get(m_context, 0);
            std::vector<llvm::Type*> println_args = { i8_ptr };
            auto* println_type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(m_context),
                println_args,
                false
            );

            auto* println_fn = llvm::Function::Create(
                println_type,
                llvm::Function::InternalLinkage,
                "println",
                m_module.get()
            );

            auto* entry = llvm::BasicBlock::Create(m_context, "entry", println_fn);
            m_ir_builder.SetInsertPoint(entry);
            auto* msg_arg = println_fn->getArg(0);
            auto* printf_fn = m_module->getFunction("printf");
            auto* format_str = m_ir_builder.CreateGlobalString("%s\n");
            m_ir_builder.CreateCall(printf_fn, { format_str, msg_arg });
            m_ir_builder.CreateRetVoid();
        }
    }
}