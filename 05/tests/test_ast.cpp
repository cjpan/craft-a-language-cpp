#include "ast.h"
#include "dbg.h"

#include <gtest/gtest.h>

TEST(AST, Ast_default)
{
    // let r:number = 4;
    // println(circleArea(r));

    Position pos;
    auto type = SysTypes::Integer;
    // let r:number = 4;
    std::shared_ptr<AstNode> init = std::make_shared<IntegerLiteral>(pos, 4);
    std::shared_ptr<AstNode> var = std::make_shared<VariableDecl>(pos, pos, "r", &type, init);
    std::shared_ptr<AstNode> stmt1 = std::make_shared<VariableStatement>(pos, pos, var);

    // println(circleArea(r));
    std::shared_ptr<AstNode> varRef = std::make_shared<Variable>(pos, pos, "r");
    std::vector<std::shared_ptr<AstNode>> parms {varRef};
    std::shared_ptr<AstNode> function = std::make_shared<FunctionCall>(pos, pos, "println", parms);
    std::shared_ptr<AstNode> stmt2 = std::make_shared<ExpressionStatement>(pos, function);

    std::vector<std::shared_ptr<AstNode>> stmts {stmt1, stmt2};
    std::shared_ptr<AstNode> prog = std::make_shared<Prog>(pos, pos, stmts);


    std::shared_ptr<AstVisitor> dumper = std::make_shared<AstDumper>();
    dumper->visit(*prog, "");
}

TEST(AST, Ast_function)
{
    // function circleArea(r : number):number{
    //     let area : number = 3.14*r*r;
    //     return area;
    // }
    //
    // let r:number = 4;
    // println(circleArea(r));

    Position pos;
    auto type = SysTypes::Integer;

    // CallSignature function circleArea(r : number):number
    std::shared_ptr<AstNode> init;
    std::shared_ptr<AstNode> para = std::make_shared<VariableDecl>(pos, pos, "r", &type, init);
    std::vector<std::shared_ptr<AstNode>> parms {para};
    std::shared_ptr<AstNode> paraList = std::make_shared<ParameterList>(pos, pos, parms);
    std::shared_ptr<AstNode> callSignature = std::make_shared<CallSignature>(pos, pos, paraList, &type);

    // function block
    // 4 * area
    std::shared_ptr<AstNode> intger = std::make_shared<IntegerLiteral>(pos, 4);
    std::shared_ptr<AstNode> r = std::make_shared<Variable>(pos, pos, "area");
    std::shared_ptr<AstNode> op = std::make_shared<Binary>(Op::Multiply, intger, r);
    std::shared_ptr<AstNode> var = std::make_shared<VariableDecl>(pos, pos, "area", &type, op);
    std::shared_ptr<AstNode> stmt = std::make_shared<VariableStatement>(pos, pos, var);

    // return area
    std::shared_ptr<AstNode> ret = std::make_shared<Variable>(pos, pos, "area");
    std::shared_ptr<AstNode> returnStmt = std::make_shared<ReturnStatement>(pos, pos, ret);

    // block
    std::vector<std::shared_ptr<AstNode>> stmts {stmt, returnStmt};
    std::shared_ptr<AstNode> block = std::make_shared<Block>(pos, pos, stmts);

    // function
    std::shared_ptr<AstNode> functionDecl = std::make_shared<FunctionDecl>(pos, "circleArea", callSignature, block);

    // prog
    std::vector<std::shared_ptr<AstNode>> progStmts {functionDecl};
    std::shared_ptr<AstNode> prog = std::make_shared<Prog>(pos, pos, progStmts);

    std::shared_ptr<AstVisitor> dumper = std::make_shared<AstDumper>();
    dumper->visit(*prog, "");

}