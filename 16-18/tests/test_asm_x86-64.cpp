#include "asm_x86-64.h"
#include "interpretor.h"
#include "semantic.h"
#include "parser.h"
#include "common.h"

#include "dbg.h"

#include <gtest/gtest.h>

TEST(ASM, Asm_VariableDecl)
{
    std::string expect =
R"(Prog
    VariableStatement
        VariableDecl i(any)
            1(integer)
    ReturnStatement
)";

    std::string program = R"(
let i = 1;
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

    auto asmStr = compileToAsm(*ast);
}

TEST(ASM, Asm_FunctionCall)
{
    std::string expect =
R"(Prog
    VariableStatement
        VariableDecl i(any)
            1(integer)
    ExpressionStatement
        FunctionCall println, built-in
            Variable: i, resolved
    ReturnStatement
)";

    std::string program = R"(
let i = 1;
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

    auto asmStr = compileToAsm(*ast);
}