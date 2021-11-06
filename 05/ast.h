#ifndef __AST_H_
#define __AST_H_

#include "scanner.h"
#include "error.h"
#include "scope.h"
#include "types.h"
#include "symbol.h"
#include "dbg.h"
#include "common.h"

#include <string>
#include <map>
#include <memory>
#include <any>
#include <stdint.h>
#include <sstream>

class AstNode;
class ErrorStmt;
class ErrorExp;
class Variable;
class IntegerLiteral;
class ParameterList;
class VariableDecl;
class VariableStatement;
class ExpressionStatement;
class FunctionCall;
class Block;
class Prog;
class CallSignature;
class FunctionDecl;
class ReturnStatement;
class Binary;
class Unary;

class AstVisitor{
public:
    //对抽象类的访问。
    //相应的具体类，会调用visitor合适的具体方法。
    virtual std::any visit(AstNode& node, std::string additional = "");


    virtual std::any visitParameterList(ParameterList& paramList, std::string additional = "");

    virtual std::any visitVariableDecl(VariableDecl& variableDecl, std::string additional = "");

    virtual std::any visitVariableStatement(VariableStatement& variableStmt, std::string additional = "");

    virtual std::any visitExpressionStatement(ExpressionStatement& stmt, std::string additional = "");

    virtual std::any visitFunctionCall(FunctionCall& functionCall, std::string additional = "");

    virtual std::any visitCallSignature(CallSignature& callSinature, std::string additional = "");

    virtual std::any visitFunctionDecl(FunctionDecl& functionDecl, std::string additional = "");
    virtual std::any visitReturnStatement(ReturnStatement& stmt, std::string additional = "");

    virtual std::any visitBlock(Block& block, std::string additional = "");
    virtual std::any visitProg(Prog& prog, std::string additional = "");

    virtual std::any visitBinary(Binary& exp, std::string additional = "");

    virtual std::any visitUnary(Unary& exp, std::string additional = "");

    virtual std::any visitVariable(Variable& node, std::string additional = "") {
        return std::any();
    }
    virtual std::any visitIntegerLiteral(IntegerLiteral& node, std::string additional = "") {
        return std::any();
    }
    virtual std::any visitErrorStmt(ErrorStmt& node, std::string additional = "") {
        return std::any();
    }
    virtual std::any visitErrorExp(ErrorExp& node, std::string additional = "") {
        return std::any();
    }
};


class AstNode {
public:
    Position beginPos; //在源代码中的第一个Token的位置
    Position endPos;   //在源代码中的最后一个Token的位置
    bool isErrorNode {false};// = false;

    AstNode(Position beginPos, Position endPos, bool isErrorNode):
        beginPos(beginPos), endPos(endPos), isErrorNode(isErrorNode){
    }

    //visitor模式中，用于接受vistor的访问。
    virtual std::any accept(AstVisitor& visitor, std::string additional) = 0;
};


class Statement: public AstNode{
public:
    Statement(Position beginPos, Position endPos, bool isErrorNode):
        AstNode(beginPos, endPos, isErrorNode){
    }
};

class Decl: public AstNode{
public:
    std::string name;
    Decl(Position beginPos, Position endPos, const std::string& name, bool isErrorNode):
        AstNode(beginPos, endPos, isErrorNode), name(name){
    }
};

class ErrorStmt: public Statement{
public:
    ErrorStmt(Position beginPos, Position endPos): Statement(beginPos, endPos, true) {
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitErrorStmt(*this, additional);
    }
};

class Expression: public AstNode{
public:
    Expression(Position beginPos, Position endPos, bool isErrorNode):
        AstNode(beginPos, endPos, isErrorNode){
    }

    std::shared_ptr<Type> theType;
    bool shouldBeLeftValue {false}; //当前位置需要一个左值。赋值符号、点符号的左边，需要左值。
    bool isLeftValue {false};       //是否是一个左值
    std::any constValue;        //本表达式的常量值。在常量折叠、流程分析等时候有用。

    //推断出来的类型。
    //这个类型一般是theType的子类型。比如，theType是any，但inferredType是number.

    std::shared_ptr<Type> inferredType;
};

class ErrorExp: public Expression{
public:
    ErrorExp(Position beginPos, Position endPos): Expression(beginPos, endPos, true) {
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitErrorExp(*this, additional);
    }
};

/**
 * 表达式语句
 * 就是在表达式后面加个分号
 */
