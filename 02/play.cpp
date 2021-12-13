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
#include <unordered_map>
#include <algorithm>
#include <memory>

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


class Tokenizer{
private:
    CharStream& stream;
    Token nextToken{TokenKind::Eof,  ""};
public:
    Tokenizer(CharStream& stream) : stream(stream){}
    Token next() {
        //在第一次的时候，先parse一个Token
        if(this->nextToken.kind == TokenKind::Eof && !this->stream.eof()){
            this->nextToken = this->getAToken();
        }
        auto lastToken = this->nextToken;

        //往前走一个Token
        this->nextToken = this->getAToken();
        return lastToken;
    }

    Token peek() {
        if (this->nextToken.kind == TokenKind::Eof && !this->stream.eof()){
            this->nextToken = this->getAToken();
        }
        return this->nextToken;
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
        return ((ch>='A' && ch<='Z') ||
                (ch>='a' && ch<='z') ||
                (ch>='0' && ch<='9') ||
                ch== '_');
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

enum class AstNodeType {AstNode, Statement, Prog, FunctionCall, FunctionBody, FunctionDecl};

class AstNode{
protected:
    AstNodeType type = AstNodeType::AstNode;
public:
    //打印对象信息，prefix是前面填充的字符串，通常用于缩进显示
    virtual ~AstNode() {}
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
class FunctionDecl;
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

class FunctionBody: public Statement{
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
    FunctionDecl(const std::string& name, std::shared_ptr<FunctionBody> body){
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
        auto token = this->tokenizer.peek();


        while(token.kind != TokenKind::Eof){
            if (token.kind == TokenKind::Keyword && token.text =="function"){
                stmt = this->parseFunctionDecl();
            }
            else if (token.kind == TokenKind::Identifier){
                stmt = this->parseFunctionCall();
            }

            if (stmt != nullptr){
                stmts.push_back(stmt);
                std::cout << ("success") << std::endl;
            }
            else{
                std::cout << ("Unrecognized token: ") << std::endl;
                std::cout << (token) << std::endl;
            }
            token = this->tokenizer.peek();
        }

        return std::make_shared<Prog>(stmts);
    }

    std::shared_ptr<Statement> parseFunctionDecl() {
        //跳过关键字'function'
        this->tokenizer.next();

        auto t = this->tokenizer.next();
        if (t.kind == TokenKind::Identifier) {
            //读取"("和")"
            auto t1 = this->tokenizer.next();
            if (t1.text=="("){
                auto t2 = this->tokenizer.next();
                if (t2.text==")") {
                    auto functionBody = this->parseFunctionBody();
                    if (functionBody != nullptr &&
                        functionBody->getType() == AstNodeType::FunctionBody){
                        //如果解析成功，从这里返回
                        return std::make_shared<FunctionDecl>(t.text, functionBody);
                    } else {
                        std::cout << ("Error parsing FunctionBody in FunctionDecl") << std::endl;
                        return nullptr;
                    }
                } else{
                    std::cout << ("Expecting ')' in FunctionDecl, while we got a " + t.text) << std::endl;
                    return nullptr;
                }
            } else{
                std::cout << ("Expecting '(' in FunctionDecl, while we got a " + t.text) << std::endl;
                return nullptr;
            }
        } else {
            std::cout << ("Expecting a function name, while we got a " + t.text) << std::endl;
            return nullptr;
        }

        return nullptr;
    }

    std::shared_ptr<FunctionBody> parseFunctionBody() {
        std::vector<std::shared_ptr<Statement>> stmts;
        auto t = this->tokenizer.next();
        if(t.text == "{"){
            while(this->tokenizer.peek().kind == TokenKind::Identifier){
                auto functionCall = this->parseFunctionCall();
                if (functionCall != nullptr && functionCall->getType() == AstNodeType::FunctionCall){
                   stmts.push_back(functionCall);
                }
                else{
                    std::cout << ("Error parsing a FunctionCall in FunctionBody.") << std::endl;
                    return nullptr;
                }
            }

            t = this->tokenizer.next();
            if (t.text == "}"){
                return std::make_shared<FunctionBody>(stmts);
            }
            else{
                std::cout << ("Expecting '}' in FunctionBody, while we got a " + t.text) << std::endl;
                return nullptr;
            }
        } else{
            std::cout << ("Expecting '{' in FunctionBody, while we got a " + t.text) << std::endl;
            return nullptr;
        }

        return nullptr;
    }

    std::shared_ptr<Statement> parseFunctionCall() {
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
                    } else{
                        std::cout<< ("Expecting parameter in FunctionCall, while we got a " + t2.text) << std::endl;
                        return nullptr;  //出错时，就不在错误处回溯了。
                    }
                    t2 = this->tokenizer.next();
                    if (t2.text != ")"){
                        if (t2.text == ","){
                            t2 = this->tokenizer.next();
                        } else{
                            std::cout << ("Expecting a comma in FunctionCall, while we got a " + t2.text) << std::endl;
                            return nullptr;
                        }
                    }
                }
                //消化掉一个分号：;
                t2 = this->tokenizer.next();
                if (t2.text == ";"){
                    return std::make_shared<FunctionCall>(t.text, params);
                } else{
                    std::cout << ("Expecting a comma in FunctionCall, while we got a " + t2.text) << std::endl;
                    return nullptr;
                }
            }
        }
        return nullptr;
    }
};

