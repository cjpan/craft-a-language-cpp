#include "error.h"

#include <gtest/gtest.h>

TEST(Error, Position_default)
{
    Position ps;
    EXPECT_STREQ("(ln: 1, col: 1, pos: 1)", ps.toString().c_str());
}


