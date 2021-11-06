#ifndef __INTERPRETOR_H_
#define __INTERPRETOR_H_

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

//
// 栈桢
// 每个函数对应一级栈桢.
//
struct StackFrame{
    //存储变量的值
    std::map<std::string, std::shared_ptr<Symbol>> symMap;
    std::map<std::string, std::any> values;

    //返回值，当调用函数的时候，返回值放在这里
    std::any retVal;
};

//
// 用于封装Return语句的返回结果，并结束后续语句的执行。
//
struct ReturnValue{
    std::any value;
    ReturnValue(std::any value): value(value){
    }
};

class Intepretor: public AstVisitor{
    //调用栈
    std::vector<std::shared_ptr<StackFrame>> callStack;

    //当前栈桢
    std::shared_ptr<StackFrame> currentFrame;

    Intepretor() {
        //创建顶层的栈桢
        this->currentFrame = std::make_shared<StackFrame>();
        this->callStack.push_back(this->currentFrame);
    }

    void pushFrame(std::shared_ptr<StackFrame>& frame){
        this->callStack.push_back(frame);
        this->currentFrame = frame;
    }

    void popFrame(){
        if (this->callStack.size() > 1){
            // auto frame = this->callStack[this->callStack.size() - 2];
            this->callStack.pop_back();
            this->currentFrame = this->callStack.back();
        }
    }

    std::any getVariableValue(std::shared_ptr<Symbol>& sym) {
        auto it = this->currentFrame->values.find(sym->name);
        if (it == this->currentFrame->values.end()) {
            dbg("Error: can't find VariableValue: " + sym->name);
            return std::any();
        }
        return it->second;
    }

    void setVariableValue(const std::shared_ptr<Symbol>& sym, std::any value) {
        this->currentFrame->values.insert({sym->name, value});
    }

    std::any visitBlock(Block& block, std::string prefix) override {
        std::any retVal;
        for (auto x: block.stmts){
            retVal = this->visit(*x);
            //如果当前执行了一个返回语句，那么就直接返回，不再执行后面的语句。
            //如果存在上一级Block，也是中断执行，直接返回。
            if (retVal.has_value() && isType<ReturnValue>(retVal)) {
                return retVal;
            }
        }
        return retVal;
    }

    //函数声明不做任何事情。
    std::any visitFunctionDecl(FunctionDecl& functionDecl, std::string prefix) override {
        return std::any();
    }

    std::any visitReturnStatement(ReturnStatement& returnStatement, std::string prefix) override {
        std::any retVal;
        if (returnStatement.exp != nullptr){
            retVal = this->visit(*returnStatement.exp);
            this->setReturnValue(retVal);
        }
        return ReturnValue(retVal);  //这里是传递一个信号，让Block和for循环等停止执行。
    }

    //把返回值设置到上一级栈桢中（也就是调用者的栈桢）
    void setReturnValue(std::any retVal){
        auto frame = this->callStack[this->callStack.size() - 2];
        frame->retVal = retVal;
    }

    std::any visitVariableDecl(VariableDecl& variableDecl, std::string prefix) override {
        if(variableDecl.init != nullptr){
            auto v = this->visit(*variableDecl.init);
            this->setVariableValue(variableDecl.sym, v);
            return v;
        }
    }

    std::any visitVariable(Variable& v, std::string prefix) override {
        if (v.isLeftValue){
            return v.sym;
        }
        else{
            return this->getVariableValue(v.sym);
        }
    }

    void println(std::vector<std::shared_ptr<AstNode>> args) {
        if(args.size() >0){
            auto retVal = this->visit(*args[0]);
            if (retVal.has_value()) {
                // PrintAny()
                dbg("call println");
            }
        }
        else{
            Print("println: None");
        }
    }

    uint32_t tick() {
        return 0;
    }

    std::string integer_to_string(std::vector<std::shared_ptr<AstNode>> args){
        if(args.size() > 0){
            auto retVal = this->visit(*args[0]);
            if (retVal.has_value()) {
                if (isType<int32_t>(retVal)) {
                    auto i = std::any_cast<int32_t>(retVal);
                    return std::to_string(i);
                } else {
                    dbg("integer_to_string not support other type");
                    return "";
                }
            } else {
                dbg("integer_to_string exp1 no has value");
                return "";
            }
        }

        dbg("integer_to_string param 0");
        return "";
    }
};

#endif