class ExpressionStatement: public Statement{
public:
    std::shared_ptr<AstNode> exp;
    ExpressionStatement(Position endPos, std::shared_ptr<AstNode>& exp, bool isErrorNode = false):
        Statement(exp->beginPos, endPos,isErrorNode), exp(exp) {
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitExpressionStatement(*this, additional);
    }
};

class IntegerLiteral: public Expression{
public:
    uint32_t value;
    IntegerLiteral(Position beginPos, uint32_t value, bool isErrorNode = false):
        Expression(beginPos, beginPos, isErrorNode), value(value){
        this->theType = SysTypes::Integer();
        this->constValue = value;
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitIntegerLiteral(*this, additional);
    }
};

/**
 * 函数调用
 */
class FunctionCall: public Expression{
public:
    std::string name;
    std::vector<std::shared_ptr<AstNode>> arguments;
    // decl: FunctionDecl|null=null;  //指向函数的声明
    std::shared_ptr<FunctionSymbol> sym;
    FunctionCall(Position beginPos, Position endPos, const std::string name,
        std::vector<std::shared_ptr<AstNode>>& paramValues,
        bool isErrorNode = false): Expression(beginPos, beginPos, isErrorNode),
        name(name), arguments(paramValues){
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitFunctionCall(*this, additional);
    }
};

class ParameterList: public AstNode{
public:
    std::vector<std::shared_ptr<AstNode>> params;
    ParameterList(Position beginPos, Position endPos,
        std::vector<std::shared_ptr<AstNode>>& params, bool isErrorNode = false):
        AstNode(beginPos, endPos, isErrorNode), params(params) {
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitParameterList(*this, additional);
    }
};

/**
 * 变量引用
 */
class Variable: public Expression{
public:
    std::string name;
    std::shared_ptr<Symbol> sym;
    Variable(Position beginPos, Position endPos, const std::string& name, bool isErrorNode = false):
        Expression(beginPos, endPos, isErrorNode), name(name){
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitVariable(*this, additional);
    }
};
/**
 * 变量声明节点
 */
class VariableDecl: public Decl{
public:
    std::shared_ptr<Type> theType;       //变量类型
    std::shared_ptr<AstNode> init; //变量初始化所使用的表达式
    std::shared_ptr<VarSymbol> sym;
    std::shared_ptr<Type> inferredType; //推测出的类型
    VariableDecl(Position beginPos, Position endPos,const std::string& name, std::shared_ptr<Type>& theType, std::shared_ptr<AstNode>& init, bool isErrorNode = false):
        Decl(beginPos, endPos,name, isErrorNode),theType(theType), init(init) {
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitVariableDecl(*this, additional);
    }
};
/**
 * 变量声明语句
 */
class VariableStatement: public Statement{
public:
    std::shared_ptr<AstNode> variableDecl;
    VariableStatement(Position beginPos, Position endPos,std::shared_ptr<AstNode> variableDecl, bool isErrorNode = false):
        Statement(beginPos, endPos, isErrorNode), variableDecl(variableDecl){
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitVariableStatement(*this, additional);
    }
};

class Block: public Statement{
public:
    std::vector<std::shared_ptr<AstNode>> stmts;
    std::shared_ptr<Scope> scope;
    Block(Position beginPos, Position endPos, std::vector<std::shared_ptr<AstNode>>& stmts, bool isErrorNode = false):
        Statement(beginPos, endPos, isErrorNode), stmts(stmts){
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitBlock(*this, additional);
    }
};

class Prog: public Block{
public:
    // stmts:Statement[];
    std::shared_ptr<FunctionSymbol> sym;
    Prog(Position beginPos, Position endPos, std::vector<std::shared_ptr<AstNode>>& stmts): Block(beginPos, endPos, stmts){
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitProg(*this,additional);
    }
};

/**
 * 调用签名
 * 可以用在函数声明等多个地方。
 */
class CallSignature: public AstNode{
public:
    std::shared_ptr<AstNode> paramList;
    std::shared_ptr<Type> theType;       //返回值类型
    CallSignature(Position beginPos, Position endPos,
        std::shared_ptr<AstNode>& paramList, std::shared_ptr<Type>& theType,
        bool isErrorNode = false): AstNode(beginPos, endPos, isErrorNode),
        paramList(paramList), theType(theType){
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitCallSignature(*this, additional);
    }
};

/**
 * 函数声明节点
 */
