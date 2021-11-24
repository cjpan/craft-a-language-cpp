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
     std::map<std::string, uint32_t> numParams;
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

    std::shared_ptr<Oprand> any2Oprand(const std::any& val) {
        if (!val.has_value()) {
            dbg("Error: any2Oprand not has value!");
            return nullptr;
        }

        if (!isType<std::shared_ptr<Oprand>>(val)) {
            dbg("Error: any2Oprand expect Oprand, but " + std::string(val.type().name()));
            return nullptr;
        }

        return std::any_cast<std::shared_ptr<Oprand>>(val);
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
        this->asmModule->numParams.insert({this->s->functionSym->name, this->s->functionSym->getNumParams()});
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
                right = any2Oprand(r);
                if (right == nullptr) {
                    dbg("Error: expect oprand!");
                    return std::any();
                }
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
            auto op = any2Oprand(ret);
            if (op == nullptr) {
                dbg("Error: expect oprand!");
                return std::any();
            }
            //把返回值赋给相应的寄存器
            this->movIfNotSame(op, this->returnSlot);
        }
        return std::any();
    }

    std::any visitFunctionDecl(FunctionDecl& functionDecl, std::string prefix) override {
        //保存原来的状态信息
        auto s = this->s;

        //新建立状态信息
        this->s = std::make_shared<TempStates>();
        this->s->functionSym = functionDecl.sym;
        this->s->nextTempVarIndex = this->s->functionSym->vars.size();

        //计算当前函数是不是叶子函数
        //先设置成叶子变量。如果遇到函数调用，则设置为false。
        this->asmModule->isLeafFunction.insert({this->s->functionSym->name, true});

        //创建新的基本块
        this->newBlock();

        //生成代码
        this->visit(*functionDecl.body, "");
        this->asmModule->fun2Code.insert({this->s->functionSym->name, this->s->bbs});
        this->asmModule->numParams.insert({this->s->functionSym->name, this->s->functionSym->getNumParams()});
        this->asmModule->numTotalVars.insert({this->s->functionSym->name, this->s->nextTempVarIndex});

        //恢复原来的状态信息
        this->s = s;

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

    std::any visitBinary(Binary& bi, std::string prefix) override {
        this->s->inExpression = true;

        auto& insts = this->getCurrentBB()->insts;


        //左子树返回的操作数
        auto l = this->visit(*bi.exp1);
        auto left = any2Oprand(l);
        if (left == nullptr) {
            dbg("Error: expect left oprand");
            return std::any();
        }

        //右子树
        auto r = this->visit(*bi.exp2);
        auto right = any2Oprand(r);
        if (right == nullptr) {
            dbg("Error: expect right oprand");
            return std::any();
        }


        //计算出一个目标操作数
        auto dest = left;

        if (!this->isTempVar(dest)){
            dest = std::make_shared<Oprand>(OprandKind::varIndex, this->allocateTempVar());
            insts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl, left, dest));
        }

        //释放掉不用的临时变量
        if (this->isTempVar(right)){
            this->s->deadTempVars.push_back(std::any_cast<uint32_t>(right->value));
        }

        //生成指令
        //todo 有问题的地方
        switch(bi.op){
            case Op::Plus: //'+'
                if (bi.theType == SysTypes::String()) { //字符串加
                    // let args:Oprand[] = [];
                    // args.push(left);
                    // args.push(right);
                    // this->callIntrinsics("string_concat", args);
                    dbg("Error: current not support SysTypes::String");
                    return std::any();
                }
                else{
                    // this->movIfNotSame(left,dest);
                    insts.push_back(std::make_shared<Inst_2>(AsmOpCode::addl,right, dest));
                }
                break;
            case Op::Minus: //'-'
                // this->movIfNotSame(left,dest);
                insts.push_back(std::make_shared<Inst_2>(AsmOpCode::subl,right, dest));
                break;
            case Op::Multiply: //'*'
                // this->movIfNotSame(left,dest);
                insts.push_back(std::make_shared<Inst_2>(AsmOpCode::imull,right, dest));
                break;
            case Op::Divide: //'/'
                // this->movIfNotSame(left,dest);
                insts.push_back(std::make_shared<Inst_2>(AsmOpCode::idivl,right, dest));
                break;
            case Op::Assign: //'='
                // this->movIfNotSame(right,left);
                insts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl,right, dest));
                this->movIfNotSame(dest,left);    //写到内存里去
                break;
            case Op::G:
            case Op::L:
            case Op::GE:
            case Op::LE:
            case Op::EQ:
            case Op::NE:
                dbg("Unsupported OpCode in AsmGenerator.visitBinary: " + toString(bi.op));
                // insts.push_back(std::make_shared<Inst_2>(AsmOpCode::cmpl, right, dest));
                // dest = std::make_shared<Oprand>(OprandKind::flag, this->getOpsiteOp(bi.op));
                // break;
            default:
                dbg("Unsupported OpCode in AsmGenerator.visitBinary: " + toString(bi.op));
        }

        this->s->inExpression = false;

        return dest;
    }

};

