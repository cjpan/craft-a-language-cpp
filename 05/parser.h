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
        auto beginPos = this->scanner.peek().pos;
        auto stmts  = this->parseStatementList();

        return std::make_shared<Prog>(beginPos, this->scanner.getLastPos(), stmts);
    }

    std::vector<std::shared_ptr<AstNode>> parseStatementList() {
        std::vector<std::shared_ptr<AstNode>> stmts;
        auto t = this->scanner.peek();
        //statementList的Follow集合里有EOF和'}'这两个元素，分别用于prog和Block等场景。
        while(t.kind != TokenKind::Eof && !(isType<Seperator>(t.code) &&
              std::any_cast<Seperator>(t.code) == Seperator::CloseBrace)){   //'}'
            auto stmt = this->parseStatement();
            stmts.push_back(stmt);
            t = this->scanner.peek();
        }
        return stmts;
    }

    std::shared_ptr<AstNode> parseStatement() {
        auto t = this->scanner.peek();
        auto code = t.code;
        //根据'function'关键字，去解析函数声明
        if (isType<KeywordKind>(code) && std::any_cast<KeywordKind>(code) == KeywordKind::Function){
            return this->parseFunctionDecl();
        }
        else if (isType<KeywordKind>(code) && std::any_cast<KeywordKind>(code) == KeywordKind::Let){
            return this->parseVariableStatement();
        }
        //根据'return'关键字，解析return语句
        else if (isType<KeywordKind>(code) && std::any_cast<KeywordKind>(code) == KeywordKind::Return){
            return this->parseReturnStatement();
        }
        else if (isType<KeywordKind>(code) && std::any_cast<KeywordKind>(code) == KeywordKind::If){
            return this->parseIfStatement();
        }
        else if (isType<KeywordKind>(code) && std::any_cast<KeywordKind>(code) == KeywordKind::For){
            return this->parseForStatement();
        }
        else if (isType<Seperator>(code) && std::any_cast<Seperator>(code) == Seperator::OpenBrace){  //'{'
            return this->parseBlock();
        }
        else if (t.kind == TokenKind::Identifier ||
                 t.kind == TokenKind::DecimalLiteral ||
                 t.kind == TokenKind::IntegerLiteral ||
                 t.kind == TokenKind::StringLiteral ||
                 (isType<Seperator>(code) && std::any_cast<Seperator>(code) == Seperator::OpenParen)){  //'('
            return this->parseExpressionStatement();
        }
        else{
            this->addError("Can not recognize a statement starting with: " + this->scanner.peek().text, this->scanner.getLastPos());
            auto beginPos = this->scanner.getNextPos();
            this->skip();
            return std::make_shared<ErrorStmt>(beginPos,this->scanner.getLastPos());
        }
    }

    std::shared_ptr<AstNode> parseVariableStatement() {
        auto beginPos = this->scanner.getNextPos();
        auto isErrorNode = false;
        //跳过'let'
        this->scanner.next();

        std::shared_ptr<AstNode> variableDecl = this->parseVariableDecl();

        //分号，结束变量声明
        auto t = this->scanner.peek();
        if (std::any_cast<Seperator>(t.code) == Seperator::SemiColon){   //';'
            this->scanner.next();
        }
        else{
            this->skip();
            isErrorNode = true;
        }

        return std::make_shared<VariableStatement>(beginPos,this->scanner.getLastPos(), variableDecl,isErrorNode);
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
        auto beginPos = this->scanner.getNextPos();
        auto t = this->scanner.peek();
        //跳过'{'
        this->scanner.next();
        auto stmts = this->parseStatementList();
        t = this->scanner.peek();
        if (isType<Seperator>(t.code) && std::any_cast<Seperator>(t.code) == Seperator::CloseBrace){  //'}'
            this->scanner.next();
            return std::make_shared<Block>(beginPos, this->scanner.getLastPos(), stmts);
        }
        else{
            this->addError("Expecting '}' while parsing a block, but we got a " + t.text, this->scanner.getLastPos());
            this->skip();
            return std::make_shared<Block>(beginPos, this->scanner.getLastPos(), stmts, true);
        }
    }

    std::shared_ptr<AstNode> parseExpression() {
        return this->parseAssignment();
    }

    std::shared_ptr<AstNode> parseReturnStatement() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseIfStatement() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseForStatement() {
        return nullptr;
    }

    std::shared_ptr<AstNode> parseExpressionStatement() {
        auto exp = this->parseExpression();
        auto t = this->scanner.peek();
        auto stmt = std::make_shared<ExpressionStatement>(this->scanner.getLastPos(),exp);
        if (isType<Seperator>(t.code) &&
            std::any_cast<Seperator>(t.code) == Seperator::SemiColon){  //';'
            this->scanner.next();
        }
        else{
            this->addError("Expecting a semicolon at the end of an expresson statement, while we got a " + t.text, this->scanner.getLastPos());
            this->skip();
            stmt->endPos = this->scanner.getLastPos();
            stmt->isErrorNode = true;
        }
        return stmt;
    }

    std::shared_ptr<AstNode> parseAssignment() {
        auto assignPrec = this->getPrec(Op::Assign);
        //先解析一个优先级更高的表达式
        std::shared_ptr<AstNode> exp1 = this->parseBinary(assignPrec);
        auto t = this->scanner.peek();
        auto tprec = isType<Op>(t.code) ? this->getPrec(std::any_cast<Op>(t.code)) : -1;
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
            tprec = isType<Op>(t.code) ? this->getPrec(std::any_cast<Op>(t.code)) : -1;
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
        // dbg("parseBinary : " + prec);
        std::shared_ptr<AstNode> exp1 = this->parseUnary();
        auto t = this->scanner.peek();

        int32_t tprec = isType<Op>(t.code) ? this->getPrec(std::any_cast<Op>(t.code)) : -1;

        //下面这个循环的意思是：只要右边出现的新运算符的优先级更高，
        //那么就把右边出现的作为右子节点。
        /**
         * 对于2+3*5
         * 第一次循环，遇到+号，优先级大于零，所以做一次递归的parseBinary
         * 在递归的binary中，遇到乘号，优先级大于+号，所以形成3*5返回，又变成上一级的右子节点。
         *
         * 反过来，如果是3*5+2
         * 第一次循环还是一样，遇到*号，做一次递归的parseBinary
         * 在递归中，新的运算符的优先级要小，所以只返回一个5，跟前一个节点形成3*5,成为新的左子节点。
         * 接着做第二次循环，遇到+号，返回5，并作为右子节点，跟3*5一起组成一个新的binary返回。
         */

        while (t.kind == TokenKind::Operator &&  tprec > prec){
            this->scanner.next();  //跳过运算符
            std::shared_ptr<AstNode> exp2 = this->parseBinary(tprec);
            std::shared_ptr<AstNode> exp = std::make_shared<Binary>(std::any_cast<Op>(t.code), exp1, exp2);
            exp1 = exp;
            t = this->scanner.peek();
            tprec = isType<Op>(t.code) ? this->getPrec(std::any_cast<Op>(t.code)) : -1;
        }
        return exp1;
    }

    std::shared_ptr<AstNode> parseUnary() {
        auto beginPos = this->scanner.getNextPos();
        auto t = this->scanner.peek();

        //前缀的一元表达式
        if(t.kind == TokenKind::Operator){
            this->scanner.next();//跳过运算符
            auto exp = this->parseUnary();
            return std::make_shared<Unary>(beginPos, this->scanner.getLastPos(), std::any_cast<Op>(t.code), exp, true);
        }
        //后缀只能是++或--
        else{
            //首先解析一个primary
            auto exp = this->parsePrimary();
            auto t1 = this->scanner.peek();
            if (t1.kind == TokenKind::Operator &&
                (isType<Op>(t.code) && (std::any_cast<Op>(t.code) == Op::Inc || std::any_cast<Op>(t.code) == Op::Dec))){
                this->scanner.next(); //跳过运算符
                return std::make_shared<Unary>(beginPos, this->scanner.getLastPos(), std::any_cast<Op>(t.code), exp, false);
            }
            else{
                return exp;
            }
        }
    }

    std::shared_ptr<AstNode> parsePrimary() {
        auto beginPos = this->scanner.getNextPos();
        auto t = this->scanner.peek();
        // console.log("parsePrimary: " + t.text);

        //知识点：以Identifier开头，可能是函数调用，也可能是一个变量，所以要再多向后看一个Token，
        //这相当于在局部使用了LL(2)算法。
        if (t.kind == TokenKind::Identifier){
            auto code2 = this->scanner.peek2().code;
            if (isType<Seperator>(code2)&&  std::any_cast<Seperator>(code2) == Seperator::OpenParen){  //'('
                return this->parseFunctionCall();
            }
            else{
                this->scanner.next();
                return std::make_shared<Variable>(beginPos, this->scanner.getLastPos(), t.text);
            }
        }
        else if (t.kind == TokenKind::IntegerLiteral){
            this->scanner.next();
            return std::make_shared<IntegerLiteral>(beginPos, std::stoi(t.text));
        }
        else if (t.kind == TokenKind::DecimalLiteral){
            this->scanner.next();
            //return std::make_shared<DecimalLiteral>(beginPos,parseFloat(t.text));
            dbg("IntegerLiteral is not support!");
            return nullptr;
        }
        else if (t.kind == TokenKind::StringLiteral){
            this->scanner.next();
            //return std::make_shared<StringLiteral>(beginPos,t.text);
            dbg("StringLiteral is not support!");
            return nullptr;
        }
        else if (isType<Seperator>(t.code) && std::any_cast<Seperator>(t.code) == Seperator::OpenParen){  //'('
            this->scanner.next();
            auto exp = this->parseExpression();
            auto t1 = this->scanner.peek();
            if (isType<Seperator>(t1.code) && std::any_cast<Seperator>(t1.code) == Seperator::CloseParen){  //')'
                this->scanner.next();
            }
            else{
                this->addError("Expecting a ')' at the end of a primary expresson, while we got a " + t.text, this->scanner.getLastPos());
                this->skip();
            }
            return exp;
        }
        else{
            //理论上永远不会到达这里
            this->addError("Can not recognize a primary expression starting with: " + t.text, this->scanner.getLastPos());
            return std::make_shared<ErrorExp>(beginPos, this->scanner.getLastPos());
        }
    }

    std::shared_ptr<AstNode> parseFunctionCall() {
        auto beginPos = this->scanner.getNextPos();
        std::vector<std::shared_ptr<AstNode>> params;
        auto name = this->scanner.next().text;

        //跳过'('
        this->scanner.next();

        //循环，读出所有参数
        auto t1 = this->scanner.peek();
        while(!(isType<Seperator>(t1.code) && std::any_cast<Seperator>(t1.code) == Seperator::CloseParen) &&
                t1.kind != TokenKind::Eof){
            auto exp = this->parseExpression();
            params.push_back(exp);

            if (exp->isErrorNode){
                this->addError("Error parsing parameter for function call "+name, this->scanner.getLastPos());
            }

            t1 = this->scanner.peek();
            if (!(isType<Seperator>(t1.code) && std::any_cast<Seperator>(t1.code) == Seperator::CloseParen)){ //')'
                if (isType<Op>(t1.code) && std::any_cast<Op>(t1.code) == Op::Comma){ //','
                    t1 = this->scanner.next();
                }
                else{
                    this->addError("Expecting a comma at the end of a parameter, while we got a " + t1.text, this->scanner.getLastPos());
                    this->skip();
                    return std::make_shared<FunctionCall>(beginPos, this->scanner.getLastPos(), name, params,true);
                }
            }
        }

        if (isType<Seperator>(t1.code) && std::any_cast<Seperator>(t1.code) == Seperator::CloseParen){
            //消化掉')'
            this->scanner.next();
        }

        return std::make_shared<FunctionCall>(beginPos, this->scanner.getLastPos(), name, params);
    }


    Type& parseType(const std::string& typeName){
        static std::map<std::string, Type&> str2Type = {
            {"any", SysTypes::Any},
            {"number", SysTypes::Number},
            {"boolean", SysTypes::Boolean},
            {"string", SysTypes::String},
            {"undefined", SysTypes::Undefined},
            {"null", SysTypes::Null},
            {"void", SysTypes::Undefined},
        };

        auto it = str2Type.find(typeName);
        if (it != str2Type.end()) {
            return it->second;
        }

        this->addError("Unrecognized type: "+typeName, this->scanner.getLastPos());
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