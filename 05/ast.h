#ifndef __AST_H_
#define __AST_H_

#include "error.h"
#include <string>
#include <map>
#include <memory>
#include <any>

class AstNode;
class ErrorStmt;
class AstVisitor{
public:
    //对抽象类的访问。
    //相应的具体类，会调用visitor合适的具体方法。
    virtual std::any visit(AstNode& node, std::any additional = std::any());
    virtual std::any visitErrorStmt(AstNode& node, std::any additional = std::any()) {
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
        AstNode(beginPos, endPos, true){
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

#endif