//Lower
class Register: public Oprand{
    uint32_t bits = 32;  //寄存器的位数
public:
    Register(const std::string registerName, uint32_t bits = 32) : Oprand(OprandKind::regist, registerName),
        bits(bits) {
    }

    //可供分配的寄存器的数量
    //16个通用寄存器中，扣除rbp和rsp，然后保留一个寄存器，用来作为与内存变量交换的区域。
    const static uint32_t numAvailableRegs;

    //32位寄存器
    //参数用的寄存器，当然也要由caller保护
    static std::shared_ptr<Oprand> edi() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("edi");
        return oprand;
    }

    static std::shared_ptr<Oprand> esi() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("esi");
        return oprand;
    }

    static std::shared_ptr<Oprand> edx() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("edx");
        return oprand;
    }

    static std::shared_ptr<Oprand> ecx() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("ecx");
        return oprand;
    }

    static std::shared_ptr<Oprand> r8d() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r8d");
        return oprand;
    }

    static std::shared_ptr<Oprand> r9d() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r9d");
        return oprand;
    }

    //通用寄存器:caller（调用者）负责保护
    static std::shared_ptr<Oprand> r10d() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r10d");
        return oprand;
    }

    static std::shared_ptr<Oprand> r11d() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r11d");
        return oprand;
    }

    //返回值，也由Caller保护
    static std::shared_ptr<Oprand> eax() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("eax");
        return oprand;
    }

    //通用寄存器:callee（调用者）负责保护
    static std::shared_ptr<Oprand> ebx() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("ebx");
        return oprand;
    }

    static std::shared_ptr<Oprand> r12d() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r12d");
        return oprand;
    }

    static std::shared_ptr<Oprand> r13d() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r13d");
        return oprand;
    }

    static std::shared_ptr<Oprand> r14d() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r14d");
        return oprand;
    }

    static std::shared_ptr<Oprand> r15d() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r15d");
        return oprand;
    }

    //栈顶和栈底
    static std::shared_ptr<Oprand> esp() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("esp");
        return oprand;
    }

    static std::shared_ptr<Oprand> ebp() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("ebp");
        return oprand;
    }

    static std::vector<std::shared_ptr<Oprand>> registers32;
    static std::vector<std::shared_ptr<Oprand>> paramRegisters32;
    static std::vector<std::shared_ptr<Oprand>> calleeProtected32;
    static std::vector<std::shared_ptr<Oprand>> callerProtected32;

    //64位寄存器
    //参数用的寄存器，当然也要由caller保护
    static std::shared_ptr<Oprand> rdi() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("rdi", 64);
        return oprand;
    }

    static std::shared_ptr<Oprand> rsi() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("rsi", 64);
        return oprand;
    }

    static std::shared_ptr<Oprand> rdx() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("rdx", 64);
        return oprand;
    }

    static std::shared_ptr<Oprand> rcx() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("rcx", 64);
        return oprand;
    }

    static std::shared_ptr<Oprand> r8() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r8", 64);
        return oprand;
    }

    static std::shared_ptr<Oprand> r9() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r9", 64);
        return oprand;
    }

    //通用寄存器:caller（调用者）负责保护
    static std::shared_ptr<Oprand> r10() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r10", 64);
        return oprand;
    }

    static std::shared_ptr<Oprand> r11() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r11", 64);
        return oprand;
    }
    //返回值，也由Caller保护
    static std::shared_ptr<Oprand> rax() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("rax", 64);
        return oprand;
    }

    //通用寄存器:callee（调用者）负责保护
    static std::shared_ptr<Oprand> rbx() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("rbx", 64);
        return oprand;
    }

    static std::shared_ptr<Oprand> r12() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r12", 64);
        return oprand;
    }

    static std::shared_ptr<Oprand> r13() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r13", 64);
        return oprand;
    }

    static std::shared_ptr<Oprand> r14() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r14", 64);
        return oprand;
    }

    static std::shared_ptr<Oprand> r15() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("r15", 64);
        return oprand;
    }

    //栈顶和栈底
    static std::shared_ptr<Oprand> rsp() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("rsp", 64);
        return oprand;
    }

    static std::shared_ptr<Oprand> rbp() {
        static std::shared_ptr<Oprand> oprand = std::make_shared<Register>("rbp", 64);
        return oprand;
    }

    //64位的可供分配的寄存器
    static std::vector<std::shared_ptr<Oprand>> registers64;
    static std::vector<std::shared_ptr<Oprand>> calleeProtected64;
    static std::vector<std::shared_ptr<Oprand>> callerProtected64;

    std::string toString() override {
        return "%"+ std::any_cast<std::string>(this->value);
    }
};

