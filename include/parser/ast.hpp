#pragma once

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

class IdentifierLit;
class CallExpression;
class BinaryExpression;
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
    explicit BlockStmt( std::vector< std::unique_ptr< Statement > > body )
        : m_body { std::move( body ) }
    { }

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
    std::vector< std::unique_ptr< Statement > > m_body { };
};

class VariableDeclaration final : public Statement
{
public:
    explicit VariableDeclaration( std::string name, std::unique_ptr< Expression > value )
        : m_name { std::move( name ) }, m_value { std::move( value ) }
    {}

    [[nodiscard]]
    const std::unique_ptr< Expression > &get_value( ) const
    {
        return m_value;
    }

    [[nodiscard]]
    std::string_view get_name( ) const
    {
        return m_name;
    }

private:
    std::string m_name;
    std::unique_ptr< Expression > m_value { nullptr };
};

class IdentifierLit final : public Expression
{
public:
    explicit IdentifierLit( std::string_view value )
        : Expression { ExpressionType::Identifier },
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