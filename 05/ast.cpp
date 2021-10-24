#include "ast.h"

std::any AstVisitor::visit(AstNode& node, std::any additional) {
    return node.accept(*this, additional);
}

std::any AstVisitor::visitParameterList(ParameterList& paramList, std::any additional) {
    std::any retVal;
    for(auto x:  paramList.params){
        retVal = this->visit(*x, additional);
    }
    return retVal;
}

std::any AstVisitor::visitVariableDecl(VariableDecl& variableDecl, std::any additional) {
    if (variableDecl.init != nullptr){
        return this->visit(*variableDecl.init, additional);
    }
    return std::any();
}

std::any AstVisitor::visitVariableStatement(VariableStatement& variableStmt, std::any additional) {
    return this->visit(*variableStmt.variableDecl, additional);
}

std::any AstVisitor::visitExpressionStatement(ExpressionStatement& stmt, std::any additional) {
    return this->visit(*stmt.exp, additional);
}

std::any AstVisitor::visitFunctionCall(FunctionCall& functionCall, std::any additional) {
    for(auto param: functionCall.arguments){
        // console.log("in AstVisitor.visitFunctionCall, visiting param: "+ param.dump(""));
        this->visit(*param, additional);
    }
    return std::any();
}