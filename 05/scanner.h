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

enum class Seperator{
    OpenBracket = 0,                //[
    CloseBracket,                   //]
    OpenParen,                      //(
    CloseParen,                     //)
    OpenBrace,                      //{
    CloseBrace,                     //}
    Colon,                          //:
    SemiColon,                      //;
};

enum class Op{
    QuestionMark = 100,             //?   让几个类型的code取值不重叠
    Ellipsis,                       //...
    Dot,                            //.
    Comma,                          //,
    At,                             //@

    RightShiftArithmetic,           //>>
    LeftShiftArithmetic,            //<<
    RightShiftLogical,              //>>>
    IdentityEquals,                 //===
    IdentityNotEquals,              //!==

    BitNot,                         //~
    BitAnd,                         //&
    BitXOr,                         //^
    BitOr,                          //|

    Not,                            //!
    And,                            //&&
    Or,                             //||

    Assign,                         //=
    MultiplyAssign,                 //*=
    DivideAssign,                   ///=
    ModulusAssign,                  //%=
    PlusAssign,                     //+=
    MinusAssign,                    //-=
    LeftShiftArithmeticAssign,      //<<=
    RightShiftArithmeticAssign,     //>>=
    RightShiftLogicalAssign,        //>>>=
    BitAndAssign,                   //&=
    BitXorAssign,                   //^=
    BitOrAssign,                    //|=

    ARROW,                          //=>

    Inc,                            //++
    Dec,                            //--

    Plus,                           //+
    Minus,                          //-
    Multiply,                       //*
    Divide,                         ///
    Modulus,                        //%

    EQ,                             //==
    NE,                             //!=
    G,                              //>
    GE,                             //>=
    L,                              //<
    LE,                             //<=
};

enum class KeywordKind {
    Function = 200,
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

inline std::string toString(uint32_t obj) {
    return std::to_string(obj);
}

std::string toString(TokenKind kind);
std::string toString(Op op);

struct Token{
    TokenKind kind;
    std::string text;
    Position pos;
    std::any code;
    Token(TokenKind kind, const std::string& text, const Position& pos, std::any code = std::any()):
        kind(kind), text(text), pos(pos), code(code){
    }

