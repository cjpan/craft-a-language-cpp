#ifndef __VM_H_
#define __VM_H_

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

enum class OpCode{
    //参考JVM的操作码
    iconst_0 = 0x03,
    iconst_1 = 0x04,
    iconst_2 = 0x05,
    iconst_3 = 0x06,
    iconst_4 = 0x07,
    iconst_5 = 0x08,
    bipush   = 0x10,  //8位整数入栈
    sipush   = 0x11,  //16位整数入栈
    ldc      = 0x12,  //从常量池加载，load const
    iload    = 0x15,  //本地变量入栈
    iload_0  = 0x1a,
    iload_1  = 0x1b,
    iload_2  = 0x1c,
    iload_3  = 0x1d,
    istore   = 0x36,
    istore_0 = 0x3b,
    istore_1 = 0x3c,
    istore_2 = 0x3d,
    istore_3 = 0x3e,
    iadd     = 0x60,
    isub     = 0x64,
    imul     = 0x68,
    idiv     = 0x6c,
    iinc     = 0x84,
    lcmp     = 0x94,
    ifeq     = 0x99,
    ifne     = 0x9a,
    iflt     = 0x9b,
    ifge     = 0x9c,
    ifgt     = 0x9d,
    ifle     = 0x9e,
    if_icmpeq= 0x9f,
    if_icmpne= 0xa0,
    if_icmplt= 0xa1,
    if_icmpge= 0xa2,
    if_icmpgt= 0xa3,
    if_icmple= 0xa4,
    igoto     = 0xa7,
    ireturn_0  = 0xac,
    ireturn_1   = 0xb1,
    invokestatic= 0xb8, //调用函数

    //自行扩展的操作码
    sadd     = 0x61,    //字符串连接
    sldc     = 0x13,    //把字符串常量入栈。字符串放在常量区，用两个操作数记录下标。
};

class BCModule{
public:
    //常量
    std::vector<std::any> consts;

    //入口函数
    std::shared_ptr<FunctionSymbol> _main;

    BCModule(){
        //系统函数
        for (auto& fun: built_ins){
            this->consts.push_back(fun.second);
        }
    }
};

class BCModuleDumper{
public:
    void dump(BCModule& bcModule) {
        SymbolDumper symbolDumper;
        for (auto& x: bcModule.consts) {
            if (isType<int32_t>(x)){
                auto val = std::any_cast<int32_t>(x);
                Print(std::string("Number: ") + std::to_string(val));
            }
            else if (isType<std::string>(x)){
                auto val = std::any_cast<std::string>(x);
                Print(std::string("string: ") + val);
            }
            else if (isType<std::shared_ptr<FunctionSymbol>>(x)){
                std::shared_ptr<Symbol> sym = std::any_cast<std::shared_ptr<FunctionSymbol>>(x);
                symbolDumper.visit(*sym,"");
            }
            else{
                Print(std::string("unknown const: ") + x.type().name());
                //console.log(x);
            }
        }
    }
};

class BCGenerator: public AstVisitor{
public:
    //编译后生成的模型
    std::shared_ptr<BCModule> m;

    //当前的函数，用于查询本地变量的下标
    std::shared_ptr<FunctionSymbol>  functionSym;

    //当前节点是否属于表达式的一部分。主要用于判断一元运算符应该如何生成指令。
    //TODO 以后这部分可以挪到数据流分析里。
    bool inExpression{ false };

    BCGenerator(){
        this->m = std::make_shared<BCModule>();
    }

    std::any visitProg(Prog& prog, std::string prefix) override {
        return std::any();
    }

    std::any visitBlock(Block& block, std::string prefix) override {
        return std::any();
    }

    std::any visitFunctionDecl(FunctionDecl& functionDecl, std::string prefix) override {
        return std::any();
    }

    std::any visitVariableDecl(VariableDecl& variableDecl, std::string prefix) override {
        return std::any();
    }

    std::any visitReturnStatement(ReturnStatement& stmt, std::string prefix) override {
        return std::any();
    }

    std::any visitFunctionCall(FunctionCall& functionCall, std::string prefix) override {
        return std::any();
    }

    std::any visitVariable(Variable& variable, std::string prefix) override {
        return std::any();
    }

    std::any visitIntegerLiteral(IntegerLiteral& exp, std::string prefix) override {
        return std::any();
    }

    std::any visitBinary(Binary& exp, std::string prefix) override {
        return std::any();
    }

    std::vector<int8_t> getVariableValue(std::shared_ptr<VarSymbol>& sym) {
        std::vector<int8_t> code;

        return code;
    }

    std::vector<int8_t> setVariableValue(std::shared_ptr<VarSymbol>& sym) {
        std::vector<int8_t> code;

        return code;
    }

    std::vector<int8_t> addOffsetToJumpOp(std::vector<int8_t> code, uint32_t offset) {


        return {};
    }

};

struct VMStackFrame{
    //对应的函数，用来找到代码
    std::shared_ptr<FunctionSymbol> funtionSym;

    //返回地址
    uint32_t returnIndex = 0;

    //本地变量
    std::vector<int8_t> localVars;

    //操作数栈
    std::vector<std::any> oprandStack;

    VMStackFrame(std::shared_ptr<FunctionSymbol>& funtionSym): funtionSym(funtionSym){
        this->localVars.resize(funtionSym->vars.size());
    }
};

class VM{
public:
    std::vector<VMStackFrame> callStack;

    VM(){
    }

    /**
     * 运行一个模块。
     * @param bcModule
     */
    int32_t execute(const BCModule& bcModule){
        return 0;
    }

};

#endif