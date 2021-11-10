#include "vm.h"
#include "interpretor.h"
#include "semantic.h"
#include "parser.h"

#include "dbg.h"

#include <gtest/gtest.h>

TEST(Vm, vm_VariableDecl)
{
    std::string expect =
R"(Prog
    VariableStatement
        VariableDecl i(any)
            1(integer)
)";

    std::string program = R"(
let i = 1;
)";
    CharStream charStream(program);
    Scanner scanner(charStream);

    auto parser = Parser(scanner);
    auto ast = parser.parseProg();

    auto dumper = AstDumper();
    dumper.visit(*ast, "");

    SemanticAnalyer semanticAnalyer;
    semanticAnalyer.execute(*ast);

    dumper.clearString();
    dumper.visit(*ast, "");
    auto str = dumper.toString();
    EXPECT_STREQ(expect.c_str(), str.c_str());

    auto scopeDumper = ScopeDumper();
    scopeDumper.visit(*ast, "");

    auto interpretor = Interpretor();
    interpretor.visit(*ast, "");


    auto generator = BCGenerator();
    auto bcModule = generator.visit(*ast, "");
    auto bcModuleDumper = BCModuleDumper();

    auto bc = std::any_cast<std::shared_ptr<BCModule>>(bcModule);
    bcModuleDumper.dump(*bc);

}