class MemAddress: public Oprand{
public:
    std::shared_ptr<Register> regist;
    int32_t offset;
    MemAddress(std::shared_ptr<Register>& regist, int32_t offset): Oprand(OprandKind::memory, "undefined"), offset(offset) {}
    std::string toString() override {
        //输出结果类似于：8(%rbp)
        //如果offset为0，那么不显示，即：(%rbp)
        return (this->offset == 0 ? std::string("") : std::to_string(this->offset)) + "(" + this->regist->toString() + ")";
    }
};

class Lower {
public:
     //前一步生成的LIR模型
    std::shared_ptr<AsmModule> asmModule;

    //当前函数使用到的那些Caller保护的寄存器
    std::vector<std::shared_ptr<Oprand>> usedCallerProtectedRegs;

    //当前函数使用到的那些Callee保护的寄存器
    std::vector<std::shared_ptr<Oprand>>  usedCalleeProtectedRegs;

    //所有变量的总数，包括参数、本地变量和临时变量
    uint32_t numTotalVars = 0;

    //当前函数的参数数量
    uint32_t numParams = 0;

    //当前函数的本地变量数量
    uint32_t numLocalVars = 0;

    //临时变量的数量
    uint32_t numTempVars = 0;

    //保存已经被Lower的Oprand，用于提高效率
    std::map<uint32_t, std::shared_ptr<Oprand>> lowedVars;

    //需要在栈里保存的为函数传参（超过6个之后的参数）保留的空间，每个参数占8个字节
    uint32_t numArgsOnStack = 0;

    //rsp应该移动的量。这个量再加8就是该函数所对应的栈桢的大小，其中8是callq指令所压入的返回地址
    int32_t rspOffset = 0;

    //是否使用RedZone，也就是栈顶之外的128个字节
    bool canUseRedZone = false;

    //已被分配的寄存器
    std::map<std::string, uint32_t> allocatedRegisters;

    Lower(std::shared_ptr<AsmModule>& asmModule): asmModule(asmModule) {}

    void lowerModule() {
        std::map<std::string, std::vector<std::shared_ptr<BasicBlock>>> newFun2Code;
        int32_t funIndex = 0;
        for (auto item: this->asmModule->fun2Code){
            auto fun = item.first;
            auto bbs = item.second;
            auto newBBs = this->lowerFunction(fun, bbs, funIndex++);
            newFun2Code.insert({fun, newBBs});
        }
        this->asmModule->fun2Code = newFun2Code;
    }

    std::vector<std::shared_ptr<BasicBlock>> lowerFunction(const std::string& funName, std::vector<std::shared_ptr<BasicBlock>>& bbs, int32_t funIndex) {
        //初始化一些状态变量
        this->initStates(funName);

        //分配寄存器
        this->lowerVars();

        // console.log(this);   //打印一下，看看状态变量是否对。

        //lower每个BasicBlock中的指令
        for (auto& bb: bbs){
            std::vector<std::shared_ptr<Inst>> newInsts;
            this->lowerInsts(bb->insts, newInsts);
            bb->insts = newInsts;
        }

        //添加序曲
        this->addPrologue(bbs[0]->insts);

        //添加尾声
        this->addEpilogue(bbs.back()->insts);

        //基本块的标签和跳转指令。
        auto newBBs = this->lowerBBLabelAndJumps(bbs, funIndex);

        return newBBs;
    }

    void initStates(const std::string& funName) {

    }

    void lowerVars() {

    }

    void lowerInsts(const std::vector<std::shared_ptr<Inst>>& insts, std::vector<std::shared_ptr<Inst>>& newInsts) {

    }

    void addPrologue(std::vector<std::shared_ptr<Inst>>& newInsts) {

    }

    void addEpilogue(std::vector<std::shared_ptr<Inst>>& newInsts) {

    }

    std::vector<std::shared_ptr<BasicBlock>> lowerBBLabelAndJumps(std::vector<std::shared_ptr<BasicBlock>>& bbs, int32_t funIndex) {
        return {};
    }
};




std::string compileToAsm(AstNode& node, bool verbose = true);

#endif