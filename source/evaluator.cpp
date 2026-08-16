#include "evaluator.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace {

using Complex = std::complex<double>;
using Token = EvaluationToken;

struct StackToken {
    const Token* token;
    bool unary;
};

struct Value {
    Complex value{};
    std::string_view assignable_variable{};
};

EvaluationResult error_at(const Token& token)
{
    return {{}, token.part, token.position, true};
}

bool is_supported_function(const std::string_view name)
{
    return name == "abs" || name == "sqrt" || name == "conj"
        || name == "cos" || name == "sin" || name == "tan"
        || name == "acos" || name == "asin" || name == "atan"
        || name == "cosh" || name == "sinh" || name == "tanh"
        || name == "acosh" || name == "asinh" || name == "atanh"
        || name == "exp" || name == "ln" || name == "log";
}

bool apply_function(const std::string_view name, Complex& value)
{
    if(name == "abs") value = std::abs(value);
    else if(name == "sqrt") value = std::sqrt(value);
    else if(name == "conj") value = std::conj(value);
    else if(name == "cos") value = std::cos(value);
    else if(name == "sin") value = std::sin(value);
    else if(name == "tan") value = std::tan(value);
    else if(name == "acos") value = std::acos(value);
    else if(name == "asin") value = std::asin(value);
    else if(name == "atan") value = std::atan(value);
    else if(name == "cosh") value = std::cosh(value);
    else if(name == "sinh") value = std::sinh(value);
    else if(name == "tanh") value = std::tanh(value);
    else if(name == "acosh") value = std::acosh(value);
    else if(name == "asinh") value = std::asinh(value);
    else if(name == "atanh") value = std::atanh(value);
    else if(name == "exp") value = std::exp(value);
    else if(name == "ln") value = std::log(value);
    else if(name == "log") value = std::log10(value);
    else return false;

    return true;
}

bool parse_number(const std::string_view text, Complex& value)
{
    if(text.empty()) return false;

    const std::string input(text);
    char* end = nullptr;
    errno = 0;
    const double parsed = std::strtod(input.c_str(), &end);
    if(errno == ERANGE || end != input.c_str() + input.size()) return false;

    value = parsed;
    return true;
}

int precedence(const StackToken& token)
{
    if(token.unary) return 3;

    switch(token.token->value.front())
    {
        case '^': return 4;
        case '*':
        case '/': return 2;
        case '+':
        case '-': return 1;
        case '>': return 0;
        default: return -1;
    }
}

bool is_right_associative(const StackToken& token)
{
    return token.unary || token.token->value == "^";
}

bool is_known_operator(const std::string_view value)
{
    return value == "+" || value == "-" || value == "*"
        || value == "/" || value == "^" || value == ">";
}

} // namespace

