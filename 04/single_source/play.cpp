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
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <memory>

enum class TokenKind {Keyword, Identifier, StringLiteral, IntegerLiteral, DecimalLiteral, NullLiteral, BooleanLiteral, Seperator, Operator, Eof};

std::unordered_map<TokenKind, std::string> tokenToString {
    {TokenKind::Keyword, "Keyword"},
    {TokenKind::Identifier, "Identifier"},
    {TokenKind::StringLiteral, "StringLiteral"},
    {TokenKind::IntegerLiteral, "IntegerLiteral"},
    {TokenKind::DecimalLiteral, "DecimalLiteral"},
    {TokenKind::NullLiteral, "NullLiteral"},
    {TokenKind::BooleanLiteral, "BooleanLiteral"},
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

class CharStream{
public:
    std::string data;
    uint32_t pos = 0;
    uint32_t line = 1;
	uint32_t col = 0;


    CharStream(const std::string& data): data(data) {}
    char peek() {
        return this->data[this->pos];
    }
    char next() {
        char ch = this->data[this->pos++];
        if(ch == '\n') {
            this->line ++;
            this->col = 0;
        }else {
            this->col ++;
        }
        return ch;
    }
    bool eof() {
        return this->peek() == '\0';
    }
};

class Scanner {
private:
    std::list<Token> tokens;
    CharStream& stream;

    std::set<std::string> KeyWords
        {
            "function", "class",     "break",       "delete",    "return",
            "case",      "do",        "if",          "switch",    "var",
            "catch",     "else",      "in",          "this",      "void",
            "continue",  "false",     "instanceof",  "throw",     "while",
            "debugger",  "finally",   "new",         "true",      "with",
            "default",   "for",       "null",        "try",       "typeof",
            //下面这些用于严格模式
            "implements","let",       "private",     "public",    "yield",
            "interface", "package",   "protected",   "static"
        };

public:
    Scanner(CharStream& stream) : stream(stream){}
    Token next() {
        if (this->tokens.empty()) {
            return this->getAToken();
        } else {
            auto t = this->tokens.front();
            this->tokens.pop_front();
            return t;
        }
    }

    Token peek() {
        if (this->tokens.empty()) {
            auto t = this->getAToken();
            this->tokens.push_back(t);
            return t;
        } else {
            auto t = this->tokens.front();
            return t;
        }
    }

    Token peek2() {
        while (this->tokens.size() < 2) {
            auto t = this->getAToken();
            this->tokens.push_back(t);
        }

        if (this->tokens.size() < 2) {
            return Token{TokenKind::Eof, text:""};
        }

        auto it = this->tokens.begin();
        std::advance(it, 1);
        return *it;
    }


private:
    //从字符串流中获取一个新Token。
    Token getAToken() {
        this->skipWhiteSpaces();
        if (this->stream.eof()){
            return {TokenKind::Eof,  ""};
        }
        else{
            auto ch = this->stream.peek();
            if (this->isLetter(ch) || this->isDigit(ch)){
                return this->parseIdentifer();
            }
            else if (ch == '"'){
                return this->parseStringLiteral();
            }
            else if (ch == '(' || ch == ')' || ch == '{' ||
                     ch == '}' || ch == ';' || ch == ','){
                this->stream.next();
                return {TokenKind::Seperator, std::string(1, ch)};
            }
            else if (ch == '/'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '*'){
                    this->skipMultipleLineComments();
                    return this->getAToken();
                }
                else if (ch1 == '/'){
                    this->skipSingleLineComment();
                    return this->getAToken();
                }
                else if (ch1 == '='){
                    this->stream.next();
                    return {TokenKind::Operator, "/="};
                }
                else{
                    return {TokenKind::Operator, "/"};
                }
            }
            else if (ch == '+'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '+'){
                    this->stream.next();
                    return {TokenKind::Operator, "++"};
                }else if (ch1 == '='){
                    this->stream.next();
                    return {TokenKind::Operator, "+="};
                }
                else{
                    return {TokenKind::Operator, "+"};
                }
            }
            else if (ch == '-'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '-'){
                    this->stream.next();
                    return {TokenKind::Operator, "--"};
                }else if (ch1 == '='){
                    this->stream.next();
                    return {TokenKind::Operator, "-="};
                }
                else{
                    return {TokenKind::Operator, "-"};
                }
            }
            else if (ch == '*'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    return {TokenKind::Operator, "*="};
                }
                else{
                    return {TokenKind::Operator, "*"};
                }
            }
            else{
                //暂时去掉不能识别的字符
                std::cout << ("Unrecognized pattern meeting ': " + std::to_string(ch) +"', at" + toString(this->stream.line) + " col: " + toString(this->stream.col)) << std::endl;
                this->stream.next();
                return this->getAToken();
            }
        }
    }

    void skipSingleLineComment(){
        //跳过第二个/，第一个之前已经跳过去了。
        this->stream.next();

        //往后一直找到回车或者eof
        while(this->stream.peek() !='\n' && !this->stream.eof()){
            this->stream.next();
        }
    }

    void skipMultipleLineComments(){
        //跳过*，/之前已经跳过去了。
        this->stream.next();

        if (!this->stream.eof()){
            auto ch1 = this->stream.next();
            //往后一直找到回车或者eof
            while(!this->stream.eof()){
                auto ch2 = this->stream.next();
                if (ch1 == '*' && ch2 == '/'){
                    return;
                }
                ch1 = ch2;
            }
        }

        //如果没有匹配上，报错。
        std::cout << ("Failed to find matching */ for multiple line comments at ': " + toString(this->stream.line) + " col: " + toString(this->stream.col)) << std::endl;
    }

    void skipWhiteSpaces(){
        while (this->isWhiteSpace(this->stream.peek())){
            this->stream.next();
        }
    }

    Token parseStringLiteral() {
        Token token {TokenKind::StringLiteral, ""};

        //第一个字符不用判断，因为在调用者那里已经判断过了
        this->stream.next();

        while(!this->stream.eof() && this->stream.peek() !='"'){
            token.text.push_back(this->stream.next());
        }

        if(this->stream.peek()=='"'){
            //消化掉字符换末尾的引号
            this->stream.next();
        }
        else{
            std::cout << ("Expecting an \" at line: " + toString(this->stream.line) + " col: " + toString(this->stream.col)) << std::endl;
        }

        return token;
    }

    Token parseIdentifer() {
        Token token = {TokenKind::Identifier,  ""};

        //第一个字符不用判断，因为在调用者那里已经判断过了
        token.text.push_back(this->stream.next());

        //读入后序字符
        while(!this->stream.eof() &&
                this->isLetterDigitOrUnderScore(this->stream.peek())){
            token.text.push_back(this->stream.next());
        }

        //识别出关键字
        if (token.text == "function"){
            token.kind = TokenKind::Keyword;
        }

        return token;
    }

    bool isLetterDigitOrUnderScore(char ch) {
        return (ch>='A' && ch<='Z' ||
                ch>='a' && ch<='z' ||
                ch>='0' && ch<='9' ||
                ch== '_');
    }

    bool isLetter(char ch) {
        return (ch>='A' && ch <='Z' || ch>= 'a' && ch <='z');
    }

    bool isDigit(char ch) {
        return (ch>='0' && ch <='9');
    }

    bool isWhiteSpace(char ch){
        return (ch == ' ' || ch == '\n' || ch== '\t');
    }
};


