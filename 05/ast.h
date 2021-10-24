#ifndef __AST_H_
#define __AST_H_

#include "error.h"
#include "types.h"
#include "symbol.h"
#include <string>
#include <map>
#include <memory>
#include <any>
#include <stdint.h>

class AstNode;
class ErrorStmt;
class Variable;
class IntegerLiteral;
class ParameterList;
class VariableDecl;
class VariableStatement;
class ExpressionStatement;
class FunctionCall;
class AstVisitor{
public:
    //对抽象类的访问。
    //相应的具体类，会调用visitor合适的具体方法。
    virtual std::any visit(AstNode& node, std::any additional = std::any());


    virtual std::any visitParameterList(ParameterList& paramList, std::any additional = std::any());

    virtual std::any visitVariableDecl(VariableDecl& variableDecl, std::any additional = std::any());

    virtual std::any visitVariableStatement(VariableStatement& variableStmt, std::any additional = std::any());

    virtual std::any visitExpressionStatement(ExpressionStatement& stmt, std::any additional = std::any());

    virtual std::any visitFunctionCall(FunctionCall& functionCall, std::any additional = std::any());


    virtual std::any visitVariable(Variable& node, std::any additional = std::any()) {
        return std::any();
    }
    virtual std::any visitIntegerLiteral(IntegerLiteral& node, std::any additional = std::any()) {
        return std::any();
    }
    virtual std::any visitErrorStmt(ErrorStmt& node, std::any additional = std::any()) {
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
    virtual std::any accept(AstVisitor& visitor, std::any additional) = 0;
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
    ErrorStmt(Position beginPos, Position endPos):
        Statement(beginPos, endPos, true){
    }
    std::any accept(AstVisitor& visitor, std::any additional) {
        return visitor.visitErrorStmt(*this, additional);
    }
};

class Expression: public AstNode{
public:
    Expression(Position beginPos, Position endPos, bool isErrorNode):
        AstNode(beginPos, endPos, isErrorNode){
    }

    Type* theType {nullptr};
    bool shouldBeLeftValue {false}; //当前位置需要一个左值。赋值符号、点符号的左边，需要左值。
    bool isLeftValue {false};       //是否是一个左值
    std::any constValue;        //本表达式的常量值。在常量折叠、流程分析等时候有用。

    //推断出来的类型。
    //这个类型一般是theType的子类型。比如，theType是any，但inferredType是number.

    Type* inferredType {nullptr};
};

/**
 * 表达式语句
 * 就是在表达式后面加个分号
 */
class ExpressionStatement: public Statement{
public:
    std::shared_ptr<AstNode> exp;
    ExpressionStatement(Position endPos, std::shared_ptr<AstNode> exp, bool isErrorNode = false):
        Statement(exp->beginPos, endPos,isErrorNode), exp(exp) {
    }
    std::any accept(AstVisitor& visitor, std::any additional) override {
        return visitor.visitExpressionStatement(*this, additional);
    }
};

class IntegerLiteral: public Expression{
public:
    uint32_t value;
    IntegerLiteral(Position beginPos, uint32_t value, bool isErrorNode = false):
        Expression(beginPos, beginPos, isErrorNode), value(value){
        this->theType = &SysTypes::Integer;
        this->constValue = value;
    }
    std::any accept(AstVisitor& visitor, std::any additional) override {
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
    std::any accept(AstVisitor& visitor, std::any additional) override {
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
    std::any accept(AstVisitor& visitor, std::any additional) override {
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
    Variable(Position beginPos, Position endPos, const std::string& name, bool isErrorNode):
        Expression(beginPos, endPos, isErrorNode), name(name){
    }
    std::any accept(AstVisitor& visitor, std::any additional) override {
        return visitor.visitVariable(*this, additional);
    }
};
/**
 * 变量声明节点
 */
class VariableDecl: public Decl{
public:
    Type* theType{nullptr};       //变量类型
    std::shared_ptr<Expression> init; //变量初始化所使用的表达式
    std::shared_ptr<VarSymbol> sym;
    Type* inferredType{nullptr}; //推测出的类型
    VariableDecl(Position beginPos, Position endPos,const std::string& name, Type* theType, std::shared_ptr<Expression>& init, bool isErrorNode = false):
        Decl(beginPos, endPos,name, isErrorNode),theType(theType), init(init) {
    }
    std::any accept(AstVisitor& visitor, std::any additional) override {
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
    std::any accept(AstVisitor& visitor, std::any additional) override {
        return visitor.visitVariableStatement(*this, additional);
    }
};

#endif