#ifndef __SCANER_H_
#define __SCANER_H_

#include "error.h"

#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <any>
#include <unordered_map>
#include <list>


enum class TokenKind {Keyword, Identifier, StringLiteral, IntegerLiteral, DecimalLiteral, Seperator, Operator, Eof};

enum class KeywordKind {
    Function,
    Class,
    Break,
    Delete,
    Return,
    Case,
    Do,
    If,
    Switch,
    Var,
    Catch,
    Else,
    In,
    This,
    Void,
    Continue,
    False,
    Instanceof,
    Throw,
    While ,
    Debugger,
    Finally,
    New,
    True,
    With,
    Default,
    For,
    Null,
    Try,
    Typeof,
    Implements,
    Let,
    Private,
    Public,
    Yield,
    Interface,
    Package,
    Protected,
    Static,
    Number,
    String,
    Boolean,
    Any,
    Symbol,
    Undefined
};

std::unordered_map<TokenKind, std::string> tokenToString {
    {TokenKind::Keyword, "Keyword"},
    {TokenKind::Identifier, "Identifier"},
    {TokenKind::StringLiteral, "StringLiteral"},
    {TokenKind::IntegerLiteral, "IntegerLiteral"},
    {TokenKind::DecimalLiteral, "DecimalLiteral"},
    {TokenKind::Seperator, "Seperator"},
    {TokenKind::Operator, "Operator"},
    {TokenKind::Eof, "Eof"},
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


struct Token{
    TokenKind kind;
    std::string text;
    Position pos;
    std::any code;
    Token(TokenKind kind, const std::string& text, const Position& pos, std::any code = std::any()):
        kind(kind), text(text), pos(pos), code(code){
    }
    std::string toString(){
        return std::string("Token") + "@" + this->pos.toString() + "\t" + ::toString(this->kind) + " \t'" + this->text +"'";
    }
};

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

    Position getPosition() {
        return Position(this->pos+1, this->pos+1, this->line, this->col);
    }
};

class Scanner {
private:
    //采用一个array，能预存多个Token，从而支持预读多个Token
    std::list<Token> tokens;
    CharStream& stream;

    Position lastPos {0,0,0,0};
    static std::unordered_map<std::string, KeywordKind> KeyWords;

public:
    Scanner(CharStream& stream) : stream(stream){}
    Token next() {
        if (this->tokens.empty()) {
            auto t = this->getAToken();
            this->lastPos = t.pos;
            return t;
        } else {
            auto t = this->tokens.front();
            this->lastPos = t.pos;
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
            return Token{TokenKind::Eof, text:"EOF", Position()};
        }

        auto it = this->tokens.begin();
        std::advance(it, 1);
        return *it;
    }

    //获取接下来的Token的位置
    Position getNextPos() {
        return this->peek().pos;
    }

    //获取前一个Token的position
    Position getLastPos() {
        return this->lastPos;
    }


private:
    //从字符串流中获取一个新Token。
    Token getAToken() {
        this->skipWhiteSpaces();
        auto pos = this->stream.getPosition();

        return {TokenKind::Eof,  "EOF", pos};
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





    bool isLetterDigitOrUnderScore(char ch) {
        return ((ch>='A' && ch<='Z') ||
                (ch>='a' && ch<='z') ||
                (ch>='0' && ch<='9') ||
                (ch== '_'));
    }

    bool isLetter(char ch) {
        return ((ch>='A' && ch <='Z') || (ch>= 'a' && ch <='z'));
    }

    bool isDigit(char ch) {
        return (ch>='0' && ch <='9');
    }

    bool isWhiteSpace(char ch){
        return (ch == ' ' || ch == '\n' || ch== '\t');
    }
};

std::unordered_map<std::string, KeywordKind> Scanner::KeyWords {
    {"function",    KeywordKind::Function},
    {"class",       KeywordKind::Class},
    {"break",       KeywordKind::Break},
    {"delete",      KeywordKind::Delete},
    {"return",      KeywordKind::Return},
    {"case",        KeywordKind::Case},
    {"do",          KeywordKind::Do},
    {"if",          KeywordKind::If},
    {"switch",      KeywordKind::Switch},
    {"var",         KeywordKind::Var},
    {"catch",       KeywordKind::Catch},
    {"else",        KeywordKind::Else},
    {"in",          KeywordKind::In},
    {"this",        KeywordKind::This},
    {"void",        KeywordKind::Void},
    {"continue",    KeywordKind::Continue},
    {"false",       KeywordKind::False},
    {"instanceof",  KeywordKind::Instanceof},
    {"throw",       KeywordKind::Throw},
    {"while",       KeywordKind::While },
    {"debugger",    KeywordKind::Debugger},
    {"finally",     KeywordKind::Finally},
    {"new",         KeywordKind::New},
    {"true",        KeywordKind::True},
    {"with",        KeywordKind::With},
    {"default",     KeywordKind::Default},
    {"for",         KeywordKind::For},
    {"null",        KeywordKind::Null},
    {"try",         KeywordKind::Try},
    {"typeof",      KeywordKind::Typeof},
    //下面这些用于严格模式
    {"implements",  KeywordKind::Implements},
    {"let",         KeywordKind::Let},
    {"private",     KeywordKind::Private},
    {"public",      KeywordKind::Public},
    {"yield",       KeywordKind::Yield},
    {"interface",   KeywordKind::Interface},
    {"package",     KeywordKind::Package},
    {"protected",   KeywordKind::Protected},
    {"static",      KeywordKind::Static},
    //类型
    //{"number",      KeywordKind::Number},
    {"string",      KeywordKind::String},
    {"boolean",     KeywordKind::Boolean},
    {"any",         KeywordKind::Any},
    {"symbol",      KeywordKind::Symbol},
    //值
    {"undefined",      KeywordKind::Undefined},
};

#endif
