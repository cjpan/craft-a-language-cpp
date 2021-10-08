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
#include <map>

#include <type_traits>
#include <any>
#include <functional>
#include <iomanip>

#include <typeindex>
#include <typeinfo>


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

    static std::set<std::string> KeyWords;

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
            if (this->isLetter(ch) || ch == '_'){
                return this->parseIdentifer();
            }
            else if (ch == '"'){
                return this->parseStringLiteral();
            }
            else if (ch == '(' || ch == ')' || ch == '{' || ch == '}' ||
                     ch == '[' || ch == ']' || ch == ',' || ch == ';' ||
                     ch == ':' || ch == '?' || ch == '@'){
                this->stream.next();
                return {TokenKind::Seperator, std::string(1, ch)};
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
                std::string literal;
                if(ch == '0'){//暂不支持八进制、二进制、十六进制
                    if (!(ch1>='1' && ch1<='9')){
                        literal= "0";
                    }
                    else {
                        std::cout << ("0 cannot be followed by other digit now, at line: " +
                        toString(this->stream.line) + " col: " + toString(this->stream.col))
                        << std::endl;
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
                    return {TokenKind::DecimalLiteral, literal};
                }
                else{
                    //返回一个整型直面量
                    return {TokenKind::IntegerLiteral, literal};
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


            else if (ch == '%'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    return {TokenKind::Operator, "%="};
                }
                else{
                    return {TokenKind::Operator, "%"};
                }
            }
            else if (ch == '>'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    return {TokenKind::Operator, ">="};
                }
                else if (ch1 == '>'){
                    this->stream.next();
                    auto ch1 = this->stream.peek();
                    if (ch1 == '>'){
                        this->stream.next();
                        ch1 = this->stream.peek();
                        if (ch1 == '='){
                            this->stream.next();
                            return {TokenKind::Operator, ">>>="};
                        }
                        else{
                            return {TokenKind::Operator, ">>>"};
                        }
                    }
                    else if (ch1 == '='){
                        this->stream.next();
                        return {TokenKind::Operator, ">>="};
                    }
                    else{
                        return {TokenKind::Operator, ">>"};
                    }
                }
                else{
                    return {TokenKind::Operator, ">"};
                }
            }
            else if (ch == '<'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    return {TokenKind::Operator, "<="};
                }
                else if (ch1 == '<'){
                    this->stream.next();
                    ch1 = this->stream.peek();
                    if (ch1 == '='){
                        this->stream.next();
                        return {TokenKind::Operator, "<<="};
                    }
                    else{
                        return {TokenKind::Operator, "<<"};
                    }
                }
                else{
                    return {TokenKind::Operator, "<"};
                }
            }
            else if (ch == '='){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    auto ch1 = this->stream.peek();
                    if (ch1='='){
                        this->stream.next();
                        return {TokenKind::Operator, "==="};
                    }
                    else{
                        return {TokenKind::Operator, "=="};
                    }
                }
                //箭头=>
                else if (ch1 == '>'){
                    this->stream.next();
                    return {TokenKind::Operator, "=>"};
                }
                else{
                    return {TokenKind::Operator, "="};
                }
            }
            else if (ch == '!'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    auto ch1 = this->stream.peek();
                    if (ch1='='){
                        this->stream.next();
                        return {TokenKind::Operator, "!=="};
                    }
                    else{
                        return {TokenKind::Operator, "!="};
                    }
                }
                else{
                    return {TokenKind::Operator, "!"};
                }
            }
            else if (ch == '|'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '|'){
                    this->stream.next();
                    return {TokenKind::Operator, "||"};
                }
                else if (ch1 == '='){
                    this->stream.next();
                    return {TokenKind::Operator, "|="};
                }
                else{
                    return {TokenKind::Operator, "|"};
                }
            }
            else if (ch == '&'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '&'){
                    this->stream.next();
                    return {TokenKind::Operator, "&&"};
                }
                else if (ch1 == '='){
                    this->stream.next();
                    return {TokenKind::Operator, "&="};
                }
                else{
                    return {TokenKind::Operator, "&"};
                }
            }
            else if (ch == '^'){
                this->stream.next();
                auto ch1 = this->stream.peek();
                if (ch1 == '='){
                    this->stream.next();
                    return {TokenKind::Operator, "^="};
                }
                else{
                    return {TokenKind::Operator, "^"};
                }
            }
            else if (ch == '~'){
                this->stream.next();
                return {TokenKind::Operator,std::string(1, ch)};
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

        //识别出关键字（从字典里查，速度会比较快）
        if (Scanner::KeyWords.count(token.text) > 0){
            token.kind = TokenKind::Keyword;
        }
        //null
        else if (token.text == "null"){
            token.kind = TokenKind::NullLiteral;
        }
        //布尔型字面量
        else if (token.text == "true" || token.text == "false"){
            token.kind = TokenKind::BooleanLiteral;
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

std::set<std::string> Scanner::KeyWords
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

struct AstVisitor;
class AstNode{
public:
    //打印对象信息，prefix是前面填充的字符串，通常用于缩进显示
    virtual void dump(const std::string& prefix) {}

    //visitor模式中，用于接受vistor的访问。
    virtual std::any accept(AstVisitor& visitor) = 0;
};

class Statement: public AstNode{
};

class Expression: public AstNode{
};

struct Block;
struct Prog;
struct Binary;
struct IntegerLiteral;
struct ExpressionStatement;
struct VariableDecl;

class AstVisitor{
public:
    //对抽象类的访问。
    //相应的具体类，会调用visitor合适的具体方法。
    std::any visit(std::shared_ptr<AstNode>& node){
        return node->accept(*this);
    }

    virtual std::any visitBlock(Block& block);
    virtual std::any visitProg(Prog& blog);

    virtual std::any visitVariableDecl(VariableDecl& variableDecl);

    virtual std::any visitBinary(Binary& exp);
    virtual std::any visitIntegerLiteral(IntegerLiteral& exp);
    virtual std::any visitExpressionStatement(ExpressionStatement& stmt);
};

class Decl: public AstNode{
public:
    std::string name;
    Decl(const std::string& name): name(name) {}
};

class Block: public AstNode{
public:
    std::vector<std::shared_ptr<AstNode>> stmts;
    Block(std::vector<std::shared_ptr<AstNode>>& stmts){
        this->stmts = stmts;
    }
    std::any accept(AstVisitor& visitor) override {
        return visitor.visitBlock(*this);
    }
    void dump(const std::string& prefix) override {
        std::cout << (prefix+"Block") << std::endl;
        std::for_each(this->stmts.begin(), this->stmts.end(),
            [&prefix](auto x) {x->dump(prefix+"    ");}
        );
    }
};

class Prog: public Block{
public:
    Prog(std::vector<std::shared_ptr<AstNode>>& stmts): Block(stmts){}

    std::any accept(AstVisitor& visitor) override {
        return visitor.visitProg(*this);
    }
    void dump(const std::string& prefix) override {
        std::cout << (prefix+"Prog") << std::endl;
        std::for_each(this->stmts.begin(), this->stmts.end(),
            [&prefix](auto x) {x->dump(prefix+"    ");}
        );
    }
};

/**
 * 变量声明节点
 */
class VariableDecl: public Decl{
public:
    std::string varType;       //变量类型
    std::shared_ptr<AstNode> init; //变量初始化所使用的表达式
    VariableDecl(const std::string& name, const std::string& varType,
        std::shared_ptr<AstNode>& init): Decl(name), varType(varType), init(init){
    }
    std::any accept(AstVisitor& visitor) override {
        return visitor.visitVariableDecl(*this);
    }
    void dump(const std::string& prefix) override {
        std::cout << (prefix+"VariableDecl "+this->name +", type: " + this->varType) << std::endl;
        if (this->init == nullptr){
            std::cout << (prefix+"no initialization.") << std::endl;
        }
        else{
            this->init->dump(prefix+"    ");
        }
    }
};
/**
 * 整型字面量
 */
class IntegerLiteral: public Expression{
public:
    int32_t value;
    IntegerLiteral(int32_t value): value(value){
    }
    std::any accept(AstVisitor& visitor) override {
        return visitor.visitIntegerLiteral(*this);
    }
    void dump(const std::string& prefix) override {
        std::cout << (prefix+ toString(this->value)) << std::endl;
    }
};

/**
 * 二元表达式
 */
class Binary: public Expression{
public:
    std::string op;      //运算符
    std::shared_ptr<AstNode> exp1; //左边的表达式
    std::shared_ptr<AstNode> exp2; //右边的表达式
    Binary(const std::string& op,
                std::shared_ptr<AstNode>& exp1,
                std::shared_ptr<AstNode>& exp2):
                op(op), exp1(exp1), exp2(exp2) {}

    std::any accept(AstVisitor& visitor) override {
        return visitor.visitBinary(*this);
    }
    void dump(const std::string& prefix) override {
        std::cout << (prefix+"Binary:"+this->op) << std::endl;
        this->exp1->dump(prefix+"    ");
        this->exp2->dump(prefix+"    ");
    }
};

class ExpressionStatement: public Statement{
public:
    std::shared_ptr<AstNode> exp;// Expression
    ExpressionStatement(std::shared_ptr<AstNode> exp): exp(exp){}
    std::any accept(AstVisitor& visitor) override {
        return visitor.visitExpressionStatement(*this);
    }
    void dump(const std::string& prefix) override {
        std::cout << (prefix+"ExpressionStatement") << std::endl;
        this->exp->dump(prefix+"    ");
    }
};

std::any AstVisitor::visitBlock(Block& block) {
    std::any ret;
    for(auto x: block.stmts){
        ret = this->visit(x);
    }
    return ret;
}

std::any AstVisitor::visitProg(Prog& prog) {
    std::any ret;
    for(auto x: prog.stmts){
        ret = this->visit(x);
    }
    return ret;
}

std::any AstVisitor::visitVariableDecl(VariableDecl& variableDecl) {
    std::any ret;
    if (variableDecl.init != nullptr){
        std::cout << "this->visit(variableDecl.init)" << std::endl;
        return this->visit(variableDecl.init);
    }
}

std::any AstVisitor::visitBinary(Binary& exp) {
    std::any ret;
    this->visit(exp.exp1);
    this->visit(exp.exp2);
    return ret;
}

std::any AstVisitor::visitIntegerLiteral(IntegerLiteral& exp) {
    return exp.value;
}

std::any AstVisitor::visitExpressionStatement(ExpressionStatement& stmt) {
    return this->visit(stmt.exp);
}

class Parser{
public:
    Scanner& scanner;
    Parser(Scanner& scanner): scanner(scanner) {}
    static std::unordered_map<std::string, int32_t> opPrec;
    static int32_t getPrec(const std::string& op){
        auto it = opPrec.find(op);
        if (it == opPrec.end()){
            return -1;
        }
        else{
            return it->second;
        }
    }


    std::shared_ptr<AstNode> parseProg() {
        auto stmts = this->parseStatementList();
        return std::make_shared<Prog>(stmts);
    }

    std::vector<std::shared_ptr<AstNode>> parseStatementList() {
        std::vector<std::shared_ptr<AstNode>> stmts;
        auto t = this->scanner.peek();
        // statementList的Follow集合里有EOF和"}"这两个元素
        // 分别用于prog和functionBody等场景。
        while(t.kind != TokenKind::Eof && t.text != "}"){
           auto stmt = this->parseStatement();

           if (stmt != nullptr){
               stmts.push_back(stmt);
           }
           else{
               std::cout << ("Error parsing a Statement in Programm.") << std::endl;
               return {};
           }
           t = this->scanner.peek();
        }

        return stmts;
    }

     std::shared_ptr<AstNode> parseStatement() {
        auto t = this->scanner.peek();
        if (t.kind == TokenKind::Keyword && t.text =="function"){
            return this->parseFunctionDecl();
        }
        else if (t.text == "let"){
            return this->parseVariableDecl();
        }
        else if (t.kind == TokenKind::Identifier ||
                 t.kind == TokenKind::DecimalLiteral ||
                 t.kind == TokenKind::IntegerLiteral ||
                 t.kind == TokenKind::StringLiteral ||
                 t.text == "("){
            return this->parseExpressionStatement();
        }
        else{
            std::cout << ("parseStatement: Can not recognize a expression starting with: " + this->scanner.peek().text) << std::endl;
            return nullptr;
        }
     }

    std::shared_ptr<AstNode> parseExpressionStatement() {
        auto exp = this->parseExpression();
        if (exp != nullptr){
            auto t = this->scanner.peek();
            if (t.text == ";"){
                this->scanner.next();
                std::shared_ptr<AstNode> expStmt = std::make_shared<ExpressionStatement>(exp);
                return expStmt;
            }
            else{
                std::cout << ("Expecting a semicolon at the end of an expresson statement, while we got a " + t.text) << std::endl;
            }
        }
        else{
            std::cout << ("Error parsing ExpressionStatement") << std::endl;
        }
        return nullptr;
    }



    std::shared_ptr<AstNode> parseExpression(){
        return this->parseBinary(0);
    }

    std::shared_ptr<AstNode> parseBinary(int32_t prec) {
        // console.log("parseBinary : " + prec);
        auto exp1 = this->parsePrimary();
        if (exp1 != nullptr){
            auto t = this->scanner.peek();
            auto tprec = this->getPrec(t.text);

            //下面这个循环的意思是：只要右边出现的新运算符的优先级更高，
            //那么就把右边出现的作为右子节点。
            /**
             * 对于2+3*5
             * 第一次循环，遇到+号，优先级大于零，所以做一次递归的binary
             * 在递归的binary中，遇到乘号，优先级大于+号，所以形成3*5返回，又变成上一级的右子节点。
             *
             * 反过来，如果是3*5+2
             * 第一次循环还是一样。
             * 在递归中，新的运算符的优先级要小，所以只返回一个5，跟前一个节点形成3*5.
             */

            while (t.kind == TokenKind::Operator &&  tprec > prec){
                this->scanner.next();  //跳过运算符
                auto exp2 = this->parseBinary(tprec);
                if (exp2 != nullptr){
                    std::shared_ptr<AstNode> exp = std::make_shared<Binary>(t.text, exp1, exp2);
                    exp1 = exp;
                    t = this->scanner.peek();
                    tprec = this->getPrec(t.text);
                }
                else{
                    std::cout << ("parseBinary1: Can not recognize a expression starting with: " + t.text) << std::endl;
                }
            }
            return exp1;
        }
        else{
            std::cout << ("parseBinary2: Can not recognize a expression starting with: " + this->scanner.peek().text) << std::endl;
            return nullptr;
        }
    }

    /**
     * 解析基础表达式。
     */
    std::shared_ptr<AstNode> parsePrimary() {
        auto t = this->scanner.peek();
        std::cout << ("parsePrimary: " + t.text) << std::endl;

        //知识点：以Identifier开头，可能是函数调用，也可能是一个变量，所以要再多向后看一个Token，
        //这相当于在局部使用了LL(2)算法。
        if (t.kind == TokenKind::Identifier){
            if (this->scanner.peek2().text == "("){
                return this->parseFunctionCall();
            }
            else{
                this->scanner.next();
                // return new Variable(t.text);
                std::cout << ("Error not support Variable in Program: " + t.text) << std::endl;
                return nullptr;
            }
        }
        else if (t.kind == TokenKind::IntegerLiteral){
            this->scanner.next();
            std::shared_ptr<AstNode> ret = std::make_shared<IntegerLiteral>(std::stoi(t.text));
            return ret;
        }
        else if (t.kind == TokenKind::DecimalLiteral){
            this->scanner.next();
            //return new DecimalLiteral(parseFloat(t.text));
            std::cout << ("Error not support DecimalLiteral in Programm.") << std::endl;
            return nullptr;
        }
        else if (t.kind == TokenKind::StringLiteral){
            this->scanner.next();
            // return new StringLiteral(t.text);
            std::cout << ("Error not support StringLiteral in Programm.") << std::endl;
            return nullptr;
        }
        else if (t.text == "("){
            this->scanner.next();
            auto exp = this->parseExpression();
            auto t1 = this->scanner.peek();
            if (t1.text == ")"){
                this->scanner.next();
                return exp;
            }
            else{
                std::cout << ("Expecting a ')' at the end of a primary expresson, while we got a " + t.text) << std::endl;
                return nullptr;
            }
        }
        else{
            std::cout << ("Can not recognize a primary expression starting with: " + t.text) << std::endl;
            return nullptr;
        }
    }

    std::shared_ptr<AstNode> parseFunctionDecl() {
        std::cout << ("Error not support function.") << std::endl;
        return nullptr;
    }

    std::shared_ptr<AstNode> parseFunctionCall() {
        std::cout << ("Error not support function.") << std::endl;
        return nullptr;
    }

    std::shared_ptr<AstNode> parseVariableDecl() {
        //跳过'let'
        this->scanner.next();

        auto t = this->scanner.next();
        if (t.kind == TokenKind::Identifier){
            auto varName = t.text;

            std::string varType = "any";
            std::shared_ptr<AstNode> init = nullptr;

            auto t1 = this->scanner.peek();
            //类型标注
            if (t1.text == ":"){
                this->scanner.next();
                t1 = this->scanner.peek();
                if (t1.kind == TokenKind::Identifier){
                    this->scanner.next();
                    varType = t1.text;
                    t1 = this->scanner.peek();
                }
                else{
                    std::cout << ("Error parsing type annotation in VariableDecl") << std::endl;
                    return nullptr;
                }
            }

            //初始化部分
            if (t1.text == "="){
                this->scanner.next();
                init = this->parseExpression();
            }

            //分号
            t1 = this->scanner.peek();
            if (t1.text==";"){
                this->scanner.next();
                return std::make_shared<VariableDecl>(varName, varType, init);
            }
            else{
                std::cout << ("Expecting ; at the end of varaible declaration, while we meet " + t1.text) << std::endl;
                return nullptr;
            }
        }
        else{
            std::cout << ("Expecting variable name in VariableDecl, while we meet " + t.text) << std::endl;
            return nullptr;
        }
    }

 };

std::unordered_map<std::string, int32_t> Parser::opPrec = {
    {"=", 2},
    {"+=", 2},
    {"-=", 2},
    {"*=", 2},
    {"-=", 2},
    {"%=", 2},
    {"&=", 2},
    {"|=", 2},
    {"^=", 2},
    {"~=", 2},
    {"<<=", 2},
    {">>=", 2},
    {">>>=", 2},
    {"||", 4},
    {"&&", 5},
    {"|", 6},
    {"^", 7},
    {"&", 8},
    {"==", 9},
    {"===", 9},
    {"!=", 9},
    {"!==", 9},
    {">", 10},
    {">=", 10},
    {"<", 10},
    {"<=", 10},
    {"<<", 11},
    {">>", 11},
    {">>>", 11},
    {"+", 12},
    {"-", 12},
    {"*", 13},
    {"/", 13},
    {"%", 13},
};

class Intepretor: public AstVisitor{
public:
    std::any visitBinary(Binary& bi) override {
        std::cout << ("visitBinary:" + bi.op) << std::endl;

        std::any ret;

        auto va1 = this->visit(bi.exp1);
        auto va2 = this->visit(bi.exp2);

        auto v1 = std::any_cast<int>(va1);
        auto v2 = std::any_cast<int>(va2);

        if (bi.op == "+") {
            ret = v1 + v2;
        } else if (bi.op == "-") {
            ret = v1 - v2;
        } else if (bi.op == "*") {
            ret = v1 * v2;
        } else if (bi.op == "/") {
            ret = v1 / v2;
        } else if (bi.op == "%") {
            ret = v1 % v2;
        } else if (bi.op == ">") {
            ret = v1 > v2;
        } else if (bi.op == ">=") {
            ret = v1 >= v2;
        } else if (bi.op == "<") {
            ret = v1 < v2;
        } else if (bi.op == "<=") {
            ret = v1 <= v2;
        // if (bi.op == "&&") {
        //     ret = v1 && v2;
        // } else if (bi.op == "||") {
        //     ret = v1 || v2;
        // } else if (bi.op == "=") {
        //     if (v1left != nullptr){
        //         this->setVariableValue(v1left.variable.name, v2);
        //     }
        //     else{
        //         std::cout << ("Assignment need a left value) { ") << std::endl;
        //     }
        }
        else {
            std::cout << ("Unsupported binary operation) { " + bi.op) << std::endl;
        }

        return ret;
    }
};

template<class T, class F>
inline std::pair<const std::type_index, std::function<void(std::any const&)>>
    to_any_visitor(F const &f)
{
    return {
        std::type_index(typeid(T)),
        [g = f](std::any const &a)
        {
            if constexpr (std::is_void_v<T>)
                g();
            else
                g(std::any_cast<T const&>(a));
        }
    };
}

static std::unordered_map<
    std::type_index, std::function<void(std::any const&)>>
    any_visitor {
        to_any_visitor<void>([]{ std::cout << "{}" << std::endl; }),
        to_any_visitor<int>([](int x){ std::cout << x << std::endl; }),
        to_any_visitor<unsigned>([](unsigned x){ std::cout << x << std::endl; }),
        to_any_visitor<float>([](float x){ std::cout << x << std::endl; }),
        to_any_visitor<double>([](double x){ std::cout << x << std::endl; }),
        to_any_visitor<char const*>([](char const *s)
            { std::cout << std::quoted(s) << std::endl; }),
        // ... add more handlers for your types ...
    };

inline void printAny(const std::any& a)
{
    if (const auto it = any_visitor.find(std::type_index(a.type()));
        it != any_visitor.cend()) {
        it->second(a);
    } else {
        std::cout << "Unregistered type "<< std::quoted(a.type().name());
    }
}


void compileAndRun(const std::string& program) {

    {
        // print stokens
        CharStream charStream(program);
        Scanner tokenizer(charStream);
        while(tokenizer.peek().kind!=TokenKind::Eof){

            // auto peek = tokenizer.peek();
            // auto peek2 = tokenizer.peek2();
            auto next = tokenizer.next();
            // std::cout << "peek:" << peek << std::endl;
            // std::cout << "peek2:" << peek2 << std::endl;
            std::cout << next << std::endl;
        }
    }

    CharStream charStream(program);
    Scanner tokenizer(charStream);

    // Syntax analysis
    auto prog = Parser(tokenizer).parseProg();
    prog->dump("");

    // Semantic analysis

    // run program
    std::cout << "------------run------------" << std::endl;
    auto ret = Intepretor().visit(prog);
    if (ret.has_value()) {
        std::cout << "program ret type: "<< ret.type().name() << std::endl;
        printAny(ret);
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