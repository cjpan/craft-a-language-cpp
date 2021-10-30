#include "parser.h"

#include "dbg.h"

#include <gtest/gtest.h>


TEST(PARSER, Parser_default)
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
