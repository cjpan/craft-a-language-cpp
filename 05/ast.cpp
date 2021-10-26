#include "ast.h"

std::any AstVisitor::visit(AstNode& node, std::string additional) {
    return node.accept(*this, additional);
}

std::any AstVisitor::visitParameterList(ParameterList& paramList, std::string additional) {
    std::any retVal;
    for(auto x:  paramList.params){
        retVal = this->visit(*x, additional);
    }
    return retVal;
}

std::any AstVisitor::visitVariableDecl(VariableDecl& variableDecl, std::string additional) {
    if (variableDecl.init != nullptr){
        return this->visit(*variableDecl.init, additional);
    }
    return std::any();
}

std::any AstVisitor::visitVariableStatement(VariableStatement& variableStmt, std::string additional) {
    return this->visit(*variableStmt.variableDecl, additional);
}

std::any AstVisitor::visitExpressionStatement(ExpressionStatement& stmt, std::string additional) {
    return this->visit(*stmt.exp, additional);
}

std::any AstVisitor::visitFunctionCall(FunctionCall& functionCall, std::string additional) {
    for(auto param: functionCall.arguments){
        // console.log("in AstVisitor.visitFunctionCall, visiting param: "+ param.dump(""));
        this->visit(*param, additional);
    }
    return std::any();
}

std::any AstVisitor::visitBlock(Block& block, std::string additional) {
        std::any retVal;
        for(auto x: block.stmts){
            retVal = this->visit(*x, additional);
        }
        return retVal;
}

std::any AstVisitor::visitProg(Prog& prog, std::string additional) {
    return this->visitBlock(prog, additional);
}

std::any AstVisitor::visitCallSignature(CallSignature& callSinature, std::string additional) {
    if (callSinature.paramList!=nullptr){
        return this->visit(*callSinature.paramList, additional);
    }
    return std::any();
}

std::any AstVisitor::visitFunctionDecl(FunctionDecl& functionDecl, std::string additional) {
    this->visit(*functionDecl.callSignature, additional);
    return this->visit(*functionDecl.body, additional);
}

std::any AstVisitor::visitReturnStatement(ReturnStatement& stmt, std::string additional) {
    if (stmt.exp != nullptr){
        return this->visit(*stmt.exp, additional);
    }
    return std::any();
}

std::any AstVisitor::visitBinary(Binary& exp, std::string additional) {
    this->visit(*exp.exp1, additional);
    this->visit(*exp.exp2, additional);
    return std::any();
}
