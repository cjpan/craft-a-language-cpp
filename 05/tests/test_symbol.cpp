#include "symbol.h"
#include "dbg.h"

#include <gtest/gtest.h>

TEST(SYMBOL, VarSymbol_default)
{
    VarSymbol  FUN_println_varSymbol{"a", SysTypes::String};
    SymbolDumper dumper;
    dumper.visit(FUN_println_varSymbol, "");
}

TEST(SYMBOL, FUN_println_default)
{
    auto it = built_ins.find("println");
    EXPECT_TRUE(it != built_ins.end());
    EXPECT_TRUE(it->second.getNumParams() == 1);

    SymbolDumper dumper;
    dumper.visit(it->second, "");
}

TEST(SYMBOL, FUN_tick_default)
{
    auto it = built_ins.find("tick");
    EXPECT_TRUE(it != built_ins.end());
    EXPECT_TRUE(it->second.getNumParams() == 0);

    SymbolDumper dumper;
    dumper.visit(it->second, "");
}

TEST(SYMBOL, FUN_integer_to_string_default)
{
    auto it = built_ins.find("integer_to_string");
    EXPECT_TRUE(it != built_ins.end());
    EXPECT_TRUE(it->second.getNumParams() == 1);

    SymbolDumper dumper;
    dumper.visit(it->second, "");
}