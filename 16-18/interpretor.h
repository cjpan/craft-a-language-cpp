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
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <mutex>
#include <optional>

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

class Interpretor: public AstVisitor{
public:
    //调用栈
    std::vector<std::shared_ptr<StackFrame>> callStack;

    //当前栈桢
    std::shared_ptr<StackFrame> currentFrame;

    using BinaryFunction = std::function<std::any(const std::any&, const std::any&)>;
    static std::map<Op,
                    std::map<std::type_index,
                        std::map<std::type_index,
                            BinaryFunction>>> binaryOp;
    static void InitBinaryFunction();
    static std::optional<BinaryFunction> GetBinaryFunction(Op op, const std::any& l, const std::any& r);

    static std::once_flag flag;

    Interpretor() {
        //创建顶层的栈桢
        this->currentFrame = std::make_shared<StackFrame>();
        this->callStack.push_back(this->currentFrame);
        std::call_once(flag, [](){
            dbg("InitBinaryFunction: called once");
            InitBinaryFunction();
        });
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
        return std::any();
    }

    std::any visitVariable(Variable& v, std::string prefix) override {
        if (v.isLeftValue){
            return v.sym;
        }
        else{
            return this->getVariableValue(v.sym);
        }
    }

    std::any visitFunctionCall(FunctionCall& functionCall, std::string prefix) override {
        // console.log("running funciton:" + functionCall.name);
        if (functionCall.name == "println"){ //内置函数
            return this->println(functionCall.arguments);
        }
        else if (functionCall.name == "tick"){
            return this->tick();
        }
        else if (functionCall.name == "integer_to_string"){
            return this->integer_to_string(functionCall.arguments);
        }

        if(functionCall.sym != nullptr){
            //清空返回值
            this->currentFrame->retVal = std::any();

            //1.创建新栈桢
            auto frame = std::make_shared<StackFrame>();
            //2.计算参数值，并保存到新创建的栈桢
            auto functionDecl = functionCall.sym->decl;
            auto callSignature = std::dynamic_pointer_cast<CallSignature>(functionDecl->callSignature);
            if (callSignature != nullptr && callSignature->paramList != nullptr){
                auto paramList = std::dynamic_pointer_cast<ParameterList>(callSignature->paramList);
                auto params = paramList->params;
                for (uint32_t i = 0; i< params.size(); i++){
                    auto variableDecl = std::dynamic_pointer_cast<VariableDecl>(params[i]);
                    auto val = this->visit(*functionCall.arguments[i]);
                    frame->values.insert({variableDecl->sym->name, val});  //设置到新的frame里。
                }
            }

            //3.把新栈桢入栈
            this->pushFrame(frame);

            //4.执行函数
            this->visit(*functionDecl->body);

            //5.弹出当前的栈桢
            this->popFrame();

            //5.函数的返回值
            return this->currentFrame->retVal;
        }
        else{
            dbg("Runtime error, cannot find declaration of " + functionCall.name +".");
            return std::any();
        }
    }

    std::any visitBinary(Binary& bi, std::string prefix) override {
        // console.log("visitBinary:" + bi.op);
        std::any ret;
        auto v1 = this->visit(*bi.exp1);
        auto v2 = this->visit(*bi.exp2);

        auto func = Interpretor::GetBinaryFunction(bi.op, v1, v2);
        if (!func) {
            dbg("Unsupported binary operation: " + toString(bi.op));
            return ret;
        }

        return func.value()(v1, v2);
    }

    std::any println(std::vector<std::shared_ptr<AstNode>> args) {
        if(args.size() >0){
            auto retVal = this->visit(*args[0]);
            if (retVal.has_value()) {
                dbg("call println");
                PrintAny(retVal);
            }
        }
        else{
            Print("println: None");
        }

        return std::any();
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