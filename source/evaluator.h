#ifndef INC_EVALUATOR_H
#define INC_EVALUATOR_H

#include <complex>
#include <map>
#include <string>
#include <string_view>
#include <vector>

struct EvaluationToken {
    enum class Type {
        Number,
        Variable,
        Function,
        Operator,
        ParenOpen,
        ParenClose,
    };

    std::string_view value;
    int part;
    int position;
    Type type;
};

struct EvaluationResult {
    std::complex<double> value{};
    int error_part{-1};
    int error_position{0};
    bool error{true};
};

EvaluationResult evaluate_tokens(
    const std::vector<EvaluationToken>& tokens,
    std::map<std::string, std::complex<double>>& variables
);

#endif
