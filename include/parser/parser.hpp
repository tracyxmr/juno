#pragma once
#include "ast.hpp"
#include "lexer/token.hpp"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace parser
{
    class Parser
    {
    public:
        explicit Parser( const std::vector< token::Token >& tokens )
            : m_tokens { tokens }
        { }

        ///@brief Parse the vector of tokens into a vector of statements.
        std::vector< std::unique_ptr< Statement > > parse();
    private:
        std::vector< token::Token > m_tokens { };
        token::Token m_current { };

        ///@brief Parse a statement.
        std::unique_ptr< Statement > parse_stmt();
        ///@brief Parse a variable declaration.
        std::unique_ptr< Statement > parse_var_decl();
        ///@brief Parse an assignment to a variable.
        std::unique_ptr< Statement > parse_assignment();
        ///@brief Parse a compound assignment.
        std::unique_ptr< Statement > parse_comp_assignment();
        ///@brief Parse an expression statement (a statement which wraps an expression).
        std::unique_ptr< Statement > parse_expr_stmt();
        ///@brief Parse a collection of statements into a block.
        std::unique_ptr< Statement > parse_block();
        ///@brief Parse a function prototype as named or anonymous (lambda).
        std::unique_ptr< Statement > parse_prototype();
        ///@brief Parse a function prototype as named or anonymous (lambda).
        std::unique_ptr< Statement > parse_lambda();
        ///@brief Parse an extern function prototype / decl
        std::unique_ptr< Statement > parse_extern_proto();
        ///@brief Parse a return statement.
        std::unique_ptr< Statement > parse_return();
        ///@brief Parse an if statement.
        std::unique_ptr< Statement > parse_if_stmt();

        ///@brief Parse function prototype parameters.
        std::vector< Parameter > parse_params();

        ///@brief Parse an expression.
        std::unique_ptr< Expression > parse_expr();
        ///@brief Parse a precedence level.
        std::unique_ptr< Expression > parse_precedence( int min_precedence );
        ///@brief Parse a primary expression.
        std::unique_ptr< Expression > parse_prim();
        ///@brief Parse a numeric literal as a number expression.
        std::unique_ptr< Expression > parse_number();
        ///@brief Parse a boolean literal as a number expression.
        std::unique_ptr< Expression > parse_boolean();
        ///@brief Parse a string literal as a string expression.
        std::unique_ptr< Expression > parse_string();
        ///@brief Parse an identifier expression.
        std::unique_ptr< Expression > parse_identifier();
        ///@brief Parse a group of expressions encapsulated by parenthesis.
        std::unique_ptr< Expression > parse_group();
        ///@brief Parse a list of arguments.
        std::vector< std::unique_ptr< Expression > > parse_args();

        std::size_t m_position { 0 };

        ///@brief Advance onto the next token in the tokens vector.
        void eat( );

        ///@brief Expect the current token type to be an expected
        ///type, if not an error will be thrown.
        ///If no error is thrown, the function will return the value of the token.
        std::string expect( token::TokenType type, std::string_view error_message );

        ///@brief Check if the current token matches type of the given type AND DO NOT CONSUME
        [[nodiscard]]
        bool check( token::TokenType type ) const;

        ///@brief Check if the current token matches the type of the givenn type and consume.
        ///@return true if the token was matched.
        bool match( token::TokenType type );

        ///@brief Check ahead in the parser to look at the next token
        [[nodiscard]]
        bool check_ahead( token::TokenType type ) const;

        ///@brief Helper function which creates a new BinaryOp struct if the current token is one.
        [[nodiscard]]
        std::optional< BinaryOp > binary_op( ) const;

        ///@brief Helper function which returns the associated compound operator with it's token.
        [[nodiscard]]
        std::optional< CompoundOperator > compound_op( ) const;

        ///@brief Helper function to determine if the token ahead type is a compound operator.
        [[nodiscard]]
        bool is_compound_op_ahead( ) const;
    };
}
