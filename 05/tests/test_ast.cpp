#include "ast.h"
#include "dbg.h"

#include <gtest/gtest.h>

TEST(AST, Ast_default)
{
    std::string name = "a";
    Position pos;
    auto type = SysTypes::Integer;

    std::shared_ptr<AstNode> init = std::make_shared<IntegerLiteral>(pos, 4);
    std::shared_ptr<AstNode> var = std::make_shared<VariableDecl>(pos, pos, "r", &type, init);
    std::shared_ptr<AstNode> stmt1 = std::make_shared<VariableStatement>(pos, pos, var);


    std::vector<std::shared_ptr<AstNode>> stmts {stmt1};
    std::shared_ptr<AstNode> prog = std::make_shared<Prog>(pos, pos, stmts);


    std::shared_ptr<AstVisitor> dumper = std::make_shared<AstDumper>();
    dumper->visit(*prog, "");

}