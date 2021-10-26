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

    std::shared_ptr<AstNode> varUse = std::make_shared<Variable>(pos, pos, "r");
    std::vector<std::shared_ptr<AstNode>> parms {varUse};
    std::shared_ptr<AstNode> function = std::make_shared<FunctionCall>(pos, pos, "println", parms);
    std::shared_ptr<AstNode> stmt2 = std::make_shared<ExpressionStatement>(pos, function);

    std::vector<std::shared_ptr<AstNode>> stmts {stmt1, stmt2};
    std::shared_ptr<AstNode> prog = std::make_shared<Prog>(pos, pos, stmts);


    std::shared_ptr<AstVisitor> dumper = std::make_shared<AstDumper>();
    dumper->visit(*prog, "");

}