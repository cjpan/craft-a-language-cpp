#ifndef __PARSE_H_
#define __PARSE_H_

#include "scanner.h"
#include "error.h"
#include "scope.h"
#include "types.h"
#include "symbol.h"
#include "common.h"
#include "ast.h"

#include "dbg.h"

#include <string>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <any>
#include <stdint.h>

class SemanticError: public CompilerError{
    AstNode& node;
public:
    SemanticError(const std::string& msg, AstNode& node, bool isWarning = false):
        CompilerError(msg, node.beginPos, /* node.endPos, */ isWarning), node(node){
    }
};


class SemanticAstVisitor: public AstVisitor{
    std::vector<std::shared_ptr<CompilerError>> errors;   //语义错误
    std::vector<std::shared_ptr<CompilerError>> warnings; //语义报警信息
public:
    void addError(const std::string& msg, AstNode& node){
        auto error = std::make_shared<SemanticError>(msg,node);
        this->errors.push_back(error);
        dbg("@" + node.beginPos.toString() +" : " + msg);
    }

    void addWarning(const std::string& msg, AstNode& node){
        auto warning = std::make_shared<SemanticError>(msg,node,true);
        this->warnings.push_back(warning);
        dbg("@" + node.beginPos.toString() +" : " + msg);
    }
};

/**
 * 把符号加入符号表。
 */
class Enter: public SemanticAstVisitor{
public:
    std::shared_ptr<Scope> scope;  //当前所属的scope
    std::shared_ptr<FunctionSymbol> functionSym;

    /**
     * 返回最顶级的Scope对象
     * @param prog
     */
    std::any visitProg(Prog& prog, std::string prefix) override {
        auto retType = SysTypes::Integer();
        std::vector<std::shared_ptr<Type>> paramTypes;
        std::shared_ptr<Type> funcType = std::make_shared<FunctionType>(retType, paramTypes);
        auto sym = std::make_shared<FunctionSymbol>("main", funcType);
        prog.sym = sym;
        this->functionSym = sym;

        return AstVisitor::visitProg(prog);
    }

    /**
     * 把函数声明加入符号表
     * @param functionDecl
     */
    std::any visitFunctionDecl(FunctionDecl& functionDecl, std::string prefix) override {

        auto currentScope = this->scope;

        //创建函数的symbol
        std::vector<std::shared_ptr<Type>> paramTypes;
        auto callSignature = std::dynamic_pointer_cast<CallSignature>(functionDecl.callSignature);
        if (callSignature!= nullptr && callSignature->paramList != nullptr){
            auto paramList = std::dynamic_pointer_cast<ParameterList>(callSignature->paramList);
            if (paramList == nullptr) {
                dbg("Error: paramList is nullptr");
            }
            for (auto& p: paramList->params){
                auto varDecl = std::dynamic_pointer_cast<VariableDecl>(p);
                if (varDecl == nullptr) {
                    dbg("Error: varDecl is nullptr");
                }
                paramTypes.push_back(varDecl->theType);
            }
        }

        auto retType = callSignature->theType;
        std::shared_ptr<Type> funcType = std::make_shared<FunctionType>(retType, paramTypes);
        std::shared_ptr<FunctionSymbol> sym = std::make_shared<FunctionSymbol>(functionDecl.name, funcType);
        sym->decl = &functionDecl;
        functionDecl.sym = sym;

        //把函数加入当前scope
        if (currentScope->hasSymbol(functionDecl.name)){
            this->addError("Dumplicate symbol: "+ functionDecl.name, functionDecl);
        }
        else{
            currentScope->enter(functionDecl.name, sym);
        }

        //修改当前的函数符号
        auto lastFunctionSym = this->functionSym;
        this->functionSym = sym;


        //创建新的Scope，用来存放参数
        auto oldScope = currentScope;
        this->scope = std::make_shared<Scope>(oldScope);
        functionDecl.scope = this->scope;

        //遍历子节点
        AstVisitor::visitFunctionDecl(functionDecl);

        //恢复当前函数
        this->functionSym = lastFunctionSym;

        //恢复原来的Scope
        this->scope = oldScope;

        return std::any();
    }

    /**
     * 遇到块的时候，就建立一级新的作用域。
     * 支持块作用域
     * @param block
     */
    std::any visitBlock(Block& block, std::string prefix) override {
        //创建下一级scope
        auto oldScope = this->scope;
        this->scope = std::make_shared<Scope>(this->scope);
        block.scope = this->scope;

        //调用父类的方法，遍历所有的语句
        AstVisitor::visitBlock(block);

        //重新设置当前的Scope
        this->scope = oldScope;

        return std::any();
   }


    /**
     * 把变量声明加入符号表
     * @param variableDecl
     */
    std::any visitVariableDecl(VariableDecl& variableDecl, std::string prefix) override {
        auto currentScope = this->scope;
        if (currentScope->hasSymbol(variableDecl.name)){
            this->addError("Dumplicate symbol: "+ variableDecl.name, variableDecl);
        }
        //把变量加入当前的符号表
        auto sym = std::make_shared<VarSymbol>(variableDecl.name, variableDecl.theType);
        variableDecl.sym = sym;
        currentScope->enter(variableDecl.name, sym);

        //把本地变量也加入函数符号中，可用于后面生成代码
        this->functionSym->vars.push_back(sym);
        return std::any();
    }

};

#endif