EvaluationResult evaluate_tokens(
    const std::vector<EvaluationToken>& tokens,
    std::map<std::string, std::complex<double>>& variables
)
{
    if(tokens.empty()) return {};

    std::vector<StackToken> postfix;
    std::vector<StackToken> operator_stack;
    bool expect_operand = true;

    for(size_t i = 0; i < tokens.size(); ++i)
    {
        const Token& token = tokens[i];
        if(token.type == Token::Type::Number || token.type == Token::Type::Variable)
        {
            if(!expect_operand || token.value.empty()) return error_at(token);
            postfix.push_back({&token, false});
            expect_operand = false;
        }
        else if(token.type == Token::Type::Function)
        {
            if(!expect_operand || !is_supported_function(token.value)
                || i + 1 >= tokens.size() || tokens[i + 1].type != Token::Type::ParenOpen)
            {
                return error_at(token);
            }
            operator_stack.push_back({&token, false});
        }
        else if(token.type == Token::Type::ParenOpen)
        {
            if(!expect_operand) return error_at(token);
            operator_stack.push_back({&token, false});
        }
        else if(token.type == Token::Type::ParenClose)
        {
            if(expect_operand) return error_at(token);

            while(!operator_stack.empty()
                && operator_stack.back().token->type != Token::Type::ParenOpen)
            {
                postfix.push_back(operator_stack.back());
                operator_stack.pop_back();
            }
            if(operator_stack.empty()) return error_at(token);
            operator_stack.pop_back();

            if(!operator_stack.empty()
                && operator_stack.back().token->type == Token::Type::Function)
            {
                postfix.push_back(operator_stack.back());
                operator_stack.pop_back();
            }
            expect_operand = false;
        }
        else if(token.type == Token::Type::Operator)
        {
            if(!is_known_operator(token.value)) return error_at(token);

            const bool unary = expect_operand && (token.value == "+" || token.value == "-");
            if(expect_operand && !unary) return error_at(token);

            const StackToken current{&token, unary};
            if(unary)
            {
                operator_stack.push_back(current);
                continue;
            }

            while(!operator_stack.empty())
            {
                const StackToken top = operator_stack.back();
                if(top.token->type == Token::Type::ParenOpen) break;
                if(top.token->type == Token::Type::Function
                    || precedence(top) > precedence(current)
                    || (precedence(top) == precedence(current) && !is_right_associative(current)))
                {
                    postfix.push_back(top);
                    operator_stack.pop_back();
                }
                else
                {
                    break;
                }
            }
            operator_stack.push_back(current);
            expect_operand = true;
        }
    }

    if(expect_operand) return error_at(tokens.back());

    while(!operator_stack.empty())
    {
        if(operator_stack.back().token->type == Token::Type::ParenOpen)
            return error_at(*operator_stack.back().token);
        postfix.push_back(operator_stack.back());
        operator_stack.pop_back();
    }

    auto working_variables = variables;
    std::vector<Value> value_stack;
    for(const StackToken& stack_token : postfix)
    {
        const Token& token = *stack_token.token;
        if(token.type == Token::Type::Number)
        {
            Complex value;
            if(!parse_number(token.value, value)) return error_at(token);
            value_stack.push_back({value, {}});
        }
        else if(token.type == Token::Type::Variable)
        {
            if(token.value == "P") value_stack.push_back({std::acos(-1.0), {}});
            else if(token.value == "e") value_stack.push_back({std::exp(1.0), {}});
            else if(token.value == "i") value_stack.push_back({Complex(0.0, 1.0), {}});
            else
            {
                const auto found = working_variables.find(std::string(token.value));
                const Complex value = found == working_variables.end() ? Complex{} : found->second;
                value_stack.push_back({value, token.value});
            }
        }
        else if(token.type == Token::Type::Function)
        {
            if(value_stack.empty()) return error_at(token);
            Complex value = value_stack.back().value;
            if(!apply_function(token.value, value)) return error_at(token);
            value_stack.back() = {value, {}};
        }
        else if(token.type == Token::Type::Operator)
        {
            if(stack_token.unary)
            {
                if(value_stack.empty()) return error_at(token);
                if(token.value == "-") value_stack.back().value = -value_stack.back().value;
                if(value_stack.back().value.imag() == 0.0)
                    value_stack.back().value.imag(0.0);
                value_stack.back().assignable_variable = {};
                continue;
            }

            if(value_stack.size() < 2) return error_at(token);
            const Value right = value_stack.back();
            value_stack.pop_back();
            const Value left = value_stack.back();
            value_stack.pop_back();

            if(token.value == "+") value_stack.push_back({left.value + right.value, {}});
            else if(token.value == "-") value_stack.push_back({left.value - right.value, {}});
            else if(token.value == "*") value_stack.push_back({left.value * right.value, {}});
            else if(token.value == "/") value_stack.push_back({left.value / right.value, {}});
            else if(token.value == "^") value_stack.push_back({std::pow(left.value, right.value), {}});
            else if(token.value == ">")
            {
                if(right.assignable_variable.empty()) return error_at(token);
                working_variables.insert_or_assign(std::string(right.assignable_variable), left.value);
                value_stack.push_back({left.value, {}});
            }
        }
    }

    if(value_stack.size() != 1) return error_at(tokens.back());

    variables = std::move(working_variables);
    return {value_stack.back().value, -1, 0, false};
}
