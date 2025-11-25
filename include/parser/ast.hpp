#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

class IdentifierLit;
class CallExpression;
class BinaryExpression;
class FunctionPrototype;
class FunctionExpression;
class Number;

///@brief This essentially allows a more robust way to access
///the underlying values of a derived class of an Expression.
///Hence, the name "Visitor", we can take a reference to the
///derived class and access it's public methods or values.
class Visitor
{
public:
    virtual ~Visitor() = default;
    virtual void visit( const Number& n ) = 0;
    virtual void visit( const BinaryExpression& b ) = 0;
    virtual void visit( const CallExpression& c ) = 0;
    virtual void visit( const IdentifierLit& c ) = 0;
    virtual void visit( const FunctionPrototype& c ) = 0;
    virtual void visit( const FunctionExpression& c ) = 0;
};

class Expression
{
public:
    virtual ~Expression( ) = default;

    virtual void accept( Visitor& v ) const = 0;

    std::strong_ordering operator<=> (const Expression & rhs ) const = default;

    enum ExpressionType
    {
        Number,
        BinaryExpr,
        CallExpr,
        Identifier,
    };

    explicit Expression( const ExpressionType type)
        : m_type { type }
    { }

    friend bool operator== ( const Expression &lhs, const Expression &rhs )
    {
        return lhs.m_type == rhs.m_type;
    }

    friend bool operator!= ( const Expression &lhs, const Expression &rhs )
    {
        return !( lhs == rhs );
    }

  private:
    ExpressionType m_type { };
};

class Statement
{
public:
    virtual ~Statement() = default;
};

class ExpressionStatement final : public Statement
{
public:
    explicit ExpressionStatement( std::unique_ptr< Expression > expr )
        : m_expression { std::move( expr ) }
    {}

    [[nodiscard]]
    Expression* get_expression() const
    {
        return m_expression.get();
    }
private:
    std::unique_ptr< Expression > m_expression { nullptr };
};

class BlockStmt final : public Statement
{
public:
    explicit BlockStmt( std::vector< std::unique_ptr< Statement > > body, const bool profiled )
        : m_profiled { profiled }, m_body { std::move( body ) }
    { }

    [[nodiscard]] bool is_profiled( ) const
    {
        return m_profiled;
    }

    [[nodiscard]]
    std::vector< std::unique_ptr< Statement > >& get_body( )
    {
        return m_body;
    }

    [[nodiscard]]
    const std::vector< std::unique_ptr< Statement > >& get_body( ) const
    {
        return m_body;
    }

private:
    ///@brief If the user prefixes a code block with @profile, this block will be profiled.
    bool m_profiled { false };
    std::vector< std::unique_ptr< Statement > > m_body { };
};

enum class TypeKind
{
    Simple,     /// int, string
    Generic,    /// List<T>
    Function,   /// (int) -> int
    Array,      /// int[]
    Optional,   /// int?
};

struct Type
{
    TypeKind kind;          /// Kind of the type of the type, what?
    std::string name;       /// The name of the type e.g int
    std::optional< std::vector< std::unique_ptr< Type > > > generic_type_args;   /// Types inside generics e.g Something<A, B, C>
};

class VariableDeclaration final : public Statement
{
public:
    explicit VariableDeclaration( std::string name, std::unique_ptr< Expression > value, std::unique_ptr< Type > type, bool comptime )
        : m_name { std::move( name ) }, m_type { std::move( type ) }, m_value { std::move( value ) }, m_comptime_value { comptime }
    {}

    [[nodiscard]]
    const std::unique_ptr< Expression > &get_value( ) const
    {
        return m_value;
    }

    [[nodiscard]]
    const std::unique_ptr< Type > &get_type( ) const
    {
        return m_type;
    }

    [[nodiscard]]
    std::string_view get_name( ) const
    {
        return m_name;
    }

    [[nodiscard]]
    bool is_comptime( ) const
    {
        return m_comptime_value;
    }

  private:
    std::string m_name;
    std::unique_ptr< Type > m_type { nullptr };
    std::unique_ptr< Expression > m_value { nullptr };
    ///@brief If @comptime is encountered the variables value will be evaluated if supported.
    bool m_comptime_value { false };
};

class IdentifierLit final : public Expression
{
public:
    explicit IdentifierLit( std::string_view value )
        : Expression { Identifier }, /// 23/11/2025 - removed redundant qualifier
          m_value { value }
    { }

    void accept(Visitor &v) const override
    {
        v.visit( *this );
    }

