#include "parser.h"

#include "dbg.h"

#include <gtest/gtest.h>

TEST(Parser, variable)
{
    std::string expect =
R"(Prog
    VariableStatement
        VariableDecl i(any)
            100(integer)
)";

    std::string program = "let i = 100;";
    CharStream charStream(program);
    Scanner scanner(charStream);

    auto parser = Parser(scanner);
    auto ast = parser.parseProg();

    auto dumper = AstDumper();
    dumper.visit(*ast, "");

    auto str = dumper.toString();
    EXPECT_STREQ(expect.c_str(), str.c_str());
}

TEST(Parser, function_call)
{
    std::string expect =
R"(Prog
    ExpressionStatement
        FunctionCall println, built-in
            Variable: r, not resolved
)";

    std::string program = "println(r);";
    CharStream charStream(program);
    Scanner scanner(charStream);

    auto parser = Parser(scanner);
    auto ast = parser.parseProg();

    auto dumper = AstDumper();
    dumper.visit(*ast, "");

    auto str = dumper.toString();
    EXPECT_STREQ(expect.c_str(), str.c_str());
}

TEST(Parser, block)
{
    std::string expect =
R"(Prog
        VariableStatement
            VariableDecl area(number)
                Binary:Multiply
                    4(integer)
                    Variable: r, not resolved
)";

    std::string program =
R"(
{
    let area : number = 4*r;
}
)";

    CharStream charStream(program);
    Scanner scanner(charStream);

    auto parser = Parser(scanner);
    auto ast = parser.parseProg();

    auto dumper = AstDumper();
    dumper.visit(*ast, "");

    auto str = dumper.toString();
    EXPECT_STREQ(expect.c_str(), str.c_str());
}


TEST(Parser, FunctionDecl)
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
                        Variable: r, not resolved
            ReturnStatement
                Variable: area, not resolved
)";

    std::string program =
R"(
function fourTimes(r : number):number{
    let area : number = 4*r;
    return area;
}
)";

    CharStream charStream(program);
    Scanner scanner(charStream);

    auto parser = Parser(scanner);
    auto ast = parser.parseProg();

    auto dumper = AstDumper();
    dumper.visit(*ast, "");

    auto str = dumper.toString();
    EXPECT_STREQ(expect.c_str(), str.c_str());
}

TEST(Parser, Program)
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
                        Variable: r, not resolved
            ReturnStatement
                Variable: area, not resolved
    ExpressionStatement
        FunctionCall println, built-in
            FunctionCall fourTimes, not resolved
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

    auto str = dumper.toString();
    EXPECT_STREQ(expect.c_str(), str.c_str());
}
