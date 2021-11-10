#ifndef __SEMANTICT_H_
#define __SEMANTICT_H_

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
public:
    std::vector<std::shared_ptr<CompilerError>> errors;   //语义错误
    std::vector<std::shared_ptr<CompilerError>> warnings; //语义报警信息

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


/**
 * 引用消解
 * 遍历AST。如果发现函数调用和变量引用，就去找它的定义。
 */
class RefResolver: public SemanticAstVisitor{
public:
    std::shared_ptr<Scope> scope;
    std::map<uint32_t, std::shared_ptr<Scope>> idToScope;
    std::map<uint32_t, std::map<std::string, std::shared_ptr<Symbol>>> declaredVarsMap;


    std::any visitBlock(Block& block, std::string prefix) override {
        //1.修改scope
        auto oldScope = this->scope;
        this->scope = block.scope;
        if(this->scope == nullptr) {
             dbg("error: block.scope must not be nullptr!");
             return std::any();
        }

        //为已声明的变量设置一个存储区域
        this->idToScope.insert({this->scope->id, this->scope});
        this->declaredVarsMap.insert({this->scope->id, {}});

        //2.遍历下级节点
        AstVisitor::visitBlock(block);

        //3.重新设置scope
        this->scope = oldScope;

        return std::any();
    }

    std::any visitFunctionDecl(FunctionDecl& functionDecl, std::string prefix) override {
        //1.修改scope
        auto oldScope = this->scope;
        this->scope = functionDecl.scope;
        if(this->scope == nullptr) {
            dbg("error: functionDecl.scope must not be nullptr!");
            return std::any();
        }

        //为已声明的变量设置一个存储区域
        this->idToScope.insert({this->scope->id, this->scope});
        this->declaredVarsMap.insert({this->scope->id, {}});

        //2.遍历下级节点
        AstVisitor::visitFunctionDecl(functionDecl);

        //3.重新设置scope
        this->scope = oldScope;

        return std::any();
    }

    std::any visitFunctionCall(FunctionCall& functionCall, std::string prefix) override {
        auto currentScope = this->scope;
        // console.log("in semantic.visitFunctionCall: " + functionCall.name);
        auto it = built_ins.find(functionCall.name);
        if (it != built_ins.end()){  //系统内置函数
            functionCall.sym = std::dynamic_pointer_cast<FunctionSymbol>(it->second);
        }
        else{
            auto sym = currentScope->getSymbolCascade(functionCall.name);
            functionCall.sym = std::dynamic_pointer_cast<FunctionSymbol>(sym);;
        }

        // console.log(functionCall.sym);

        //调用下级，主要是参数。
        AstVisitor::visitFunctionCall(functionCall);

        return std::any();
    }

    /**
     * 标记变量是否已被声明
     * @param variableDecl
     */
    std::any visitVariableDecl(VariableDecl& variableDecl, std::string prefix) override {
        auto currentScope = this->scope;
        auto declaredSyms = this->declaredVarsMap.find(currentScope->id);
        auto sym = currentScope->getSymbol(variableDecl.name);
        if (sym != nullptr){  //TODO 需要检查sym是否是变量
            declaredSyms->second.insert({variableDecl.name, sym});
            dbg("insert sym variableDecl: " + variableDecl.name);
        }

        //处理初始化的部分
        AstVisitor::visitVariableDecl(variableDecl);

        return std::any();
    }

    /**
     * 变量引用消解
     * 变量必须声明在前，使用在后。
     * @param variable
     */
    std::any visitVariable(Variable& variable, std::string prefix) override {
        auto currentScope = this->scope;
        variable.sym = this->findVariableCascade(currentScope, variable);

        return std::any();
    }
    /**
     * 逐级查找某个符号是不是在声明前就使用了。
     * @param scope
     * @param name
     * @param kind
     */
    std::shared_ptr<Symbol> findVariableCascade(std::shared_ptr<Scope>& scope, Variable& variable) {
        auto declaredSyms = this->declaredVarsMap.find(scope->id);
        auto symInScope = scope->getSymbol(variable.name);
        if (symInScope != nullptr){
            auto it = declaredSyms->second.find(variable.name);
            if (it != declaredSyms->second.end()){
                return it->second; //找到了，成功返回。
            }
            else{
                if (symInScope->kind == SymKind::Variable){
                    this->addError("Variable: '" + variable.name + "' is used before declaration.", variable);
                }
                else{
                    this->addError("We expect a variable of name: '" + variable.name + "', but find a " + SymKindtoString(symInScope->kind) + ".", variable);
                }
            }
        }
        else{
            if (scope->enclosingScope != nullptr){
                return this->findVariableCascade(scope->enclosingScope, variable);
            }
            else{
                this->addError("Cannot find a variable of name: '" + variable.name +"'", variable);
            }
        }

        return nullptr;
    }

};

/////////////////////////////////////////////////////////////////////////
// 自动添加return语句，以及其他导致AST改变的操作
// todo 后面用数据流分析的方法

class Trans: public SemanticAstVisitor{
public:
    std::any visitProg(Prog& prog, std::string prefix) override {
        //在后面添加return语句
        //TODO: 需要判断最后一个语句是不是已经是Return语句
        std::shared_ptr<AstNode> exp;
        auto returnStmt = std::make_shared<ReturnStatement>(prog.endPos, prog.endPos, exp);
        prog.stmts.push_back(returnStmt);

        return std::any();
    }
};

class SemanticAnalyer {
public:
    std::vector<std::shared_ptr<SemanticAstVisitor>> passes = {
        std::make_shared<Enter>(),
        std::make_shared<RefResolver>(),
        std::make_shared<Trans>(),
    };

    std::vector<std::shared_ptr<CompilerError>> errors;   //语义错误
    std::vector<std::shared_ptr<CompilerError>> warnings; //语义报警信息

    void execute(AstNode& prog) {
        for (auto pass: this->passes){
            pass->visit(prog);
            this->errors.insert(this->errors.end(), pass->errors.begin(), pass->errors.end());
            this->warnings.insert(this->warnings.end(), pass->warnings.begin(), pass->warnings.end());
        }
    }
};



#endif