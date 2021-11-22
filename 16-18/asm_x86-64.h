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

    template<typename T>
    bool isSame(std::shared_ptr<Oprand>& oprand1) {
        return this->kind == oprand1->kind && this->value.type() == oprand1->value.type() &&
            std::any_cast<T>(this->value) == std::any_cast<T>(oprand1->value);
    }

    virtual std::string toString() {
        if(this->kind == OprandKind::bb){
            return value2String();
        }
        else if (this->kind == OprandKind::immediate){
            return "$" + value2String();
        }
        else if (this->kind == OprandKind::returnSlot){
                return "returnSlot";
        }
        else{
            return ::toString(this->kind) + "(" + value2String() + ")";
        }

    }

    std::string value2String() {
        if (isType<uint32_t>(this->value)) {
            auto val = std::any_cast<uint32_t>(this->value);
            return std::to_string(val);
        } else if (isType<int32_t>(this->value)) {
            auto val = std::any_cast<int32_t>(this->value);
            return std::to_string(val);
        } else if (isType<std::string>(this->value)) {
            return std::any_cast<std::string>(this->value);
        } else {
            return std::string("Oprand unsupport type") + this->value.type().name();
        }
    }
};

class Inst{
public:
    AsmOpCode op;
    Inst(AsmOpCode op):op(op) {}
    virtual std::string toString() = 0;
};

class Inst_0: public Inst{
    Inst_0(AsmOpCode op): Inst(op) {}

    std::string toString() override {
        return ::toString(this->op);
    }
};

class Inst_1: public Inst{
public:
    std::shared_ptr<Oprand> oprand;
    Inst_1(AsmOpCode op, std::shared_ptr<Oprand>& oprand):Inst(op), oprand(oprand) {}

    std::string toString() override {
        return ::toString(this->op) + "\t" + this->oprand->toString();
    }

};

class Inst_2: public Inst {
public:
    std::shared_ptr<Oprand> oprand1;
    std::shared_ptr<Oprand> oprand2;
    Inst_2(AsmOpCode op, std::shared_ptr<Oprand>& oprand1, std::shared_ptr<Oprand>& oprand2):Inst(op), oprand1(oprand1), oprand2(oprand2) {}
    std::string toString() override {
        return ::toString(this->op) + "\t" + this->oprand1->toString() + ", " + this->oprand2->toString();
    }
};


class FunctionOprand: public Oprand{
public:
    std::vector<std::shared_ptr<Oprand>> args;
    std::shared_ptr<Type> returnType;
    FunctionOprand(const std::string& funtionName, std::vector<std::shared_ptr<Oprand>>& args, std::shared_ptr<Type> returnType):
        Oprand(OprandKind::function, funtionName), args(args), returnType(returnType) {
    }

    std::string toString() override {
        return "_" + std::any_cast<std::string>(this->value);
    }
};

class BasicBlock {
public:
    std::vector<std::shared_ptr<Inst>> insts;      //基本块内的指令

    int32_t funIndex {-1};   //函数编号
    int32_t bbIndex {-1};    //基本块的编号。在Lower的时候才正式编号，去除空块。
    bool isDestination {false};  //有其他块跳转到该块。

    std::string getName() {
        if (this->bbIndex != -1 && this->funIndex != -1){
            return "LBB" + std::to_string(this->funIndex) + "_" + std::to_string(this->bbIndex);
        }
        else{
            return "LBB";
        }
    }

    std::string toString(){
        std::string str;
        if (this->isDestination){
            str = this->getName()+":\n";
        }
        else{
            str = "## bb." + std::to_string(this->bbIndex) + "\n";
        }
        dbg("--------- inst.size " + std::to_string(this->insts.size()));
        for (auto inst: this->insts){
            str += ("    " + inst->toString() + "\n");
        }

        return str;
    }
};

class AsmModule{
public:
    //每个函数对应的指令数组
    std::map<std::string, std::vector<std::shared_ptr<BasicBlock>>>  fun2Code;

    //每个函数的变量数，包括参数、本地变量和临时变量
     std::map<std::string, uint32_t> numTotalVars;

    //是否是叶子函数
    std::map<std::string, bool> isLeafFunction;

    //字符串常量
    std::vector<std::string> stringConsts;

