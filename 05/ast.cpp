#include "ast.h"

std::any AstVisitor::visit(AstNode& node, std::any additional) {
    return node.accept(*this, additional);
}