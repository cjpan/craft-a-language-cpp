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
#include <memory>

//using namespace std;

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

std::ostream& operator<<(std::ostream& out, Token& token) {
    out << "{ " << toString(token.kind) << " , " << token.text << " }";
    return out;
}

/*
std::vector<Token> tokenArray = {
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
*/

std::vector<Token> tokenArray = {
    {TokenKind::Identifier,   "println"},
    {TokenKind::Seperator,    "("},
    {TokenKind::StringLiteral,"Hello World!"},
    {TokenKind::Seperator,    ")"},
    {TokenKind::Seperator,    ";"},
    {TokenKind::Identifier,   "sayHello"},
    {TokenKind::Seperator,    "("},
    {TokenKind::Seperator,    ")"},
    {TokenKind::Seperator,    ";"},
    {TokenKind::Eof,          ""}
};

class Tokenizer{
private:
    std::vector<Token> tokens;
    uint32_t pos = 0;

public:
    Tokenizer(const std::vector<Token>& tokens) {
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

enum class AstNodeType {AstNode, Statement, Prog, FunctionCall, FunctionBody, FunctionDecl};

class AstNode{
protected:
    AstNodeType type = AstNodeType::AstNode;
public:
    //打印对象信息，prefix是前面填充的字符串，通常用于缩进显示
    virtual void dump(const std::string& prefix) {};
    virtual AstNodeType getType() {return this->type;}
};

class Statement: public AstNode{
public:
    Statement() {
        this->type = AstNodeType::Statement;
    }

    void dump(const std::string& prefix) override {
        std::cout << (prefix+"Statement") << std::endl;
    }
};

class Prog: public AstNode{
public:
    std::vector<std::shared_ptr<Statement>> stmts; //程序中可以包含多个语句
    Prog(std::vector<std::shared_ptr<Statement>> stmts){
        this->stmts = stmts;
        this->type = AstNodeType::Prog;
    }
    void dump(const std::string& prefix) override {
        std::cout << (prefix+"Prog") << std::endl;
        std::for_each(stmts.begin(), stmts.end(), [prefix](auto& x) { x->dump(prefix+"\t");});
    }
};

/**
 * 函数调用
 */
struct FunctionDecl;
class FunctionCall: public Statement{
public:
    std::string name;
    std::vector<std::string> parameters;
    std::shared_ptr<FunctionDecl> definition = nullptr;  //指向函数的声明
    FunctionCall(std::string name, const std::vector<std::string>& parameters){
        this->name = name;
        this->parameters = parameters;
        this->type = AstNodeType::FunctionCall;
    }

    void dump(const std::string& prefix) override {
        std::cout << (prefix+"FunctionCall "+this->name + (this->definition!=nullptr ? ", resolved" : ", not resolved")) << std::endl;
        std::for_each(parameters.begin(), parameters.end(), [prefix](auto& x) {
            std::cout << (prefix+"\t"+"Parameter: "+ x) << std::endl;
        });
    }
};

class FunctionBody: public AstNode{
public:
    std::vector<std::shared_ptr<Statement>> stmts;
    FunctionBody(std::vector<std::shared_ptr<Statement>> stmts){
        this->stmts = stmts;
        this->type = AstNodeType::FunctionBody;
    }

    void dump(const std::string& prefix) override {
        std::cout << (prefix+"FunctionBody") << std::endl;
        std::for_each(stmts.begin(), stmts.end(), [prefix](auto& x) { x->dump(prefix+"\t");});
    }
};

class FunctionDecl: public Statement{
public:
    std::string name;       //函数名称
    std::shared_ptr<FunctionBody> body; //函数体
    FunctionDecl(std::string name, std::shared_ptr<FunctionBody> body){
        this->name = name;
        this->body = body;
        this->type = AstNodeType::FunctionDecl;
    }
    void dump(const std::string& prefix) override {
        std::cout << (prefix+"FunctionDecl "+this->name) << std::endl;
        this->body->dump(prefix+"\t");
    }
};

class Parser{
    Tokenizer& tokenizer;
public:
    Parser(Tokenizer& tokenizer): tokenizer(tokenizer) {}
    /**
     * 解析Prog
     * 语法规则：
     * prog = (functionDecl | functionCall)* ;
     */
    std::shared_ptr<Prog> parseProg(){

        std::vector<std::shared_ptr<Statement>> stmts;
        std::shared_ptr<Statement> stmt;
        while(true){  //每次循环解析一个语句
            //尝试一下函数声明
            stmt = this->parseFunctionDecl();
            if (stmt != nullptr && stmt->getType() == AstNodeType::FunctionDecl){
                stmts.push_back(stmt);
                continue;
            }

            //如果前一个尝试不成功，那么再尝试一下函数调用
            stmt = this->parseFunctionCall();
            if (stmt != nullptr && stmt->getType() == AstNodeType::FunctionCall){
                stmts.push_back(stmt);
                continue;
            }

            //如果都没成功，那就结束
            if (stmt == nullptr){
                break;
            }
        }
        return std::make_shared<Prog>(stmts);
    }

    std::shared_ptr<Statement> parseFunctionDecl() {
        return nullptr;
    }
    std::shared_ptr<Statement> parseFunctionCall() {
        uint32_t oldPos = this->tokenizer.position();
        std::vector<std::string> params;
        auto t = this->tokenizer.next();
        if(t.kind == TokenKind::Identifier){
            auto t1 = this->tokenizer.next();
            if (t1.text == "("){
                auto t2 = this->tokenizer.next();
                //循环，读出所有
                while(t2.text != ")"){
                    if (t2.kind == TokenKind::StringLiteral){
                        params.push_back(t2.text);
                    }
                    else{
                        std::cout<< ("Expecting parameter in FunctionCall, while we got a " + t2.text) << std::endl;
                        return nullptr;  //出错时，就不在错误处回溯了。
                    }
                    t2 = this->tokenizer.next();
                    if (t2.text != ")"){
                        if (t2.text == ","){
                            t2 = this->tokenizer.next();
                        }
                        else{
                            std::cout << ("Expecting a comma in FunctionCall, while we got a " + t2.text) << std::endl;
                            return nullptr;
                        }
                    }
                }
                //消化掉一个分号：;
                t2 = this->tokenizer.next();
                if (t2.text == ";"){
                    return std::make_shared<FunctionCall>(t.text, params);
                }
                else{
                    std::cout << ("Expecting a comma in FunctionCall, while we got a " + t2.text) << std::endl;
                    return nullptr;
                }
            }
        }
        //如果解析不成功，回溯，返回null。
        this->tokenizer.traceBack(oldPos);
        return nullptr;
    }


};

int main() {
    //词法分析
    Tokenizer tokenizer(tokenArray);
    std::cout << "program start use tokens" << std::endl;

    for (auto& token: tokenArray) {
        std::cout << token << std::endl;
    }

    auto prog = Parser(tokenizer).parseProg();
    prog->dump("");

    return 0;
}