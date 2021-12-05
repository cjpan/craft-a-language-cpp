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
    Print(program, Color::Blue);

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

TEST(ASM, Asm_Multi_VariableDecl)
{
    std::string expect =
R"(Prog
    VariableStatement
        VariableDecl i(any)
            1(integer)
    VariableStatement
        VariableDecl j(any)
            2(integer)
    VariableStatement
        VariableDecl x(any)
            Binary:Plus
                Variable: i, resolved
                Variable: j, resolved
    ReturnStatement
)";

    std::string program = R"(
let i = 1;
let j = 2;
let x = i + j;
)";
    Print(program, Color::Blue);

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

    Print(program, Color::Blue);

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

TEST(ASM, Program)
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
    ReturnStatement
)";

    std::string program =
R"(
function fourTimes(r : number):number{
    let area : number = 4*r;
    return area;
}

println(fourTimes(4));
)";

    Print(program, Color::Blue);

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

    auto asmStr = compileToAsm(*ast);
}