#include "interpretor.h"
#include "semantic.h"
#include "parser.h"

#include "dbg.h"

#include <gtest/gtest.h>

TEST(Interpretor, Interpretor_binary_function_plus)
{
    dbg("Interpretor_basic");
    std::any v1 = 2;
    std::any v2 = 2;

    Interpretor interpretor;
    auto func = Interpretor::GetBinaryFunction(Op::Plus, v1, v2);
    EXPECT_TRUE(func != std::nullopt);

    auto retVal = func.value()(v1, v2);
    EXPECT_TRUE(retVal.has_value());

    auto val = std::any_cast<int32_t>(retVal);
    EXPECT_TRUE(val == 4);
}

TEST(Interpretor, Interpretor_binary_function_minus)
{
    dbg("Interpretor_basic");
    std::any v1 = 2;
    std::any v2 = 2;

    Interpretor interpretor;
    auto func = Interpretor::GetBinaryFunction(Op::Minus, v1, v2);
    EXPECT_TRUE(func != std::nullopt);

    auto retVal = func.value()(v1, v2);
    EXPECT_TRUE(retVal.has_value());

    auto val = std::any_cast<int32_t>(retVal);
    EXPECT_TRUE(val == 0);
}

TEST(Interpretor, Interpretor_basic)
{
    std::string expect =
R"(Prog
    VariableStatement
        VariableDecl i(any)
            100(integer)
    ExpressionStatement
        FunctionCall println, built-in
            Variable: i, resolved
)";

    std::string program = R"(
let i = 100;
println(i);
)";
    CharStream charStream(program);
    Scanner scanner(charStream);

    auto parser = Parser(scanner);
    auto ast = parser.parseProg();

    auto dumper = AstDumper();
    dumper.visit(*ast, "");

    SemanticAnalyer semanticAnalyer;
    semanticAnalyer.execute(*ast);

    dumper.clearString();
    dumper.visit(*ast, "");
    auto str = dumper.toString();
    EXPECT_STREQ(expect.c_str(), str.c_str());

    auto scopeDumper = ScopeDumper();
    scopeDumper.visit(*ast, "");

    auto interpretor = Interpretor();
    interpretor.visit(*ast, "");
}

TEST(Interpretor, Program)
{
    std::string expect =
R"(Prog
    FunctionDecl fourTimes
        Return type: number
            ParamList:
                VariableDecl r(number)
                no initialization.
            VariableStatement
                VariableDecl area(number)
                    Binary:Multiply
                        4(integer)
                        Variable: r, resolved
            ReturnStatement
                Variable: area, resolved
    ExpressionStatement
        FunctionCall println, built-in
            FunctionCall fourTimes, resolved
                4(integer)
)";

    std::string program =
R"(
function fourTimes(r : number):number{
    let area : number = 4*r;
    return area;
}

println(fourTimes(4));
)";

    CharStream charStream(program);
    Scanner scanner(charStream);

    auto parser = Parser(scanner);
    auto ast = parser.parseProg();

    auto dumper = AstDumper();
    dumper.visit(*ast, "");

    SemanticAnalyer semanticAnalyer;
    semanticAnalyer.execute(*ast);

    dumper.clearString();
    dumper.visit(*ast, "");
    auto str = dumper.toString();
    EXPECT_STREQ(expect.c_str(), str.c_str());

    auto scopeDumper = ScopeDumper();
    scopeDumper.visit(*ast, "");

    EXPECT_STREQ(expect.c_str(), str.c_str());

    auto interpretor = Interpretor();
    interpretor.visit(*ast, "");
}