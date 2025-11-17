#pragma once
#include <memory>

class Expression
{
public:
    virtual ~Expression( ) = default;

    enum ExpressionType
    {
        Number,
        BinaryExpression
    };

    explicit Expression(ExpressionType type)
        : m_type { type }
    {}
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
    std::unique_ptr< Expression > m_expression { };
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
private:
    double m_value { };
};