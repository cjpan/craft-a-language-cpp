#ifndef __PARSE_H_
#define __PARSE_H_

#include "scanner.h"
#include "error.h"
#include "scope.h"
#include "types.h"
#include "symbol.h"
#include "common.h"
#include "ast.h"

#include "dbg.h"

#include <string>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <any>
#include <stdint.h>

class Parser{
    static std::map<Op, int32_t> opPrec;
public:
    Scanner& scanner;
    Parser(Scanner& scanner): scanner(scanner){
    }

    std::vector<CompilerError> errors;   //语法错误
    std::vector<CompilerError> warnings; //语法报警

    void addError(const std::string msg, Position pos){
        this->errors.push_back(CompilerError(msg,pos,false));
        dbg("@" + pos.toString() +" : " + msg);
    }

    void addWarning(const std::string msg, Position pos){
        this->warnings.push_back(CompilerError(msg,pos,true));
        dbg("@" + pos.toString() +" : " + msg);
    }

    std::shared_ptr<AstNode> parseProg() {
        return nullptr;
    }

    std::vector<std::shared_ptr<AstNode>> parseStatementList() {
        return {};
    }

    std::shared_ptr<AstNode> parseStatement() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseReturnStatement() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseVariableStatement() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseVariableDecl() {
        auto beginPos = this->scanner.getNextPos();
        auto t = this->scanner.next();
        if (t.kind == TokenKind::Identifier){
            auto varName = t.text;

            std::string varType = "any";
            std::shared_ptr<AstNode> init;
            auto isErrorNode = false;

            auto t1 = this->scanner.peek();
            //可选的类型注解
            if (isType<Seperator>(t1.code) && std::any_cast<Seperator>(t1.code) == Seperator::Colon){  //':'
                this->scanner.next();
                t1 = this->scanner.peek();
                if (t1.kind == TokenKind::Identifier){
                    this->scanner.next();
                    varType = t1.text;
                }
                else{
                    this->addError("Error parsing type annotation in VariableDecl", this->scanner.getLastPos());
                    //找到下一个等号
                    this->skip({"="});
                    isErrorNode=true;
                }
            }

            //可选的初始化部分
            t1 = this->scanner.peek();
            if (isType<Op>(t1.code) && std::any_cast<Op>(t1.code) == Op::Assign){  //'='
                this->scanner.next();
                init = this->parseExpression();
            }

            return std::make_shared<VariableDecl>(beginPos, this->scanner.getLastPos(), varName, &this->parseType(varType), init, isErrorNode);
        }
        else{
            this->addError("Expecting variable name in VariableDecl, while we meet " + t.text, this->scanner.getLastPos());
            this->skip();
            std::shared_ptr<AstNode> init;
            return std::make_shared<VariableDecl>(beginPos, this->scanner.getLastPos(), "unknown", &SysTypes::Any, init, true);
        }
    }

    std::shared_ptr<AstNode> parseFunctionDecl() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseCallSignature() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseParameterList() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseBlock() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseExpression() {
        return this->parseAssignment();
    }

    std::shared_ptr<AstNode> parseExpressionStatement() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseAssignment() {
        auto assignPrec = this->getPrec(Op::Assign);
        //先解析一个优先级更高的表达式
        std::shared_ptr<AstNode> exp1 = this->parseBinary(assignPrec);
        auto t = this->scanner.peek();
        auto tprec = this->getPrec(std::any_cast<Op>(t.code));
        //存放赋值运算符两边的表达式
        std::vector<std::shared_ptr<AstNode>> expStack;
        expStack.push_back(exp1);
        //存放赋值运算符
        std::vector<Op> opStack;

        //解析赋值表达式
        while (t.kind == TokenKind::Operator &&  tprec == assignPrec){
            opStack.push_back(std::any_cast<Op>(t.code));
            this->scanner.next();  //跳过运算符
            //获取运算符优先级高于assignment的二元表达式
            exp1 = this->parseBinary(assignPrec);
            expStack.push_back(exp1);
            t = this->scanner.peek();
            tprec = this->getPrec(std::any_cast<Op>(t.code));
        }

        //组装成右结合的AST
        exp1 = expStack.back();
        if(opStack.size() > 0){
            for(int32_t i = expStack.size() - 2; i>=0; i--){
                exp1 = std::make_shared<Binary>(opStack[i], expStack[i], exp1);
            }
        }
        return exp1;
    }

    std::shared_ptr<AstNode> parseBinary(int32_t prec) {
        return nullptr;
    }

    std::shared_ptr<AstNode> parsePrimary() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseFunctionCall() {
        return nullptr;
    }


    Type& parseType(const std::string& typeName){
        return SysTypes::Any;
    }

    int32_t getPrec(Op op) {
        auto it = this->opPrec.find(op);
        if (it == this->opPrec.end()){
            return -1;
        }

        return it->second;
    }

    void skip(const std::set<std::string> seperators = {}) {
        auto t = this->scanner.peek();
        while(t.kind != TokenKind::Eof){
            if (t.kind == TokenKind::Keyword){
                return;
            }
            else if (t.kind == TokenKind::Seperator &&
                     (t.text == "," || t.text == ";" ||
                      t.text == "{" || t.text == "}" ||
                      t.text == "(" || t.text == ")" || seperators.find(t.text) != seperators.end())){
                return;
            }
            else{
                this->scanner.next();
                t= this->scanner.peek();
            }
        }
    }
};

#endif