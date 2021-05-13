#include "Halide.h"

#include "g2_generator.h"

namespace Halide {
namespace Testing {

// TODO: buffers. imageparams? outputbuffers?

Func g2_func_impl(Func input, Expr offset, int scaling) {
    Var x, y;

    Func output;
    output(x, y) = input(x, y) * scaling + offset;
    output.compute_root();

    return output;
}

const auto g2_lambda_impl = [](Func input, Expr offset, int scaling, Type ignored_type, bool ignored_bool, std::string ignored_string) {
    Var x, y;

    std::cout << "Ignoring type: " << ignored_type << "\n";
    std::cout << "Ignoring bool: " << (int)ignored_bool << "\n";
    std::cout << "Ignoring string: " << ignored_string << "\n";

    Func output;
    output(x, y) = input(x, y) * scaling + offset;
    output.compute_root();

    return output;
};

}  // namespace Testing
}  // namespace Halide

using namespace Halide;
using namespace Halide::Internal;

RegisterGenerator register_1(
    "g2",
    [](const GeneratorContext &context) -> std::unique_ptr<AbstractGenerator> {
        FnBinder d{
            Halide::Testing::g2_func_impl,
            {
                FnBinder::Input("input", Int(32), 2),
                FnBinder::Input("offset", Int(32)),
                FnBinder::Constant("scaling", "2"),
            },
            FnBinder::Output("output", Int(32), 2),
        };
        return G2GeneratorFactory("g2", std::move(d))(context);
    });

RegisterGenerator register_2(
    "g2_lambda",
    [](const GeneratorContext &context) -> std::unique_ptr<AbstractGenerator> {
        FnBinder d{
            Halide::Testing::g2_lambda_impl,
            {
                FnBinder::Input("input", Int(32), 2),
                FnBinder::Input("offset", Int(32)),
                FnBinder::Constant("scaling", "2"),
                FnBinder::Constant("ignored_type", "int32"),
                FnBinder::Constant("ignored_bool", "false"),
                FnBinder::Constant("ignored_string", "qwerty"),
            },
            FnBinder::Output("output", Int(32), 2),
        };
        return G2GeneratorFactory("g2_lambda", std::move(d))(context);
    });
