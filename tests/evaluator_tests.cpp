#include "../source/evaluator.h"

#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

using Token = EvaluationToken;
using Type = Token::Type;
using Variables = std::map<std::string, std::complex<double>>;

int failures = 0;

#define CHECK(condition) do { \
    if(!(condition)) { \
        std::cerr << "FAILED at line " << __LINE__ << ": " #condition "\n"; \
        ++failures; \
    } \
} while(false)

Token token(const Type type, const std::string_view value, const int position = 0)
{
    return {value, 1, position, type};
}

EvaluationResult evaluate(const std::initializer_list<Token> tokens, Variables& variables)
{
    return evaluate_tokens(std::vector<Token>(tokens), variables);
}

EvaluationResult evaluate(const std::initializer_list<Token> tokens)
{
    Variables variables;
    return evaluate(tokens, variables);
}

bool near(const std::complex<double> actual, const std::complex<double> expected)
{
    return std::abs(actual - expected) < 1e-12;
}

void test_arithmetic_and_unary_operators()
{
    const auto precedence = evaluate({
        token(Type::Number, "2"), token(Type::Operator, "+"),
        token(Type::Number, "3"), token(Type::Operator, "*"),
        token(Type::Number, "4"),
    });
    CHECK(!precedence.error);
    CHECK(near(precedence.value, {14.0, 0.0}));

    const auto right_associative_power = evaluate({
        token(Type::Number, "2"), token(Type::Operator, "^"),
        token(Type::Number, "3"), token(Type::Operator, "^"),
        token(Type::Number, "2"),
    });
    CHECK(!right_associative_power.error);
    CHECK(near(right_associative_power.value, {512.0, 0.0}));

    const auto negative_power = evaluate({
        token(Type::Operator, "-"), token(Type::Number, "2"),
        token(Type::Operator, "^"), token(Type::Number, "2"),
    });
    CHECK(!negative_power.error);
    CHECK(near(negative_power.value, {-4.0, 0.0}));

    const auto negative_exponent = evaluate({
        token(Type::Number, "2"), token(Type::Operator, "^"),
        token(Type::Operator, "-"), token(Type::Number, "3"),
    });
    CHECK(!negative_exponent.error);
    CHECK(near(negative_exponent.value, {0.125, 0.0}));

    const auto repeated_unary = evaluate({
        token(Type::Operator, "-"), token(Type::Operator, "-"),
        token(Type::Number, "2"),
    });
    CHECK(!repeated_unary.error);
    CHECK(near(repeated_unary.value, {2.0, 0.0}));
}

void test_negative_square_root()
{
    const auto result = evaluate({
        token(Type::Function, "sqrt"), token(Type::ParenOpen, ""),
        token(Type::Operator, "-"), token(Type::Number, "1"),
        token(Type::ParenClose, ""),
    });

    CHECK(!result.error);
    CHECK(near(result.value, {0.0, 1.0}));
}

void test_assignments_are_validated_and_transactional()
{
    Variables variables{{"x", {1.0, 0.0}}};
    const auto assigned = evaluate({
        token(Type::Number, "5"), token(Type::Operator, ">"),
        token(Type::Variable, "x"),
    }, variables);
    CHECK(!assigned.error);
    CHECK(near(assigned.value, {5.0, 0.0}));
    CHECK(near(variables.at("x"), {5.0, 0.0}));

    variables["x"] = {1.0, 0.0};
    const auto invalid = evaluate({
        token(Type::Number, "5"), token(Type::Operator, ">"),
        token(Type::Variable, "x"), token(Type::Operator, ">"),
        token(Type::Number, "3"),
    }, variables);
    CHECK(invalid.error);
    CHECK(near(variables.at("x"), {1.0, 0.0}));
}

void test_logical_errors_are_reported()
{
    const std::vector<std::vector<Token>> invalid_expressions{
        {token(Type::Operator, "*")},
        {token(Type::Number, "1"), token(Type::Operator, "+")},
        {token(Type::Number, "1"), token(Type::Number, "2")},
        {token(Type::ParenClose, "")},
        {token(Type::ParenOpen, ""), token(Type::Number, "1")},
        {token(Type::Function, "sqrt"), token(Type::ParenOpen, ""), token(Type::ParenClose, "")},
        {token(Type::Function, "unknown"), token(Type::ParenOpen, ""), token(Type::Number, "1"), token(Type::ParenClose, "")},
        {token(Type::Number, "1..2")},
        {token(Type::Number, "1"), token(Type::Operator, "?")},
        {token(Type::Number, "1"), token(Type::Operator, ">"), token(Type::Number, "2")},
    };

    for(const auto& expression : invalid_expressions)
    {
        Variables variables;
        CHECK(evaluate_tokens(expression, variables).error);
    }
}

} // namespace

int main()
{
    test_arithmetic_and_unary_operators();
    test_negative_square_root();
    test_assignments_are_validated_and_transactional();
    test_logical_errors_are_reported();

    if(failures != 0)
    {
        std::cerr << failures << " test assertion(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All evaluator tests passed\n";
    return EXIT_SUCCESS;
}
