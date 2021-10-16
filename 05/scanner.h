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


enum class TokenKind {Keyword, Identifier, StringLiteral, IntegerLiteral, DecimalLiteral, Seperator, Operator, Eof};

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
    Position& pos;
    std::any code;
    Token(TokenKind kind, const std::string& text, Position& pos, std::any code):
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

#endif