class FunctionDecl: public Decl{
public:
    std::shared_ptr<AstNode> callSignature;
    std::shared_ptr<AstNode> body; //函数体
    std::shared_ptr<Scope> scope; //该函数对应的Scope
    std::shared_ptr<FunctionSymbol> sym;
    FunctionDecl(Position beginPos, const std::string& name,
        std::shared_ptr<AstNode>& callSignature,
        std::shared_ptr<AstNode>& body, bool isErrorNode = false):
        Decl(beginPos, endPos,name, isErrorNode),
        callSignature(callSignature), body(body){

    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitFunctionDecl(*this, additional);
    }
};

/**
 * Return语句
 */
class ReturnStatement: public Statement{
public:
    std::shared_ptr<AstNode> exp;
    ReturnStatement(Position beginPos, Position endPos, std::shared_ptr<AstNode>& exp,bool isErrorNode = false):
        Statement(beginPos, endPos, isErrorNode), exp(exp){
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitReturnStatement(*this, additional);
    }
};

class Binary: public Expression{
public:
    Op op;      //运算符
    std::shared_ptr<AstNode> exp1; //左边的表达式
    std::shared_ptr<AstNode> exp2; //右边的表达式
    Binary(Op op, std::shared_ptr<AstNode> exp1, std::shared_ptr<AstNode> exp2,
        bool isErrorNode = false): Expression(beginPos, endPos, isErrorNode),
        op(op), exp1(exp1), exp2(exp2) {
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitBinary(*this, additional);
    }
};

class Unary: public Expression{
public:
    Op op;      //运算符
    std::shared_ptr<AstNode> exp;  //表达式
    bool isPrefix;//前缀还是后缀
    Unary(Position beginPos, Position endPos, Op op, std::shared_ptr<AstNode> exp, bool isPrefix, bool isErrorNode = false):
        Expression(beginPos, endPos, isErrorNode), op(op), exp(exp), isPrefix(isPrefix) {
    }
    std::any accept(AstVisitor& visitor, std::string additional) override {
        return visitor.visitUnary(*this, additional);
    }
};

class AstDumper: public AstVisitor{
    std::stringstream ss;
public:
    std::any visit(AstNode& node, std::string prefix) override {
        return AstVisitor::visit(node, prefix);
    }

    std::string toString() {
        return ss.str();
    }

    void clearString() {
        ss.str(std::string());
    }

    std::any visitParameterList(ParameterList& paramList, std::string prefix) override {
        ss << Print(prefix+"ParamList:" + (paramList.isErrorNode? " **E** " : "") + (paramList.params.size()== 0 ? "none":""));
        for(auto x: paramList.params){
            this->visit(*x, prefix+"    ");
        }
        return std::any();
    }

    std::any visitVariableDecl(VariableDecl& variableDecl, std::string prefix) override {
        ss << Print(prefix+"VariableDecl "+variableDecl.name +
            (variableDecl.theType == nullptr? "" : "("+variableDecl.theType->name+")") +
            (variableDecl.isErrorNode? " **E** " : ""));
        if (variableDecl.init == nullptr){
            ss << Print(prefix+"no initialization.");
        }
        else{
            this->visit(*variableDecl.init, prefix+"    ");
        }
        return std::any();
    }

    std::any visitVariableStatement(VariableStatement& variableStmt, std::string prefix) override {
        ss << Print(prefix+"VariableStatement" + (variableStmt.isErrorNode? " **E** " : ""));
        return this->visit(*variableStmt.variableDecl, prefix+"    ");
    }

    std::any visitExpressionStatement(ExpressionStatement& stmt, std::string prefix) override {
        ss << Print(prefix+"ExpressionStatement" + (stmt.isErrorNode? " **E** " : ""));
        return this->visit(*stmt.exp, prefix+"    ");
    }

    std::any visitFunctionCall(FunctionCall& functionCall, std::string prefix) override {
        ss << Print(prefix+"FunctionCall "+ (functionCall.theType == nullptr? "" : "("+functionCall.theType->name+")") +
            (functionCall.isErrorNode? " **E** " : "")+functionCall.name +
            (built_ins.count(functionCall.name) > 0 ? ", built-in" :
             functionCall.sym!=nullptr ? ", resolved" : ", not resolved"));
        for(auto param: functionCall.arguments){
            this->visit(*param, prefix+"    ");
        }
        return std::any();
    }

    std::any visitProg(Prog& prog, std::string prefix) override {
        ss << Print(prefix + "Prog"+ (prog.isErrorNode? " **E** " : ""));
        for(auto x: prog.stmts){
            this->visit(*x, prefix+"    ");
        }
        return std::any();
    }

