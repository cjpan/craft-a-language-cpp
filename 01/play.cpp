// MIT License

// Copyright (c) 2021 caofeixiang_hw

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

using namespace std;

enum class TokenKind {Keyword, Identifier, StringLiteral, Seperator, Operator, Eof};
std::unordered_map<TokenKind, std::string> tokenToString {
    {TokenKind::Keyword, "Keyword"},
    {TokenKind::Identifier, "Identifier"},
    {TokenKind::StringLiteral, "StringLiteral"},
    {TokenKind::Seperator, "Seperator"},
    {TokenKind::Operator, "Operator"},
    {TokenKind::Eof, "Eof"},
};

struct Token{
    TokenKind kind;
    std::string text;
};

inline std::string toString(uint32_t obj) {
    return std::to_string(obj);
}

inline std::string toString(TokenKind kind) {
    auto it = tokenToString.find(kind);
    if (it != tokenToString.end()) {
        return it->second;
    }

    return "Unknow";
}

ostream& operator<<(ostream& out, Token& token) {
    out << "{ " << toString(token.kind) << " , " << token.text << " }";
    return out;
}

vector<Token> tokenArray = {
    {TokenKind::Keyword,      "function"},
    {TokenKind::Identifier,   "sayHello"},
    {TokenKind::Seperator,    "("},
    {TokenKind::Seperator,    ")"},
    {TokenKind::Seperator,    "{"},
    {TokenKind::Identifier,   "println"},
    {TokenKind::Seperator,    "("},
    {TokenKind::StringLiteral,"Hello World!"},
    {TokenKind::Seperator,    ")"},
    {TokenKind::Seperator,    ";"},
    {TokenKind::Seperator,    "}"},
    {TokenKind::Identifier,   "sayHello"},
    {TokenKind::Seperator,    "("},
    {TokenKind::Seperator,    ")"},
    {TokenKind::Seperator,    ";"},
    {TokenKind::Eof,          ""}
};

class Tokenizer{
private:
    vector<Token> tokens;
    uint32_t pos = 0;

public:
    Tokenizer(const vector<Token>& tokens) {
        this->tokens = tokens;
    }
    Token next() {
        if (this->pos <= this->tokens.size()){
            return this->tokens[this->pos++];
        }
        else{
            //如果已经到了末尾，总是返回EOF
            return this->tokens[this->pos];
        }
    }
    uint32_t position() {
        return this->pos;
    }
    void traceBack(uint32_t newPos){
        this->pos = newPos;
    }
};

class AstNode{
public:
    //打印对象信息，prefix是前面填充的字符串，通常用于缩进显示
    virtual void dump(string prefix) = 0;
};

class Statement: public AstNode{
public:
    bool isStatementNode(const AstNode* node) {
        if (!node) {
            return false;
        }
        else {
            return true;
        }
    }

    void dump(string prefix) override {
        std::cout << (prefix+"Statement") << std::endl;
    }
};

class Prog: public AstNode{
public:
    vector<Statement> stmts; //程序中可以包含多个语句
    Prog(vector<Statement> stmts){
        this->stmts = stmts;
    }
    void dump(string prefix) override {
        std::cout << (prefix+"Prog") << std::endl;
        std::for_each(stmts.begin(), stmts.end(), [prefix](auto& x) { x.dump(prefix+"\t");});
    }
};

int main() {
    //词法分析
    auto tokenizer = new Tokenizer(tokenArray);
    cout << "program start use tokens" << endl;

    for (auto& token: tokenArray) {
        cout << token << endl;
    }

    return 0;
}