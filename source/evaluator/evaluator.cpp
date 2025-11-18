#include <evaluator/evaluator.hpp>

void EvalVisitor::visit( const BinaryExpression &b )
{
    b.get_lhs(  )->accept( *this );
    const double left { m_result };

    b.get_rhs(  )->accept( *this );
    const double right { m_result };

    switch ( b.get_op(  ).op )
    {
        case BinaryOp::ADD: m_result = left + right; break;
        case BinaryOp::SUB: m_result = left - right; break;
        case BinaryOp::MUL: m_result = left * right; break;
        case BinaryOp::DIV: m_result = left / right; break;
        case BinaryOp::NOP: m_result = 0.0; break;
    }
}

void EvalVisitor::visit( const Number &n )
{
    m_result = n.get_value(  );
}