    /**
     * 输出代表该模块的asm文件的字符串。
     */
    std::string toString(){
        std::string str = "    .section	__TEXT,__text,regular,pure_instructions\n";  //伪指令：一个文本的section
        for (auto& item: this->fun2Code){
            auto funName = "_" + item.first;
            str += ("\n    .global " + funName + "\n");  //添加伪指令
            str += (funName + ":\n");
            str += "    .cfi_startproc\n";
            auto bbs = item.second;
            dbg("--------- bb.size " + std::to_string(bbs.size()));
            for (auto& bb: bbs){
                str += bb->toString();
            }
            str += "    .cfi_endproc\n";
        }
        return str;

    }
};


struct TempStates{
    //当前的函数，用于查询本地变量的下标
    std::shared_ptr<FunctionSymbol> functionSym;

    //当前函数生成的指令
    std::vector<std::shared_ptr<BasicBlock>> bbs;

    //下一个临时变量的下标
    uint32_t nextTempVarIndex {0};

    //已经不再使用的临时变量，可以被复用
    //优先使用返回值寄存器，可以减少寄存器之间的拷贝
    std::vector<uint32_t> deadTempVars;

    //每个表达式节点对应的临时变量的索引
    // tempVarMap:Map<Expression, number> = new Map();

    //主要用于判断当前的Unary是一个表达式的一部分，还是独立的一个语句
    bool inExpression {false};

    //保存一元后缀运算符对应的指令。
    // postfixUnaryInst:Inst_1|null = null;
};


class AsmGenerator: public AstVisitor{
public:
    //编译后的结果
    std::shared_ptr<AsmModule> asmModule;

    //用来存放返回值的位置
    std::shared_ptr<Oprand> returnSlot;

    //一些状态变量
    std::shared_ptr<TempStates> s;

    AsmGenerator() {
        this->asmModule = std::make_shared<AsmModule>();
        this->returnSlot = std::make_shared<Oprand>(OprandKind::returnSlot, -1);
        this->s = std::make_shared<TempStates>();
    }

    uint32_t allocateTempVar() {
        uint32_t varIndex = 0;
        if (this->s->deadTempVars.size() >0){
            varIndex = this->s->deadTempVars.back();
            this->s->deadTempVars.pop_back();
        }
        else{
            varIndex = this->s->nextTempVarIndex++;
        }
        return varIndex;
    }

    bool isTempVar(std::shared_ptr<Oprand>& oprand) {
        if (oprand != nullptr && this->s->functionSym!= nullptr){
            return oprand->kind == OprandKind::varIndex && isType<uint32_t>(oprand->value) &&
                std::any_cast<uint32_t>(oprand->value) >= this->s->functionSym->vars.size();
        }
        else{
            return false;
        }
    }

    bool isParamOrLocalVar(std::shared_ptr<Oprand>& oprand) {
        if (this->s->functionSym != nullptr){
            return oprand->kind == OprandKind::varIndex && isType<uint32_t>(oprand->value) &&
                std::any_cast<uint32_t>(oprand->value) < this->s->functionSym->vars.size();
        }
        else{
            return false;
        }
    }

    /**
     * 如果操作数不同，则生成mov指令；否则，可以减少一次拷贝。
     * @param src
     * @param dest
     */
    void movIfNotSame(std::shared_ptr<Oprand>& src, std::shared_ptr<Oprand>& dest) {
        if (!src->isSame<uint32_t>(dest)){
            auto inst = std::make_shared<Inst_2>(AsmOpCode::movl, src, dest);
            this->getCurrentBB()->insts.push_back(inst);
        }
    }

    std::shared_ptr<BasicBlock> getCurrentBB() {
        return this->s->bbs.back();
    }

    std::shared_ptr<BasicBlock> newBlock(){
        auto bb = std::make_shared<BasicBlock>();
        this->s->bbs.push_back(bb);

        return bb;
    }

    std::any visitProg(Prog& prog, std::string prefix) override {
        this->s->functionSym = prog.sym;
        this->s->nextTempVarIndex = this->s->functionSym->vars.size();

        //创建新的基本块
        this->newBlock();

        this->visitBlock(prog);
        this->asmModule->fun2Code.insert({this->s->functionSym->name, this->s->bbs});
        this->asmModule->numTotalVars.insert({this->s->functionSym->name, this->s->nextTempVarIndex});

        return this->asmModule;
    }