void compileAndRun(const std::string& program) {

    {
        CharStream charStream(program);
        Scanner tokenizer(charStream);
        while(tokenizer.peek().kind!=TokenKind::Eof){

            auto peek = tokenizer.peek();
            auto peek2 = tokenizer.peek2();
            auto next = tokenizer.next();
            std::cout << "peek:" << peek << std::endl;
            std::cout << "peek2:" << peek2 << std::endl;
            //std::cout << "next:" << next << std::endl;
        }
    }

/*
    std::cout << "program start use tokens" << std::endl;
    CharStream charStream(program);
    Tokenizer tokenizer(charStream);

    std::cout << "ast after parser: " << std::endl;
    auto prog = Parser(tokenizer).parseProg();
    prog->dump("");

    std::cout << "ast after resolved: " << std::endl;
    RefResolver().visitProg(prog);
    prog->dump("");

    std::cout << "run prog: " << std::endl;
    Intepretor().visitProg(prog);
*/
}


static std::string ReadFile(const std::string& filename) {
    std::ifstream ifile(filename.c_str());
    if (!ifile.is_open()) {
        std::cout << "Open file: [" << filename << "] failed." << std::endl;
        return "";
    }

    std::ostringstream buf;
    char ch;
    while (buf && ifile.get(ch)) {
        buf.put(ch);
    }
    ifile.close();
    return buf.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
         std::cout << (std::string("Usage: ") + argv[0] + " FILENAME");
         return 0;
    }

    std::string program = ReadFile(argv[1]);
    std::cout << ("source code:") << std::endl;
    std::cout << (program) << std::endl;

    compileAndRun(program);

    return 0;
}