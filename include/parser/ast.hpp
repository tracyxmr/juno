#pragma once
#include <memory>

class Expression
{
public:
    virtual ~Expression( ) = default;

    std::strong_ordering operator<=> (const Expression & rhs ) const = default;

    enum ExpressionType
    {
        Number,
        BinaryExpr
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

class Number final : public Expression
{
public:
    explicit Number( const double value )
        : Expression { ExpressionType::Number },
          m_value { value }
    { }

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

class BinaryExpression final : public Expression
{
public:
    enum Operator
    {
        ADD,
        SUB,
        MUL,
        DIV,
        NOP,    /// No operation
    };

    explicit BinaryExpression(
        std::unique_ptr< Expression > lhs,
        std::unique_ptr< Expression > rhs, const Operator op
    ) : Expression { BinaryExpr },
        m_lhs { std::move( lhs ) },
        m_rhs { std::move( rhs ) },
        m_op { op }
    {}

private:
    std::unique_ptr< Expression > m_lhs { nullptr };
    std::unique_ptr< Expression > m_rhs { nullptr };
    Operator m_op { NOP };
};