#include "scope.h"
#include "ast.h"
#include "parser.h"
#include "dbg.h"

#include <gtest/gtest.h>


TEST(SCOPE, Scope_default)
{
    std::string name = "a";

    Scope scope(nullptr);
    auto varType = SysTypes::String();
    std::shared_ptr<Symbol> varSymbol = std::make_shared<VarSymbol>(name, varType);

    scope.enter(name, varSymbol);
    bool has = scope.hasSymbol(name);
    EXPECT_TRUE(has);

    auto sym = scope.getSymbol(name);
    EXPECT_TRUE(sym != nullptr);

    sym = scope.getSymbolCascade(name);
    EXPECT_TRUE(sym != nullptr);
}

TEST(SCOPE, ScopeDumper_default)
{
    std::string program = "let i = 100;";
    CharStream charStream(program);
    Scanner scanner(charStream);

    auto parser = Parser(scanner);
    auto ast = parser.parseProg();

    auto astDumper = AstDumper();
    astDumper.visit(*ast, "");

    auto scopeDumper = ScopeDumper();
    scopeDumper.visit(*ast, "");
}
