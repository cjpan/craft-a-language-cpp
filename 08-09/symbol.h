#ifndef __SYMBOL_H_
#define __SYMBOL_H_

#include "dbg.h"
#include "types.h"
#include "common.h"

#include <any>
#include <vector>
#include <memory>
#include <string>
#include <map>

#include <stdio.h>
#include <stdint.h>

enum class SymKind{Variable, Function, Class, Interface, Parameter, Prog};

std::string SymKindtoString(SymKind kind);

class VarSymbol;
class FunctionSymbol;
class SymbolVisitor{
public:
    virtual std::any visitVarSymbol(VarSymbol& sym, std::string additional);
    virtual std::any visitFunctionSymbol(FunctionSymbol&sym, std::string additional);
};


class Symbol{
public:
    std::string name;
    std::shared_ptr<Type> theType;
    SymKind kind;
    Symbol(const std::string& name, std::shared_ptr<Type>& theType, SymKind kind):
        name(name), theType(theType), kind(kind){
    }

    //
    // visitor模式
    // @param vistor
    // @param additional 额外需要传递给visitor的信息。
    //
    virtual std::any accept(SymbolVisitor& vistor, std::string additional) = 0;

};

class VarSymbol: public Symbol{
public:
    VarSymbol(const std::string& name, std::shared_ptr<Type>& theType):
        Symbol(name, theType, SymKind::Variable){
    }

    //
    // visitor模式
    // @param vistor
    // @param additional 额外需要传递给visitor的信息。
    //
    std::any accept(SymbolVisitor& vistor, std::string additional = "") override {
        return vistor.visitVarSymbol(*this, additional);
    }
};

class FunctionDecl;
class FunctionSymbol: public Symbol{
public:
    //vars:VarSymbol[] = [];
    std::vector<std::shared_ptr<Symbol>> vars;
    //本地变量的列表。参数也算本地变量。

    uint32_t opStackSize = 10; //操作数栈的大小

    std::vector<uint8_t> byteCode; //存放生成的字节码

    FunctionDecl* decl; //存放AST，作为代码来运行

    FunctionSymbol(const std::string& name, std::shared_ptr<Type>& theType, std::vector<std::shared_ptr<Symbol>> vars = {}):
        Symbol(name, theType, SymKind::Function), vars(vars)
    {
    }

    //
    // visitor模式
    // @param vistor
    // @param additional 额外需要传递给visitor的信息。
    //
    std::any accept(SymbolVisitor& vistor, std::string additional) override {
        return vistor.visitFunctionSymbol(*this, additional);
    }

    //获取参数数量
    uint32_t getNumParams(){
        auto ptr = std::dynamic_pointer_cast<FunctionType>(this->theType);
        if (ptr == nullptr) {
            dbg("error: this->theType is not FunctionType!");
            return 0;
        }
        return ptr->paramTypes.size();
    }
};

class SymbolDumper: public SymbolVisitor{
public:
    std::any visit(Symbol& sym, std::string additional){
        return sym.accept(*this, additional);
    }


    //
    // 输出VarSymbol的调试信息
    // @param sym
    // @param additional 前缀字符串
    //
    std::any visitVarSymbol(VarSymbol& sym, std::string additional) override {
        Print(additional + sym.name + "{" + SymKindtoString(sym.kind) + "}");
        return std::any();
    }

    //
    // 输出FunctionSymbol的调试信息
    // @param sym
    // @param additional 前缀字符串
    //
    std::any visitFunctionSymbol(FunctionSymbol&sym, std::string additional) override {
        Print(additional + sym.name + "{" + SymKindtoString(sym.kind) + ", local var count:"+ std::to_string(sym.vars.size())+ "}");
        //输出字节码
        if (!sym.byteCode.empty()){
            std::string str;
            for(uint8_t code: sym.byteCode){
                char tmp[8] = {0};
                snprintf(tmp, sizeof(tmp), "%02x", code);
                str += tmp;
                str += " ";
            }
            Print(additional + "    bytecode: " + str);
        }
        return std::any();
    }
};

extern std::map<std::string, std::shared_ptr<FunctionSymbol>> built_ins;

#endif