    /*
    std::any visitVariableDecl(VariableDecl& variableDecl, std::string prefix) override {
        if(variableDecl.init != nullptr && this->s->functionSym != nullptr){
            auto r = this->visit(*variableDecl.init);
            if (!r.has_value() || !isType<std::shared_ptr<Oprand>>(r)) {
                dbg("Error: visitVariableDecl expect Oprand");
                return std::any();
            }

            std::shared_ptr<Oprand> right = std::any_cast<std::shared_ptr<Oprand>>(r);

            auto left = std::make_shared<Oprand>(OprandKind::varIndex, this->s->functionSym->getVarIndex(variableDecl.sym->name));
            //不可以两个都是内存变量
            if (this->isParamOrLocalVar(right) || right->kind == OprandKind::immediate){
                auto newRight = std::make_shared<Oprand>(OprandKind::varIndex, this->allocateTempVar());
                this->getCurrentBB()->insts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl, right, newRight));
                if (this->isTempVar(right)){
                    if (!isType<uint32_t>(right->value)) {
                        dbg("Error: visitVariableDecl isTempVar expect uint32_t");
                        return std::any();
                    }
                    uint32_t index = std::any_cast<uint32_t>(right->value);
                    this->s->deadTempVars.push_back(index);
                }
                right = newRight;
            }
            this->movIfNotSame(right, left);
            return left;
        }
        return std::any();
    }*/

    std::any visitVariableDecl(VariableDecl& variableDecl, std::string prefix) override {
        if(this->s->functionSym !=nullptr){
            std::shared_ptr<Oprand> right;
            if (variableDecl.init != nullptr){
                auto r = this->visit(*variableDecl.init);
                if (!r.has_value() || !isType<std::shared_ptr<Oprand>>(r)) {
                    dbg("Error: visitVariableDecl expect Oprand");
                    return std::any();
                }

                right = std::any_cast<std::shared_ptr<Oprand>>(r);
            }
            auto varIndex = this->s->functionSym->getVarIndex(variableDecl.sym->name);
            auto left = std::make_shared<Oprand>(OprandKind::varIndex, varIndex);

            //插入一条抽象指令，代表这里声明了一个变量
            this->getCurrentBB()->insts.push_back(std::make_shared<Inst_1>(AsmOpCode::decl, left));

            //赋值
            if (right != nullptr) {
                this->movIfNotSame(right, left);
            }

            return left;
        }

        return std::any();
    }

    std::any visitVariable(Variable& variable, std::string prefix) override {
        if (this->s->functionSym !=nullptr && variable.sym != nullptr){
            return std::make_shared<Oprand>(OprandKind::varIndex, this->s->functionSym->getVarIndex(variable.sym->name));
        }
        return std::any();
    }

    std::any visitIntegerLiteral(IntegerLiteral& integerLiteral, std::string prefix) override {
        return std::make_shared<Oprand>(OprandKind::immediate, integerLiteral.value);
    }

    std::any visitReturnStatement(ReturnStatement& returnStatement, std::string prefix) override {
        if (returnStatement.exp != nullptr){
            auto ret = this->visit(*returnStatement.exp);
            auto op = std::any_cast<std::shared_ptr<Oprand>>(ret);
            //把返回值赋给相应的寄存器
            this->movIfNotSame(op, this->returnSlot);
        }
        return std::any();
    }

    std::any visitFunctionCall(FunctionCall& functionCall, std::string prefix) override {
        //当前函数不是叶子函数
        this->asmModule->isLeafFunction.insert({this->s->functionSym->name, false});

        auto& insts = this->getCurrentBB()->insts;

        std::vector<std::shared_ptr<Oprand>> args;
        for(auto arg: functionCall.arguments){
            auto res = this->visit(*arg);
            if (!res.has_value() || !isType<std::shared_ptr<Oprand>>(res)) {
                dbg("Error: visitFunctionCall get arg failed: ");
                return std::any();
            }

            auto oprand = std::any_cast<std::shared_ptr<Oprand>>(res);
            args.push_back(oprand);
        }

        auto functionSym = functionCall.sym;
        auto functionType = std::dynamic_pointer_cast<FunctionType>(functionSym->theType);

        dbg("--------- inst.size " + std::to_string(insts.size()));
        std::shared_ptr<Oprand> op = std::make_shared<FunctionOprand>(functionCall.name, args, functionType->returnType);
        insts.push_back(std::make_shared<Inst_1>(AsmOpCode::callq, op));

        //把结果放到一个新的临时变量里
        if(functionType->returnType != SysTypes::Void()) { //函数有返回值时
            auto dest = std::make_shared<Oprand>(OprandKind::varIndex, this->allocateTempVar());
            insts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl, this->returnSlot, dest));
            return dest;
        }

        dbg("--------- inst.size " + std::to_string(insts.size()));

        return std::any();
    }

};


std::string compileToAsm(AstNode& node, bool verbose = true);

#endif