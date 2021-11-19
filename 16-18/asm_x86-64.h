#ifndef __ASM_X86_64_H_
#define __ASM_X86_64_H_

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

/**
 * 指令的编码
 */
enum class AsmOpCode{
    //不区分字节宽度的指令
    jmp=0,
    je,
    jne,
    jle,
    jl,
    jge,
    jg,

    sete=20,
    setne,
    setl,
    setle,
    setg,
    setge,

    //8字节指令
    movq=40,
    addq,
    subq,
    mulq,
    imulq,
    divq,
    idivq,
    negq,
    incq,
    decq,
    xorq,
    orq,
    andq,
    notq,
    leaq,
    callq,
    retq,
    pushq,
    popq,
    cmpq,

    //4字节指令
    movl=80,
    addl,
    subl,
    mull,
    imull,
    divl,
    idivl,
    negl,
    incl,
    decl,
    xorl,
    orl,
    andl,
    notl,
    leal,
    calll,
    retl,
    pushl,
    popl,
    cmpl,

    //2字节指令
    movw=120,
    addw,
    subw,
    mulw,
    imulw,
    divw,
    idivw,
    negw,
    incw,
    decw,
    xorw,
    orw,
    andw,
    notw,
    leaw,
    callw,
    retw,
    pushw,
    popw,
    cmpw,

    //单字节指令
    movb=160,
    addb,
    subb,
    mulb,   //无符号乘
    imulb,  //有符号乘
    divb,   //无符号除
    idivb,  //有符号除
    negb,
    incb,
    decb,
    xorb,
    orb,
    andb,
    notb,
    leab,
    callb,
    retb,
    pushb,
    popb,
    cmpb,
};

/**
 * 操作数的类型
 */
enum class OprandKind{
    //抽象度较高的操作数
    varIndex,       //变量下标
    returnSlot,     //用于存放返回值的位置（通常是一个寄存器）
    bb,             //跳转指令指向的基本块
    function,       //函数调用
    stringConst,    //字符串常量

    //抽象度较低的操作数
    regist,       //物理寄存器
    memory,         //内存访问
    immediate,      //立即数

    //cmp指令的结果，是设置寄存器的标志位
    //后面可以根据flag和比较操作符的类型，来确定后续要生成的代码
    flag,
};


std::string toString(AsmOpCode op);
std::string toString(OprandKind kind);

class Oprand {
public:
    OprandKind kind;
    std::any value;
    Oprand(OprandKind kind, std::any value): kind(kind), value(value) {}

    bool isSame(Oprand oprand1) {
        return this->kind == oprand1.kind && this->value.type() == oprand1.value.type(); // todo
    }

    std::string toString() {
        if(this->kind == OprandKind::bb){
            return std::any_cast<std::string>(this->value);
        }
        else if (this->kind == OprandKind::immediate){
            return "$" + std::any_cast<std::string>(this->value);
        }
        else if (this->kind == OprandKind::returnSlot){
                return "returnSlot";
        }
        else{
            return ::toString(this->kind) + "(" + std::any_cast<std::string>(this->value) + ")";
        }

    }
};

class Inst{
public:
    AsmOpCode op;
    Inst(AsmOpCode op):op(op) {}
    virtual std::string toString();
};

class Inst_0: public Inst{
    Inst_0(AsmOpCode op): Inst(op) {}

    std::string toString() override {
        return ::toString(this->op);
    }
};

class Inst_1: public Inst{
public:
    Oprand oprand;
    Inst_1(AsmOpCode op, Oprand oprand):Inst(op), oprand(oprand) {}

    std::string toString() override {
        return ::toString(this->op) + "\t" + this->oprand.toString();
    }

};

class Inst_2: public Inst{
    Oprand oprand1;
    Oprand oprand2;
    Inst_2(AsmOpCode op, Oprand oprand1, Oprand oprand2):Inst(op), oprand1(oprand1), oprand2(oprand2) {}
    std::string toString() override {
        return ::toString(this->op) + "\t" + this->oprand1.toString() + ", " + this->oprand2.toString();
    }
};

#endif