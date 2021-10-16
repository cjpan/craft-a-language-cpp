#include "scanner.h"
#include "dbg.h"

#include <gtest/gtest.h>
#include <any>
#include <string>

TEST(Scanner, Token_default)
{
    Position ps;
    EXPECT_STREQ("(ln: 1, col: 1, pos: 1)", ps.toString().c_str());

    std::any any;
    Token t(TokenKind::Keyword, "let", ps, any);
    EXPECT_STREQ(t.toString().c_str(),
        "Token@(ln: 1, col: 1, pos: 1)\tKeyword \t'let'");
    dbg(t.toString());
}

TEST(CharStream, CharStream_default)
{
    std::string data = "let a = 10;";
    CharStream charStream(data);
    auto pos = charStream.getPosition();
    EXPECT_STREQ(pos.toString().c_str(),
        "(ln: 1, col: 0, pos: 1)");
    dbg(pos.toString());
}
