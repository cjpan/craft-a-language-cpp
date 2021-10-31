#include "parser.h"

#include "dbg.h"

#include <gtest/gtest.h>


TEST(PARSER, variable)
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

TEST(PARSER, function_call)
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

TEST(PARSER, block)
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