    std::any visitBlock(Block& block, std::string prefix) override {
        if(block.isErrorNode){
            ss << Print(prefix + "Block" + (block.isErrorNode? " **E** " : ""));
        }
        for(auto x: block.stmts){
            this->visit(*x, prefix+"    ");
        }
        return std::any();
    }


    std::any visitVariable(Variable& variable, std::string prefix) override {
        ss << Print(prefix+"Variable: "+ (variable.isErrorNode? " **E** " : "")+
            variable.name + (variable.theType == nullptr? "" : "("+variable.theType->name+")") +
            (variable.isLeftValue ? ", LeftValue" : "") +
            (variable.sym != nullptr ? ", resolved" : ", not resolved"));
            return std::any();
    }
    std::any visitIntegerLiteral(IntegerLiteral& exp, std::string prefix) override {
        ss << Print(prefix+ std::to_string(exp.value) +
            (exp.theType == nullptr? "" : "("+exp.theType->name+")") +
            (exp.isErrorNode? " **E** " : ""));
        return std::any();
    }
    std::any visitErrorStmt(ErrorStmt& node, std::string prefix) override {
        ss << Print(prefix+"Error Statement **E**");
        return std::any();
    }

    std::any visitErrorExp(ErrorExp& node, std::string prefix) override {
        ss << Print(prefix+"Error Expression **E**");
        return std::any();
    }

    std::any visitCallSignature(CallSignature& callSinature, std::string prefix) override {
        ss << Print(prefix+ (callSinature.isErrorNode? " **E** " : "")+"Return type: " + callSinature.theType->name);
        if (callSinature.paramList!=nullptr){
            this->visit(*callSinature.paramList, prefix + "    ");
        }

        return std::any();
    }

    std::any visitFunctionDecl(FunctionDecl& functionDecl, std::string prefix) override {
        ss << Print(prefix+"FunctionDecl "+ functionDecl.name + (functionDecl.isErrorNode? " **E** " : ""));
        this->visit(*functionDecl.callSignature, prefix+"    ");
        this->visit(*functionDecl.body, prefix+"    ");

        return std::any();
    }

    std::any visitReturnStatement(ReturnStatement& stmt, std::string prefix) override {
        ss << Print(prefix+"ReturnStatement" + (stmt.isErrorNode? " **E** " : ""));
        if (stmt.exp != nullptr){
            return this->visit(*stmt.exp, prefix+"    ");
        }

        return std::any();
    }

    std::any visitBinary(Binary& exp, std::string prefix) override {
        ss << Print(prefix+"Binary:"+ ::toString(exp.op)+ (exp.theType == nullptr? "" : "("+exp.theType->name+")") + (exp.isErrorNode? " **E** " : ""));

        this->visit(*exp.exp1, prefix+"    ");
        this->visit(*exp.exp2, prefix+"    ");
        return std::any();
    }

    std::any visitUnary(Unary& exp, std::string prefix) override {
        ss << Print(prefix + (exp.isPrefix ? "Prefix ": "PostFix ") +"Unary:"+::toString(exp.op)+ (exp.theType == nullptr? "" : "("+exp.theType->name+")") + (exp.isErrorNode? " **E** " : ""));
        this->visit(*exp.exp, prefix+"    ");
        return std::any();
    }
};


class ScopeDumper: public AstVisitor{
public:
    std::any visitFunctionDecl(FunctionDecl& functionDecl, std::string prefix) override {
        Print(prefix + "Scope of function: " + functionDecl.name);

        //显示本级Scope
        if(functionDecl.scope != nullptr){
            this->dumpScope(functionDecl.scope, prefix);
        }
        else{
            Print(prefix + "{null}");
        }

        //继续遍历
        AstVisitor::visitFunctionDecl(functionDecl, prefix+"    ");

        return std::any();
    }

    std::any visitBlock(Block& block, std::string prefix) override {
        Print(prefix + "Scope of block");
        //显示本级Scope
        if(block.scope != nullptr){
            this->dumpScope(block.scope, prefix);
        }
        else{
            Print(prefix + "{null}");
        }

        //继续遍历
        AstVisitor::visitBlock(block, prefix+"    ");
        return std::any();
    }

    void dumpScope(std::shared_ptr<Scope>& scope, std::string prefix) {
        if (scope->name2sym.size()>0){
            //遍历该作用域的符号。
            SymbolDumper symbolDumper;
            for (auto sym: scope->name2sym){
                symbolDumper.visit(*sym.second,prefix+"    ");
            }
        }
        else{
            Print(prefix + "    {empty}");
        }
    }
};
#endif