    [[nodiscard]]
    const std::string& get_value( ) const
    {
        return m_value;
    }
private:
    std::string m_value;
};

class Number final : public Expression
{
public:
    explicit Number( const double value )
        : Expression { ExpressionType::Number },
          m_value { value }
    { }

    void accept(Visitor &v) const override
    {
        v.visit( *this );
    }

    [[nodiscard]]
    double get_value( ) const
    {
        return m_value;
    }

    friend bool operator== ( const Number &lhs, const Number &rhs )
    {
        return lhs == static_cast< const Expression & >( rhs ) && lhs.m_value == rhs.m_value;
    }

    friend bool operator== ( const Number &lhs, const double rhs)
    {
        return lhs.m_value == rhs;
    }

    friend bool operator!= ( const Number &lhs, const Number &rhs )
    {
        return !( lhs == rhs );
    }

    friend bool operator< ( const Number &lhs, const Number &rhs )
    {
        if ( static_cast< const Expression & >( lhs ) < static_cast< const Expression & >( rhs ) )
            return true;
        if ( static_cast< const Expression & >( rhs ) < static_cast< const Expression & >( lhs ) )
            return false;
        return lhs.m_value < rhs.m_value;
    }

    friend bool operator<= ( const Number &lhs, const Number &rhs )
    {
        return rhs >= lhs;
    }

    friend bool operator> ( const Number &lhs, const Number &rhs )
    {
        return rhs < lhs;
    }

    friend bool operator>= ( const Number &lhs, const Number &rhs )
    {
        return !( lhs < rhs );
    }

  private:
    double m_value { 0.0 };
};

struct BinaryOp
{
    enum Type
    {
        ADD,
        SUB,
        MUL,
        DIV,
        NOP,    /// No operation
    };

    Type op;        /// Operator

    ///@brief Returns the op Type as a string
    [[nodiscard]]
    std::string to_string() const
    {
        switch ( op )
        {
            case ADD: return "ADD";
            case SUB: return "SUB";
            case MUL: return "MUL";
            case DIV: return "DIV";
            case NOP: return "NOP";
        }

        return "";
    }

    ///@brief Returns the precedence level depending on type.
    [[nodiscard]]
    int precedence( ) const
    {
        switch ( op )
        {
            case ADD: case SUB: return 1;
            case MUL: case DIV: return 2;
            case NOP: return -1;
            default: throw std::runtime_error( "[juno::ast_error] unknown operation type in BinaryOp" );
        }
    }
};

class CallExpression final : public Expression
{
public:
    explicit CallExpression(
        std::string callee,
        std::vector< std::unique_ptr< Expression > > args
    ) :
      Expression(CallExpr),
      m_callee { std::move(callee) },
      m_args { std::move(args) }
    {
    }

    [[nodiscard]]
    const std::string& get_callee() const
    {
        return m_callee;
    }

    [[nodiscard]]
    const std::vector< std::unique_ptr< Expression > >& get_args() const
    {
        return m_args;
    }

    void accept(Visitor &v) const override
    {
        v.visit( *this );
    }

private:
    std::string m_callee;
    std::vector< std::unique_ptr< Expression > > m_args {};
};

class BinaryExpression final : public Expression
{
public:
    explicit BinaryExpression(
        std::unique_ptr< Expression > lhs,
        std::unique_ptr< Expression > rhs,
        const BinaryOp op
    ) : Expression { BinaryExpr },
        m_lhs { std::move( lhs ) },
        m_rhs { std::move( rhs ) },
        m_op { op }
    {
    }

    void accept( Visitor& v ) const override
    {
        v.visit(*this);
    }

    [[nodiscard]]
    std::unique_ptr< Expression > &get_lhs( )
    {
        return m_lhs;
    }

    [[nodiscard]]
    std::unique_ptr< Expression > &get_rhs( )
    {
        return m_rhs;
    }

    [[nodiscard]]
    BinaryOp &get_op( )
    {
        return m_op;
    }

    [[nodiscard]]
    const std::unique_ptr< Expression > &get_lhs( ) const
    {
        return m_lhs;
    }

    [[nodiscard]]
    const std::unique_ptr< Expression > &get_rhs( ) const
    {
        return m_rhs;
    }

    [[nodiscard]]
    const BinaryOp &get_op( ) const
    {
        return m_op;
    }

  private:
    std::unique_ptr< Expression > m_lhs { nullptr };
    std::unique_ptr< Expression > m_rhs { nullptr };
    BinaryOp m_op { BinaryOp::NOP };
};

