#include "semantic.h"
#include "parser.h"

#include "dbg.h"

#include <gtest/gtest.h>


TEST(Semantic, Semantic_basic)
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
}

TEST(Semantic, Program)
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
}