class AstVisitor{
public:
    virtual void visitProg(std::shared_ptr<Prog>& prog) = 0;

    virtual void visitFunctionDecl(std::shared_ptr<FunctionDecl> functionDecl) {
        return this->visitFunctionBody(functionDecl->body);
    }
    virtual void visitFunctionBody(std::shared_ptr<FunctionBody> functionBody) = 0;

};

class RefResolver: public AstVisitor{
    std::shared_ptr<Prog> prog;
public:
    void visitProg(std::shared_ptr<Prog>& prog) override {
        this->prog = prog;
        for(auto x: prog->stmts){
            auto type = x->getType();
            if (type == AstNodeType::FunctionCall){
                std::cout << "AstNodeType::FunctionCall" << std::endl;
                auto functionCall = std::dynamic_pointer_cast<FunctionCall>(x);
                this->resolveFunctionCall(prog, functionCall);
            } else if (type == AstNodeType::FunctionDecl) {
                std::cout << "AstNodeType::FunctionDecl" << std::endl;
                auto functionDecl = std::dynamic_pointer_cast<FunctionDecl>(x);
                this->visitFunctionDecl(functionDecl);
            }
        }
    }

    void visitFunctionBody(std::shared_ptr<FunctionBody> functionBody) override {
        if(this->prog != nullptr){
            for(auto x: functionBody->stmts){
                auto functionCall = std::dynamic_pointer_cast<FunctionCall>(x);
                return this->resolveFunctionCall(this->prog, functionCall);
            }
        }
    }

    void resolveFunctionCall(std::shared_ptr<Prog>& prog, std::shared_ptr<FunctionCall> functionCall) {
        auto functionDecl = this->findFunctionDecl(prog, functionCall->name);
        if (functionDecl != nullptr){
            functionCall->definition = functionDecl;
        } else {
            if (functionCall->name != "println"){  //系统内置函数不用报错
                std::cout << ("Error: cannot find definition of function " + functionCall->name) << std::endl;
            }
        }
    }

    std::shared_ptr<FunctionDecl> findFunctionDecl(std::shared_ptr<Prog>& prog, std::string name) {
        for(auto x: prog->stmts){
            if (x->getType() == AstNodeType::FunctionDecl) {
                auto functionDecl = std::dynamic_pointer_cast<FunctionDecl>(x);
                if (functionDecl != nullptr && functionDecl->name == name) {
                    return functionDecl;
                }
            }
        }
        return nullptr;
    }
};

class Intepretor: public AstVisitor{
public:
    void visitProg(std::shared_ptr<Prog>& prog) override {
        for(auto x: prog->stmts){
            if (x->getType() == AstNodeType::FunctionCall) {
                auto functionCall = std::dynamic_pointer_cast<FunctionCall>(x);
                this->runFunction(functionCall);
            }
        };
    }

    void visitFunctionBody(std::shared_ptr<FunctionBody> functionBody) override {
        for(auto x: functionBody->stmts){
            if (x->getType() == AstNodeType::FunctionCall) {
                auto functionCall = std::dynamic_pointer_cast<FunctionCall>(x);
                std::cout << ("visitFunctionBody.stmts: " + functionCall->name) << std::endl;
                this->runFunction(functionCall);
            }
        };
    }

    void runFunction(std::shared_ptr<FunctionCall> functionCall){
        if (functionCall->name == "println") { //内置函数
            if(!functionCall->parameters.empty()) {
                for (auto s: functionCall->parameters) {
                    std::cout << s;
                }
            }
            std::cout << std::endl;
        } else{ //找到函数定义，继续遍历函数体
            if (functionCall->definition != nullptr){
                this->visitFunctionBody(functionCall->definition->body);
            }
        }
    }
};

void compileAndRun(const std::string& program) {
    {
        CharStream charStream(program);
        Tokenizer tokenizer(charStream);
        while(tokenizer.peek().kind!=TokenKind::Eof){
            auto t = tokenizer.next();
            std::cout << t << std::endl;
        }
    }


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