///@brief This structure represents a parameter in a function prototype.
struct Parameter
{
    std::string name;
    std::unique_ptr< Type > type;

    explicit Parameter( std::string _name, std::unique_ptr< Type > _type )
        : name { std::move( _name ) }, type { std::move( _type ) }
    {}
};

///@brief This class represents a function prototype
/// this class supports named functions and lambdas
class FunctionPrototype final : public Statement
{
public:
    ///@brief This constructor is for named functions (non-lambdas).
    explicit FunctionPrototype(
        std::string name,
        std::vector< Parameter > params,
        std::unique_ptr< Type > ret_type,
        std::unique_ptr< BlockStmt > body
    ) :
        m_name { std::move( name ) },
        m_params { std::move( params ) },
        m_ret_type { std::move( ret_type ) },
        m_body { std::move( body ) }
    {}

    ///@brief This constructor is for lambdas.
    explicit FunctionPrototype(
        std::vector< Parameter > params,
        std::unique_ptr< Type > ret_type,
        std::unique_ptr< BlockStmt > body
    ) :
        m_params { std::move( params ) },
        m_ret_type { std::move( ret_type ) },
        m_body { std::move( body ) },
        m_is_lambda { true }
    {}

    [[nodiscard]]
    std::string_view get_name() const
    {
        return m_name;
    }

    [[nodiscard]]
    const std::vector< Parameter >& get_params() const
    {
        return m_params;
    }

    [[nodiscard]]
    const std::unique_ptr< Type >& get_return_type() const
    {
        return m_ret_type;
    }

    [[nodiscard]]
    const std::unique_ptr< BlockStmt >& get_body() const
    {
        return m_body;
    }

    [[nodiscard]]
    std::unique_ptr< BlockStmt >& get_body()
    {
        return m_body;
    }

    [[nodiscard]]
    bool is_lambda() const
    {
        return m_is_lambda;
    }
private:
    ///@brief Function prototype name, if it is empty then it's a lambda
    std::string m_name;
    ///@brief Function prototype's parameters.
    std::vector< Parameter > m_params;
    ///@brief Function prototype's return type.
    std::unique_ptr< Type > m_ret_type { nullptr };
    ///@brief Function prototype's body
    std::unique_ptr< BlockStmt > m_body { nullptr };
    ///@brief Determines if the function prototype is a lambda
    bool m_is_lambda { false };
};

///@brief A wrapper class for a function prototype, it allows for functions to become
///expressions
class FunctionExpression final : public Expression
{
public:
    explicit FunctionExpression( std::unique_ptr< FunctionPrototype > proto )
        : Expression { Identifier }, /// will change later, tired ash
          m_proto { std::move( proto ) }
    {}

    void accept(Visitor &v) const override
    {
        if ( m_proto ) v.visit( *m_proto );
    }

    [[nodiscard]]
    const std::unique_ptr< FunctionPrototype >& get_proto( ) const
    {
        return m_proto;
    }

    [[nodiscard]]
    std::unique_ptr< FunctionPrototype >& get_proto( )
    {
        return m_proto;
    }

  private:
    std::unique_ptr< FunctionPrototype > m_proto;
};

///@brief A simple return statement.
class ReturnStatement final : public Statement
{
public:
    explicit ReturnStatement( std::unique_ptr< Expression > value )
        : m_value { std::move( value ) } {}

    [[nodiscard]]
    const std::unique_ptr< Expression >& get_value() const
    {
        return m_value;
    }

    [[nodiscard]]
    std::unique_ptr< Expression >& get_value()
    {
        return m_value;
    }

    [[nodiscard]]
    bool has_value() const
    {
        return m_value != nullptr;
    }
private:
    std::unique_ptr< Expression > m_value { nullptr };
};

/*
 *  These functions below have been replaced by the visitor,
 *  but can still be used for other things.
 */

///@brief Safely cast a unique_ptr to a derived type, const.
///@return Pointer to the derived type, or nullptr if cast fails.
template<typename Derived, typename Base>
[[nodiscard]]
const Derived* safe_cast( const std::unique_ptr< Base >& ptr )
{
    return dynamic_cast<const Derived*>( ptr.get( ) );
}

///@brief Safely cast a unique_ptr to a derived type, which is non const.
template<typename Derived, typename Base>
[[nodiscard]]
Derived* safe_cast(std::unique_ptr<Base>& ptr)
{
    return dynamic_cast<Derived*>(ptr.get());
}