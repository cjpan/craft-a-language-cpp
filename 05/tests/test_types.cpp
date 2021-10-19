#include "types.h"
#include "dbg.h"

#include <gtest/gtest.h>
#include <any>
#include <string>
#include <utility>
#include <vector>

TEST(TYPES, Types_default)
{
    SimpleType type("any", {});
    auto str = type.toString();
    const char* expect = "SimpleType {name: any, upperTypes: []}";
    EXPECT_STREQ(expect, str.c_str());
    //dbg(str);
}

TEST(TYPES, SysTypes_basic)
{
    auto number = SysTypes::Number;
    auto flag = SysTypes::isSysType(number);
    EXPECT_EQ(flag, true);

    const char* expect = "SimpleType {name: number, upperTypes: [any, ]}";
    auto str = number.toString();
    EXPECT_STREQ(expect, str.c_str());
}