    Token(TokenKind kind, char c, const Position& pos, std::any code = std::any()):
        kind(kind), text(std::string(1, c)), pos(pos), code(code){
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
    static std::unordered_map<std::string, KeywordKind> KeywordMap;

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
        //跳过所有空白字符
        this->skipWhiteSpaces();
        auto pos = this->stream.getPosition();
        if (this->stream.eof()){
            return Token(TokenKind::Eof,"EOF",pos);
        }
        else{
            auto ch = this->stream.peek();
            if (this->isLetter(ch) || ch == '_'){
                return this->parseIdentifer();
            }
            else if (ch == '"'){
                return this->parseStringLiteral();
            }
            else if (ch == '('){
                this->stream.next();
                return Token(TokenKind::Seperator,ch,pos,Seperator::OpenParen);
            }
            else if (ch == ')'){
                this->stream.next();
                return Token(TokenKind::Seperator,ch,pos,Seperator::CloseParen);
            }
            else if (ch == '{'){
                this->stream.next();
                return Token(TokenKind::Seperator,ch,pos,Seperator::OpenBrace);
            }
            else if (ch == '}'){
                this->stream.next();
                return Token(TokenKind::Seperator,ch,pos,Seperator::CloseBrace);
            }
            else if (ch == '['){
                this->stream.next();
                return Token(TokenKind::Seperator,ch,pos,Seperator::OpenBracket);
            }
            else if (ch == ']'){
                this->stream.next();
                return Token(TokenKind::Seperator,ch,pos,Seperator::CloseBracket);
            }
            else if (ch == ':'){
                this->stream.next();
                return Token(TokenKind::Seperator,ch,pos,Seperator::Colon);
            }
            else if (ch == ';'){
                this->stream.next();
                return Token(TokenKind::Seperator,ch,pos,Seperator::SemiColon);
            }
            else if (ch == ','){
                this->stream.next();
                return Token(TokenKind::Seperator,ch,pos,Op::Comma);
            }
            else if (ch == '?'){
                this->stream.next();
                return Token(TokenKind::Seperator,ch,pos,Op::QuestionMark);
            }
            else if (ch == '@'){
                this->stream.next();
                return Token(TokenKind::Seperator,ch,pos,Op::At);
            }
            //解析数字字面量，语法是：
            // DecimalLiteral: IntegerLiteral '.' [0-9]*
            //   | '.' [0-9]+
            //   | IntegerLiteral
            //   ;
            // IntegerLiteral: '0' | [1-9] [0-9]* ;
            else if (this->isDigit(ch)){
                this->stream.next();
                auto ch1 = this->stream.peek();
                std::string literal = "";
                if(ch == '0'){//暂不支持八进制、二进制、十六进制
                    if (!(ch1>='1' && ch1<='9')){
                        literal += '0';
                    }
                    else {
                        dbg("0 cannot be followed by other digit now, at line: " +
                        ::toString(this->stream.line) + " col: " + toString(this->stream.col));
                        //暂时先跳过去
                        this->stream.next();
                        return this->getAToken();
                    }
                }
                else if(ch>='1' && ch<='9'){
                    literal += ch;
                    while(this->isDigit(ch1)){
                        ch = this->stream.next();
                        literal += ch;
                        ch1 = this->stream.peek();
                    }
                }
                //加上小数点.
                if (ch1 == '.'){
                    //小数字面量
                    literal += '.';
                    this->stream.next();
                    ch1 = this->stream.peek();
                    while(this->isDigit(ch1)){
                        ch = this->stream.next();
                        literal += ch;
                        ch1 = this->stream.peek();
                    }
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::DecimalLiteral, literal, pos);
                }
                else{
                    //返回一个整型字面量
                    return Token(TokenKind::IntegerLiteral, literal, pos);
                }
            }
            else if (ch == '.'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (this->isDigit(ch1)){
                    //小数字面量
                    std::string literal = ".";
                    while(this->isDigit(ch1)){
                        ch = this->stream.next();
                        literal += ch;
                        ch1 = this->stream.peek();
                    }
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::DecimalLiteral, literal, pos);
                }
                //...省略号
                else if (ch1=='.'){
                    this->stream.next();
                    //第三个.
                    ch1 = this->stream.peek();
                    if (ch1 == '.'){
                        pos.end = this->stream.pos+1;
                        return Token(TokenKind::Seperator, "...", pos, Op::Ellipsis);
                    }
                    else{
                        dbg("Unrecognized pattern : .., missed a . ?");
                        return this->getAToken();
                    }
                }
                //.号分隔符
                else{
                    return Token(TokenKind::Operator, ".", pos, Op::Dot);
                }
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
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "/=", pos, Op::DivideAssign);
                }
                else{
                    return Token(TokenKind::Operator, "/", pos, Op::Divide);
                }
            }
            else if (ch == '+'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '+'){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "++", pos, Op::Inc);
                }else if (ch1 == '='){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "+=", pos, Op::PlusAssign);
                }
                else{
                    return Token(TokenKind::Operator, "+", pos, Op::Plus);
                }
            }
            else if (ch == '-'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '-'){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "--", pos, Op::Dec);
                }else if (ch1 == '='){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "-=", pos, Op::MinusAssign);
                }
                else{
                    return Token(TokenKind::Operator, "-", pos, Op::Minus);
                }
            }
            else if (ch == '*'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "*=", pos, Op::MultiplyAssign);
                }
                else{
                    return Token(TokenKind::Operator, "*", pos, Op::Multiply);
                }
            }
            else if (ch == '%'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "%=", pos, Op::ModulusAssign);
                }
                else{
                    return Token(TokenKind::Operator, "%", pos, Op::Modulus);
                }
            }
            else if (ch == '>'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, ">=", pos, Op::GE);
                }
                else if (ch1 == '>'){
                    this->stream.next();
                    auto ch1 = this->stream.peek();
                    if (ch1 == '>'){
                        this->stream.next();
                        ch1 = this->stream.peek();
                        if (ch1 == '='){
                            this->stream.next();
                            pos.end = this->stream.pos+1;
                            return Token(TokenKind::Operator, ">>>=", pos, Op::RightShiftLogicalAssign);
                        }
                        else{
                            pos.end = this->stream.pos+1;
                            return Token(TokenKind::Operator, ">>>", pos, Op::RightShiftLogical);
                        }
                    }
                    else if (ch1 == '='){
                        this->stream.next();
                        pos.end = this->stream.pos+1;
                        return Token(TokenKind::Operator, ">>=", pos, Op::LeftShiftArithmeticAssign);
                    }
                    else{
                        pos.end = this->stream.pos+1;
                        return Token(TokenKind::Operator, ">>", pos, Op::RightShiftArithmetic);
                    }
                }
                else{
                    return Token(TokenKind::Operator, ">", pos, Op::G);
                }
            }
            else if (ch == '<'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "<=", pos, Op::LE);
                }
                else if (ch1 == '<'){
                    this->stream.next();
                    ch1 = this->stream.peek();
                    if (ch1 == '='){
                        this->stream.next();
                        pos.end = this->stream.pos+1;
                        return Token(TokenKind::Operator, "<<=", pos, Op::LeftShiftArithmeticAssign);
                    }
                    else{
                        pos.end = this->stream.pos+1;
                        return Token(TokenKind::Operator, "<<", pos, Op::LeftShiftArithmetic);
                    }
                }
                else{
                    return Token(TokenKind::Operator, "<", pos, Op::L);
                }
            }
            else if (ch == '='){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    auto ch1 = this->stream.peek();
                    if (ch1 == '='){
                        this->stream.next();
                        pos.end = this->stream.pos+1;
                        return Token(TokenKind::Operator, "===", pos, Op::IdentityEquals);
                    }
                    else{
                        pos.end = this->stream.pos+1;
                        return Token(TokenKind::Operator, "==", pos, Op::EQ);
                    }
                }
                //箭头=>
                else if (ch1 == '>'){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "=>", pos, Op::ARROW);
                }
                else{
                    return Token(TokenKind::Operator, "=", pos, Op::Assign);
                }
            }
            else if (ch == '!'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    auto ch1 = this->stream.peek();
                    if (ch1 == '='){
                        this->stream.next();
                        pos.end = this->stream.pos+1;
                        return Token(TokenKind::Operator, "!==", pos, Op::IdentityNotEquals);
                    }
                    else{
                        pos.end = this->stream.pos+1;
                        return Token(TokenKind::Operator, "!=", pos, Op::NE);
                    }
                }
                else{
                    return Token(TokenKind::Operator, "!", pos, Op::Not);
                }
            }
            else if (ch == '|'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '|'){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "||", pos, Op::Or);
                }
                else if (ch1 == '='){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "|=", pos, Op::BitOrAssign);
                }
                else{
                    return Token(TokenKind::Operator, "|", pos, Op::BitOr);
                }
            }
            else if (ch == '&'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '&'){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "&&", pos, Op::And);
                }
                else if (ch1 == '='){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "&=", pos, Op::BitAndAssign);
                }
                else{
                    return Token(TokenKind::Operator, "&", pos, Op::BitAnd);
                }
            }
            else if (ch == '^'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    pos.end = this->stream.pos+1;
                    return Token(TokenKind::Operator, "^=", pos, Op::BitXorAssign);
                }
                else{
                    return Token(TokenKind::Operator, "^", pos, Op::BitXOr);
                }
            }
            else if (ch == '~'){
                this->stream.next();
                return Token(TokenKind::Operator, "~", pos, Op::BitNot);
            }
            else{
                //暂时去掉不能识别的字符
                dbg("Unrecognized pattern meeting ': " + std::string(1, ch) + "', at ln:" +
                ::toString(this->stream.line) + " col: " + ::toString(this->stream.col));
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

    /**
     * 字符串字面量。
     * 目前只支持双引号，并且不支持转义。
     */
    Token parseStringLiteral() {
        auto pos = this->stream.getPosition();
        Token token(TokenKind::StringLiteral, "", pos);

        //第一个字符不用判断，因为在调用者那里已经判断过了
        this->stream.next();

        while(!this->stream.eof() && this->stream.peek() !='"'){
            token.text += this->stream.next();
        }

        if(this->stream.peek()=='"'){
            //消化掉字符换末尾的引号
            this->stream.next();
        }
        else{
            dbg("Expecting an \" at line: " + ::toString(this->stream.line) + " col: " +
            ::toString(this->stream.col));
        }

        pos.end = this->stream.pos + 1;
        return token;
    }

    /**
     * 解析标识符。从标识符中还要挑出关键字。
     */
    Token parseIdentifer() {
        auto pos = this->stream.getPosition();
        Token token(TokenKind::Identifier, "", pos);

        //第一个字符不用判断，因为在调用者那里已经判断过了
        token.text += this->stream.next();

        //读入后序字符
        while(!this->stream.eof() &&
                this->isLetterDigitOrUnderScore(this->stream.peek())){
            token.text+=this->stream.next();
        }

        pos.end = this->stream.pos + 1;

        //识别出关键字（从字典里查，速度会比较快）
        auto it = KeywordMap.find(token.text);
        if (it != KeywordMap.end()){
            token.kind = TokenKind::Keyword;
            token.code = it->second;
        }

        return token;
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

#endif
