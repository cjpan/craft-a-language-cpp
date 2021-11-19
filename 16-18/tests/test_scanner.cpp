#include "scanner.h"
#include "dbg.h"

#include <gtest/gtest.h>
#include <any>
#include <string>
#include <utility>
#include <vector>

TEST(Scanner, Token_default)
{
    Position ps;
    EXPECT_STREQ("(ln: 1, col: 1, pos: 1)", ps.toString().c_str());

    std::any any;
    Token t(TokenKind::Keyword, "let", ps, any);
    EXPECT_STREQ(t.toString().c_str(),
        "Token@(ln: 1, col: 1, pos: 1)\tKeyword \t'let'");
    //dbg(t.toString());
}

TEST(CharStream, CharStream_default)
{
    std::string data = "let a = 10;";
    CharStream charStream(data);
    auto pos = charStream.getPosition();
    EXPECT_STREQ(pos.toString().c_str(),
        "(ln: 1, col: 0, pos: 1)");
    //dbg(pos.toString());
}

void ScannerTestHelp(const std::string& program,
    const std::vector<std::pair<TokenKind, std::string>>& expectKind) {
    std::vector<Token> actual;

    CharStream charStream(program);
    Scanner tokenizer(charStream);
    while(tokenizer.peek().kind!=TokenKind::Eof){

        // auto peek = tokenizer.peek();
        // auto peek2 = tokenizer.peek2();
        auto next = tokenizer.next();
        // std::cout << "peek:" << peek << std::endl;
        // std::cout << "peek2:" << peek2 << std::endl;
        actual.push_back(next);
        // dbg(next.toString());
    }

    EXPECT_EQ(actual.size(), expectKind.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i].kind, expectKind[i].first);
        EXPECT_STREQ(actual[i].text.c_str(), expectKind[i].second.c_str());
    }
}


TEST(Scanner, Scanner_default)
{
    std::vector<std::pair<TokenKind, std::string>> expectKind {
        {TokenKind::Keyword,         "let"},
        {TokenKind::Identifier,      "i"},
        {TokenKind::Operator,        "="},
        {TokenKind::IntegerLiteral,  "100"},
        {TokenKind::Seperator,       ";"},
    };
    std::string program = "let i = 100;";

    ScannerTestHelp(program, expectKind);
}

TEST(Scanner, Scanner_basic)
{
    std::vector<std::pair<TokenKind, std::string>> expectKind {
        {TokenKind::Keyword       ,  "function"  },
        {TokenKind::Identifier    ,  "circleArea"},
        {TokenKind::Seperator     ,  "("         },
        {TokenKind::Identifier    ,  "r"         },
        {TokenKind::Seperator     ,  ":"         },
        {TokenKind::Identifier    ,  "number"    },
        {TokenKind::Seperator     ,  ")"         },
        {TokenKind::Seperator     ,  ":"         },
        {TokenKind::Identifier    ,  "number"    },
        {TokenKind::Seperator     ,  "{"         },
        {TokenKind::Keyword       ,  "let"       },
        {TokenKind::Identifier    ,  "area"      },
        {TokenKind::Seperator     ,  ":"         },
        {TokenKind::Identifier    ,  "number"    },
        {TokenKind::Operator      ,  "="         },
        {TokenKind::DecimalLiteral,  "3.14"      },
        {TokenKind::Operator      ,  "*"         },
        {TokenKind::Identifier    ,  "r"         },
        {TokenKind::Operator      ,  "*"         },
        {TokenKind::Identifier    ,  "r"         },
        {TokenKind::Seperator     ,  ";"         },
        {TokenKind::Keyword       ,  "return"    },
        {TokenKind::Identifier    ,  "area"      },
        {TokenKind::Seperator     ,  ";"         },
        {TokenKind::Seperator     ,  "}"         },
        {TokenKind::Keyword       ,  "let"       },
        {TokenKind::Identifier    ,  "r"         },
        {TokenKind::Seperator     ,  ":"         },
        {TokenKind::Identifier    ,  "number"    },
        {TokenKind::Operator      ,  "="         },
        {TokenKind::IntegerLiteral,  "4"         },
        {TokenKind::Seperator     ,  ";"         },
        {TokenKind::Identifier    ,  "println"   },
        {TokenKind::Seperator     ,  "("         },
        {TokenKind::StringLiteral ,  "r="        },
        {TokenKind::Operator      ,  "+"         },
        {TokenKind::Identifier    ,  "r"         },
        {TokenKind::Operator      ,  "+"         },
        {TokenKind::StringLiteral ,  ", area="   },
        {TokenKind::Operator      ,  "+"         },
        {TokenKind::Identifier    ,  "circleArea"},
        {TokenKind::Seperator     ,  "("         },
        {TokenKind::Identifier    ,  "r"         },
        {TokenKind::Seperator     ,  ")"         },
        {TokenKind::Seperator     ,  ")"         },
        {TokenKind::Seperator     ,  ";"         },
        {TokenKind::Identifier    ,  "r"         },
        {TokenKind::Operator      ,  "="         },
        {TokenKind::IntegerLiteral,  "5"         },
        {TokenKind::Seperator     ,  ";"         },
        {TokenKind::Identifier    ,  "println"   },
        {TokenKind::Seperator     ,  "("         },
        {TokenKind::StringLiteral ,  "r="        },
        {TokenKind::Operator      ,  "+"         },
        {TokenKind::Identifier    ,  "r"         },
        {TokenKind::Operator      ,  "+"         },
        {TokenKind::StringLiteral ,  ", area="   },
        {TokenKind::Operator      ,  "+"         },
        {TokenKind::Identifier    ,  "circleArea"},
        {TokenKind::Seperator     ,  "("         },
        {TokenKind::Identifier    ,  "r"         },
        {TokenKind::Seperator     ,  ")"         },
        {TokenKind::Seperator     ,  ")"         },
        {TokenKind::Seperator     ,  ";"         },
    };


    std::string program = R"(
        function circleArea(r : number):number{
            let area : number = 3.14*r*r;
            return area;
        }

        let r:number =4;
        println("r=" + r +", area="+circleArea(r));
        r = 5;
        println("r=" + r +", area="+circleArea(r));
    )";

    ScannerTestHelp(program, expectKind);
}

