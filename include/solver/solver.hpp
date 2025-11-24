#pragma once

#include <unordered_map>
#include <parser/ast.hpp>

///@brief A visitor class for statements.
class StmtVisitor
{
public:
    virtual ~StmtVisitor() = default;

    virtual void visit( const VariableDeclaration& v ) = 0;
    virtual void visit( const ExpressionStatement& e ) = 0;
    virtual void visit( const BlockStmt& b ) = 0;
    virtual void visit( const FunctionPrototype& f ) = 0;
    virtual void visit( const ReturnStatement& r ) = 0;
};

///@brief An extension for the Type class which provides override operators for checking types.
struct TypeExtended
{
    TypeKind kind;
    std::string name;

    ///@brief Default the type name to unknown.
    TypeExtended( ) : kind { TypeKind::Simple }, name { "unknown" } { }
    TypeExtended( const TypeKind tk, std::string _name ) : kind { tk }, name { std::move( _name ) } { }

    ///@brief A converter constructor
    explicit TypeExtended( const Type& t ) : kind { t.kind }, name { t.name } { }

    bool operator==( const TypeExtended &o ) const
    {
        return kind == o.kind && name == o.name;
    }

    bool operator!=( const TypeExtended& o ) const
    {
        return !( *this == o );
    }

    [[nodiscard]]
    std::string to_string( ) const
    {
        return name;
    }
};

/*
 *  Visit an expression or statement and ensure the types match with others
 *  or any annotations.
 */
class Solver : public Visitor, public StmtVisitor
{
public:
    Solver( ) = default;

    ///@brief Solve types for the whole AST
    void solve( const std::vector< std::unique_ptr< Statement > >& ast );

    ///@brief  Get the inferred type of the last visitedd experession
    [[nodiscard]]
    TypeExtended get_last( ) const { return m_last; }

    ///@brief Expression visitor methods
    void visit( const BinaryExpression &b ) override;
    void visit( const Number &n ) override;
    void visit( const CallExpression &c ) override;
    void visit( const IdentifierLit &i ) override;
    void visit( const FunctionPrototype &f ) override;
    void visit( const FunctionExpression &f ) override;


    ///@brief Statement visitor methods
    void visit( const VariableDeclaration &v ) override;
    void visit( const ReturnStatement &r ) override;
    void visit( const BlockStmt &b ) override;
    void visit( const ExpressionStatement &e ) override;

private:
    ///@brief Symbol table for variable types
    std::unordered_map< std::string, TypeExtended > m_symbols_tab;
    ///@brief Function signature table e.g name to return type
    std::unordered_map< std::string, TypeExtended > m_func_tab;
    ///@brief Current function return type, which is used for checking return statements.
    std::optional< TypeExtended > m_cur_fn_ret_type { std::nullopt };
    ///@brief The type of the last visited expression
    TypeExtended m_last;

    ///@brief This method is a helper which ensures two types are compatible
    void ensure_types_compatibility( const TypeExtended& expected, const TypeExtended& got, const std::string& ctx );
    ///@brief This method is a helper which simply registers a variable in the symbol table
    void register_var( const std::string& name, const TypeExtended& type );
    ///@brief THis method is a helper which registers a function in the function table
    void register_func( const std::string& name, const TypeExtended& ret_type );
    ///@brief This method is a helper which looks up a variables type
    TypeExtended lookup_variable_type( const std::string& name );
    ///@brief This method is a helper which looks up a functions return type
    TypeExtended lookup_return_type( const std::string& name );
    ///@brief This method is a helper which infers the type from an expression
    TypeExtended infer_type( const Expression* expr );
};