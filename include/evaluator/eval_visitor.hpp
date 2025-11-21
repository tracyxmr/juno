#pragma once

#include <parser/ast.hpp>

/*
 *  Evaluate certain expressions to improve performance
 *  for the virtual machine.
 *
 *  This uses a classic visitor approach.
 */

class EvalVisitor : public Visitor
{
public:
    void visit(const BinaryExpression &b) override;
    void visit(const Number &n) override;

    [[nodiscard]]
    double get_result() const { return m_result; }
private:
    double m_result { 0.0 };
};