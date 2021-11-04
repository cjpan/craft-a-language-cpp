#include "scope.h"
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
