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
#include <climits>

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

    //伪指令
    declVar,   //变量声明
    reload,    //重新装载被溢出到内存的变量到寄存器
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
    label,          //标签，从bb类型的操作数Lower而成

    //cmp指令的结果，是设置寄存器的标志位
    //后面可以根据flag和比较操作符的类型，来确定后续要生成的代码
    flag,
};


std::string toString(AsmOpCode op);
std::string toString(OprandKind kind);

class OpCodeHelper{
public:
    static bool isReturn(AsmOpCode op){
        return op == AsmOpCode::retb || op == AsmOpCode::retw || op == AsmOpCode::retl ||  op == AsmOpCode::retq;
    }

    static bool isJump(AsmOpCode op){
        return op < AsmOpCode::sete;
    }
};

class Oprand {
public:
    OprandKind kind;
    std::any value;
    std::string name;
    Oprand(OprandKind kind, std::any value, std::string name = ""): kind(kind), value(value), name(name) {}

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
        else if (this->kind == OprandKind::label){
                return value2String();
        }
        else if (this->kind == OprandKind::varIndex){
                return "var" + value2String();
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
    uint32_t numOprands;
    static uint32_t index;
    std::string comment;

    virtual bool isInst_0() { return false; };
    virtual bool isInst_1() { return false; };
    virtual bool isInst_2() { return false; };

    Inst(AsmOpCode op, uint32_t numOprands):op(op), numOprands(numOprands) {
        index++;
    }
    virtual std::string toString() = 0;
    virtual std::string patchComments(std::string& str) {
        if(!this->comment.empty()){
            if (str.size() < 11)
                str += "\t\t\t";
            else if (str.size() < 19)
                str += "\t\t";
            else if (str.size() < 21)
                str += "\t";

            str += (this->comment.empty() ? "" : ("\t\t#  " + this->comment));
        }
        return str;
    }
};

class Inst_0: public Inst{
public:
    Inst_0(AsmOpCode op): Inst(op, 0) {}

    bool isInst_0() { return true; };

    std::string toString() override {
        auto str = ::toString(this->op);
        return this->patchComments(str);
    }
};

class Inst_1: public Inst{
public:
    std::shared_ptr<Oprand> oprand;
    Inst_1(AsmOpCode op, std::shared_ptr<Oprand>& oprand):Inst(op, 1), oprand(oprand) {}

    bool isInst_1() { return true; };
    std::string toString() override {
        auto str = ::toString(this->op) + "\t" + this->oprand->toString();
        return this->patchComments(str);
    }

};

class Inst_2: public Inst {
public:
    std::shared_ptr<Oprand> oprand1;
    std::shared_ptr<Oprand> oprand2;
    Inst_2(AsmOpCode op, std::shared_ptr<Oprand>& oprand1, std::shared_ptr<Oprand>& oprand2):Inst(op, 2), oprand1(oprand1), oprand2(oprand2) {}

    bool isInst_2() { return true; };
    std::string toString() override {
        auto str = ::toString(this->op) + "\t" + this->oprand1->toString() + ", " + this->oprand2->toString();
        return this->patchComments(str);
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
        else if (this->bbIndex != -1){
            return "LBB" + std::to_string(this->bbIndex);
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

/**
 * 变量活跃性分析的结果
 */
struct LivenessResult {
    std::map<std::shared_ptr<Inst>, std::set<uint32_t>> liveVars;
    std::map<std::shared_ptr<BasicBlock>, std::set<uint32_t>> initialVars;
};

class AsmModule{
public:
    //每个函数对应的指令数组
    std::map<std::string, std::vector<std::shared_ptr<BasicBlock>>>  fun2Code;


     std::map<std::string, uint32_t> numParams;  //每个函数的参数
     std::map<std::string, uint32_t> numVars;  // 每个函数的变量数，包括参数、本地变量
     std::map<std::string, uint32_t> numTotalVars; //每个函数的总变量数，包括参数、本地变量和临时变量

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


    //每个表达式节点对应的临时变量的索引
    // tempVarMap:Map<Expression, number> = new Map();

    //主要用于判断当前的Unary是一个表达式的一部分，还是独立的一个语句
    bool inExpression {false};

    //保存一元后缀运算符对应的指令。
    // postfixUnaryInst:Inst_1|null = null;

    //当前的BasicBlock编号
    uint32_t blockIndex {0};
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

    std::shared_ptr<Oprand> allocateTempVar() {
        uint32_t varIndex = this->s->nextTempVarIndex++;
        auto oprand = std::make_shared<Oprand>(OprandKind::varIndex, varIndex);
        //这里要添加一个变量声明
        auto inst = std::make_shared<Inst_1>(AsmOpCode::declVar,oprand);
        this->getCurrentBB()->insts.push_back(inst);
        return oprand;
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
        bb->bbIndex = this->s->blockIndex++;
        this->s->bbs.push_back(bb);

        return bb;
    }

    std::any visitProg(Prog& prog, std::string prefix) override {
        //设置一些状态变量
        this->asmModule = std::make_shared<AsmModule>();

        this->s->functionSym = prog.sym;
        this->s->nextTempVarIndex = this->s->functionSym->vars.size();

        //创建新的基本块
        this->newBlock();

        this->visitBlock(prog);
        this->asmModule->fun2Code.insert({this->s->functionSym->name, this->s->bbs});
        this->asmModule->numParams.insert({this->s->functionSym->name, this->s->functionSym->getNumParams()});
        this->asmModule->numVars.insert({this->s->functionSym->name, this->s->functionSym->vars.size()});
        this->asmModule->numTotalVars.insert({this->s->functionSym->name, this->s->nextTempVarIndex});


        //重新设置状态变量
        this->s = std::make_shared<TempStates>();

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
    }
    */

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
            this->getCurrentBB()->insts.push_back(std::make_shared<Inst_1>(AsmOpCode::declVar, left));

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
        this->asmModule->numVars.insert({this->s->functionSym->name, this->s->functionSym->vars.size()});
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
        std::shared_ptr<Oprand> dest;
        if(functionType->returnType != SysTypes::Void()) { //函数有返回值时
            dest = this->allocateTempVar();
            insts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl, this->returnSlot, dest));
        }

        dbg("--------- inst.size " + std::to_string(insts.size()));

        //调用函数完毕以后，要重新装载被Spilled的变量
        //这个动作要在获取返回值之后
        insts.push_back(std::make_shared<Inst_0>(AsmOpCode::reload));

        return dest;
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
        if (bi.op == Op::Plus || bi.op == Op::Minus || bi.op == Op::Multiply || bi.op == Op::Divide) {
            if (!this->isTempVar(dest)){
                dest = this->allocateTempVar();
                insts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl, left, dest));
            }
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
                this->movIfNotSame(right, dest);
                // insts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl,right, dest));
                // this->movIfNotSame(dest,left);    //写到内存里去
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
    Register(const std::string registerName, uint32_t bits = 32) : Oprand(OprandKind::regist, registerName, registerName),
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
    std::shared_ptr<Oprand> regist;
    int32_t offset;
    MemAddress(std::shared_ptr<Oprand>& regist, int32_t offset): Oprand(OprandKind::memory, "undefined"), regist(regist), offset(offset) {}
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

    //变量活跃性分析的结果
    std::shared_ptr<LivenessResult> livenessResult;

    //To Delete 当前函数使用到的那些Caller保护的寄存器
    std::vector<std::shared_ptr<Oprand>> usedCallerProtectedRegs;

    //当前函数使用到的那些Callee保护的寄存器
    std::vector<std::shared_ptr<Oprand>>  usedCalleeProtectedRegs;

    //To Delete 所有变量的总数，包括参数、本地变量和临时变量
    uint32_t numTotalVars = 0;

    //当前函数的参数数量
    uint32_t numParams = 0;

    //当前函数的本地变量数量
    uint32_t numLocalVars = 0;

    //临时变量的数量
    uint32_t numTempVars = 0;

    //To Delete 保存已经被Lower的Oprand，用于提高效率
    std::map<uint32_t, std::shared_ptr<Oprand>> lowedVars;


    std::map<uint32_t, std::shared_ptr<Oprand>> loweredVars;

    //需要在栈里保存的为函数传参（超过6个之后的参数）保留的空间，每个参数占8个字节
    uint32_t numArgsOnStack = 0;

    //rsp应该移动的量。这个量再加8就是该函数所对应的栈桢的大小，其中8是callq指令所压入的返回地址
    int32_t rspOffset = 0;

    //是否使用RedZone，也就是栈顶之外的128个字节
    bool canUseRedZone = false;

    //To Delete 已被分配的寄存器
    std::map<std::string, uint32_t> allocatedRegisters;

    //预留的寄存器。
    //主要用于在调用函数前，保护起那些马上就要用到的寄存器，不再分配给其他变量。
    std::set<std::shared_ptr<Oprand>> reservedRegisters;

    //spill的register在内存中的位置。
    uint32_t spillOffset = 0;

    //被spill的变量
    //key是varIndex，value是内存地址
    std::map<uint32_t, std::shared_ptr<Oprand>> spilledVars2Address;

    //key是varIndex，value是原来的寄存器
    std::map<uint32_t, std::shared_ptr<Oprand>> spilledVars2Reg;

    Lower(std::shared_ptr<AsmModule>& asmModule, std::shared_ptr<LivenessResult>& livenessResult):
        asmModule(asmModule), livenessResult(livenessResult) {}

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
        this->usedCalleeProtectedRegs.clear();
        this->usedCallerProtectedRegs.clear();
        this->numTotalVars = this->asmModule->numTotalVars[funName];
        this->numParams = this->asmModule->numParams[funName];
        this->numLocalVars = this->asmModule->numVars[funName] - this->numParams;
        this->numTempVars = this->numTotalVars - this->asmModule->numVars[funName];
        this->numArgsOnStack = 0;
        this->rspOffset = 0;
        this->lowedVars.clear();
        this->allocatedRegisters.clear();

        //是否可以使用RedZone
        //需要是叶子函数，并且对栈外空间的使用量小于128个字节，也就是32个整数
        this->canUseRedZone = false;
        bool isLeafFunction = this->asmModule->isLeafFunction[funName];
        if (isLeafFunction){
            uint32_t paramsToSave = (this->numParams > 6) ? 6: this->numParams;
            uint32_t bytes = paramsToSave * 4 + this->numLocalVars * 4 + Register::calleeProtected32.size() * 8;
            this->canUseRedZone = bytes < 128u;
        }
        return;
    }

    void lowerParams(){
        for (uint32_t i = 0; i< this->numParams; i++){
            if (i < 6u){
                auto reg = Register::paramRegisters32[i];
                this->assignRegToVar(i, reg);
            }
            else{
                //从Caller的栈里访问参数
                int32_t offset = (static_cast<int32_t>(i) - 6) * 8 + 16;  //+16是因为有一个callq压入的返回地址，一个pushq rbp又加了8个字节
                auto rbp = Register::rbp();
                auto oprand = std::make_shared<MemAddress>(rbp, offset);
                this->loweredVars.insert({i, oprand});
            }
        }
    }


    void lowerVars() {
        uint32_t paramsToSave = this->numParams > 6 ? 6: this->numParams;

        //处理参数
        for (uint32_t varIndex = 0; varIndex < this->numTotalVars; varIndex++){
            std::shared_ptr<Oprand> newOprand;
            if (varIndex < this->numParams){
                if (varIndex < 6){
                    //从自己的栈桢里访问。在程序的序曲里，就把这些变量拷贝到栈里了。
                    int32_t offset = -(varIndex + 1) * 4;
                    auto oprand = Register::rbp();
                    newOprand = std::make_shared<MemAddress>(oprand, offset);
                }
                else{
                    //从Caller的栈里访问参数
                    int32_t offset = (varIndex - 6) * 8 + 16;  //+16是因为有一个callq压入的返回地址，一个pushq rbp又加了8个字节
                    auto oprand = Register::rbp();
                    newOprand = std::make_shared<MemAddress>(oprand, offset);
                }
            }
            //本地变量，在栈桢里。
            else if (varIndex < this->numParams + this->numLocalVars) {
                int32_t offset = -(varIndex - this->numParams + paramsToSave + 1) * 4;
                auto oprand = Register::rbp();
                newOprand = std::make_shared<MemAddress>(oprand, offset);
            }
            //临时变量，分配寄存器
            else{
                newOprand = this->allocateRegister(varIndex);
            }

            //缓存起来
            this->lowedVars.insert({varIndex, newOprand});
        }
    }

    std::shared_ptr<Oprand> allocateRegister(uint32_t varIndex) {
        for (auto& reg: Register::registers32){
            auto it = this->allocatedRegisters.find(reg->name);
            if (it == this->allocatedRegisters.end()){
                this->allocatedRegisters.insert({reg->name, varIndex});

                //更新usedCalleeProtectedRegs
                if(std::find(Register::calleeProtected32.begin(), Register::calleeProtected32.end(), reg) !=
                    Register::calleeProtected32.end()){
                    this->usedCalleeProtectedRegs.push_back(reg);
                }
                //更新usedCallerProtectedRegs
                else if (std::find(Register::callerProtected32.begin(), Register::callerProtected32.end(), reg) !=
                    Register::callerProtected32.end()){
                    this->usedCallerProtectedRegs.push_back(reg);
                }
                return reg;
            }
        }
        //不应该执行到这里，执行到这里应该报错
        dbg("Error: Unable to allocate a Register, the generated asm is not reliable");
        return nullptr;
    }

    void reserveReturnSlot(){
        //给予一个特殊的变量下标，-1
        this->allocatedRegisters.insert({Register::eax()->name, UINT_MAX});
    }

    void freeRegister(std::shared_ptr<Oprand>& reg){
        auto it = this->allocatedRegisters.find(reg->name);
        if (it != this->allocatedRegisters.end())
            this->allocatedRegisters.erase(it);

        auto iter1 = std::find(this->usedCalleeProtectedRegs.begin(), this->usedCalleeProtectedRegs.end(), reg);
        if (iter1 != this->usedCalleeProtectedRegs.end()) {
            this->usedCalleeProtectedRegs.erase(iter1);
        }

        auto iter2 = std::find(this->usedCallerProtectedRegs.begin(), this->usedCallerProtectedRegs.end(), reg);
        if (iter2 != this->usedCallerProtectedRegs.end()) {
            this->usedCallerProtectedRegs.erase(iter2);
        }
    }

    void lowerInsts(const std::vector<std::shared_ptr<Inst>>& insts, std::vector<std::shared_ptr<Inst>>& newInsts) {
        for (uint32_t i = 0; i < insts.size(); i++) {
            auto inst = insts[i];
            //两个操作数
            if (inst->isInst_2()) {
                auto inst_2 = std::dynamic_pointer_cast<Inst_2>(inst);
                inst_2->oprand1 = this->lowerOprand(inst_2->oprand1);
                inst_2->oprand2 = this->lowerOprand(inst_2->oprand2);
                // 对mov再做一次优化
                if (!(inst_2->op == AsmOpCode::movl && inst_2->oprand1 == inst_2->oprand2)) {
                    newInsts.push_back(inst_2);
                }
            }
            //1个操作数
            else if (inst->isInst_1()) {
                auto inst_1 = std::dynamic_pointer_cast<Inst_1>(inst);
                inst_1->oprand = this->lowerOprand(inst_1->oprand);
                //处理函数调用
                //函数调用前后，要设置参数；
                if (inst_1->op == AsmOpCode::callq) {
                    this->lowerFunctionCall(inst_1, newInsts);
                }
                else {
                    newInsts.push_back(inst_1);
                }
            }
            //没有操作数
            else {
                newInsts.push_back(inst);
            }
        }
    }

    void lowerFunctionCall(std::shared_ptr<Inst_1>& inst_1, std::vector<std::shared_ptr<Inst>>& newInsts) {
        auto functionOprand = std::dynamic_pointer_cast<FunctionOprand>(inst_1->oprand);
        auto& args = functionOprand->args;
        //需要在栈桢里为传参保留的空间
        uint32_t numArgs = args.size();
        if (numArgs > 6 && numArgs - 6 > this->numArgsOnStack) {
            this->numArgsOnStack = numArgs - 6;
        }
        //保存Caller负责保护的寄存器
        uint32_t paramsToSave = this->numParams > 6 ? 6 : this->numParams;
        int32_t offset = -(paramsToSave + this->numLocalVars + 1) * 4;
        std::vector<uint32_t> spilledTempVars;
        std::vector<std::shared_ptr<Oprand>> spilledRegs;
        for (uint32_t i = 0; i < this->usedCallerProtectedRegs.size(); i++) {
            auto reg = this->usedCallerProtectedRegs[i];
            auto rbp = Register::rbp();
            std::shared_ptr<Oprand> op = std::make_shared<MemAddress>(rbp, offset - i * 4);
            newInsts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl, reg, op));
            auto varIndex = this->allocatedRegisters[reg->name];
            spilledRegs.push_back(reg);
            spilledTempVars.push_back(varIndex);
        }

        for (auto reg: spilledRegs) { //这个一定要单起一个循环
            this->freeRegister(reg);
        }

        //把前6个参数设置到寄存器
        for (uint32_t j = 0; j < numArgs && j < 6; j++) {
            auto regSrc = this->lowerOprand(args[j]);
            auto regDest = Register::paramRegisters32[j];
            if (regDest != regSrc)
                newInsts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl, regSrc, regDest));
        }

        //超过6个之后的参数是放在栈桢里的，并要移动栈顶指针
        if (args.size() > 6) {
            //参数是倒着排的。
            //栈顶是参数7，再往上，依次是参数8、参数9...
            //在Callee中，会到Caller的栈桢中去读取参数值
            for (uint32_t j = 6; j < numArgs; j++) {
                int32_t offset = (j - 6) * 8;
                auto rsp = Register::rsp();
                std::shared_ptr<Oprand> op = std::make_shared<MemAddress>(rsp, offset);
                newInsts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl, functionOprand->args[j], op));
            }
        }

        //调用函数，修改操作数为functionName
        newInsts.push_back(inst_1);
        //为返回值预留eax寄存器
        this->reserveReturnSlot();
        //恢复Caller负责保护的寄存器
        for (uint32_t i = 0; i < spilledTempVars.size(); i++) {
            uint32_t varIndex = spilledTempVars[i];
            auto reg = this->allocateRegister(varIndex);
            auto rbp = Register::rbp();
            std::shared_ptr<Oprand> op = std::make_shared<MemAddress>(rbp, offset - i * 4);
            newInsts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl, op, reg));
            this->lowedVars.insert({varIndex, reg});
        }

    }

    void addPrologue(std::vector<std::shared_ptr<Inst>>& insts) {
        std::vector<std::shared_ptr<Inst>> newInsts;
        //保存rbp的值
        auto rbp = Register::rbp();
        newInsts.push_back(std::make_shared<Inst_1>(AsmOpCode::pushq, rbp));
        //把原来的栈顶保存到rbp,成为现在的栈底
        auto rsp = Register::rsp();
        newInsts.push_back(std::make_shared<Inst_2>(AsmOpCode::movq, rsp, rbp));
        //把前6个参数存到栈桢里
        int32_t paramsToSave = this->numParams > 6 ? 6 : this->numParams;
        for (int32_t i = 0; i < paramsToSave; i++) {
            int32_t offset = -(i + 1) * 4;
            auto src = Register::paramRegisters32[i];
            std::shared_ptr<Oprand> dst = std::make_shared<MemAddress>(rbp, offset);
            newInsts.push_back(std::make_shared<Inst_2>(AsmOpCode::movl, src, dst));
        }

        //计算栈顶指针需要移动多少位置
        //要保证栈桢16字节对齐
        if (!this->canUseRedZone) {
            this->rspOffset = paramsToSave * 4 + this->numLocalVars * 4 + this->usedCallerProtectedRegs.size() * 4 + this->numArgsOnStack * 8 + 16;
            //当前占用的栈空间，还要加上Callee保护的寄存器占据的空间
            auto rem = (this->rspOffset + this->usedCalleeProtectedRegs.size() * 8) % 16;
            // console.log("this->rspOffset="+this->rspOffset);
            // console.log("rem="+rem);
            if (rem == 8) {
                this->rspOffset += 8;
            }
            else if (rem == 4) {
                this->rspOffset += 12;
            }
            else if (rem == 12) {
                this->rspOffset += 4;
            }
            // console.log("this->rspOffset="+this->rspOffset);
            if (this->rspOffset > 0) {
                auto op = std::make_shared<Oprand>(OprandKind::immediate, this->rspOffset);
                newInsts.push_back(std::make_shared<Inst_2>(AsmOpCode::subq, op, rsp));
            }

        }

        //保存Callee负责保护的寄存器
        this->saveCalleeProtectedRegs(newInsts);
        //合并原来的指令
        insts.insert(insts.begin(), newInsts.begin(), newInsts.end());
    }

    void addEpilogue(std::vector<std::shared_ptr<Inst>>& newInsts) {
        //恢复Callee负责保护的寄存器
        this->restoreCalleeProtectedRegs(newInsts);
        //缩小栈桢
        if (!this->canUseRedZone && this->rspOffset > 0) {
            auto src = std::make_shared<Oprand>(OprandKind::immediate, this->rspOffset);
            auto dst = Register::rsp();
            newInsts.push_back(std::make_shared<Inst_2>(AsmOpCode::addq, src, dst));
        }
        //恢复rbp的值
        auto rbp = Register::rbp();
        newInsts.push_back(std::make_shared<Inst_1>(AsmOpCode::popq, rbp));
        //返回
        newInsts.push_back(std::make_shared<Inst_0>(AsmOpCode::retq));
    }

    std::vector<std::shared_ptr<BasicBlock>> lowerBBLabelAndJumps(std::vector<std::shared_ptr<BasicBlock>>& bbs, int32_t funIndex) {
        std::vector<std::shared_ptr<BasicBlock>> newBBs;
        uint32_t bbIndex = 0;
        //去除空的BasicBlock，并给BasicBlock编号
        for (uint32_t i = 0; i < bbs.size(); i++) {
            auto bb = bbs[i];
            //如果是空的BasicBlock，就跳过
            if (bb->insts.size() > 0) {
                bb->funIndex = funIndex;
                bb->bbIndex = bbIndex++;
                newBBs.push_back(bb);
            }
            else {
                //如果有一个BasicBlock指向该block，那么就指向下一个block;
                // for (uint32_t j = 0; j < newBBs.size(); j++) {
                    // auto lastInst = newBBs[j]->insts.back();
                    // if (lastInst->op < AsmOpCode::sete) {
                        // auto jumpInst = lastInst;
                        // auto destBB = jumpInst->oprand.value;
                        // if (destBB == bb) {
                            // jumpInst.oprand.value = bbs[i + 1];
                        // }
                    // }
                // }
                dbg("Error: current not support jmp!");
            }
        }
        //把jump指令的操作数lower一下,从BasicBlock变到标签
        // for (uint32_t i = 0; i < newBBs.size(); i++) {
            // auto& insts = newBBs[i]->insts;
            // auto lastInst = insts.back();
            // if (lastInst.op < 20) { //jump指令
                // auto jumpInst = lastInst;
                // let bbDest = jumpInst.oprand.value;
                // jumpInst.oprand.value = bbDest->getName();
                // bbDest.isDestination = true; //有其他block跳到这个block
            // }
        // }
        return newBBs;
    }


    std::shared_ptr<Oprand> lowerOprand(std::shared_ptr<Oprand>& oprand) {
        auto newOprand = oprand;
        if (oprand->kind == OprandKind::varIndex) {
            if (!isType<uint32_t>(oprand->value)) {
                dbg("Error: OprandKind::varIndex value must uint32_t, but " + std::string(oprand->value.type().name()));
            }
            uint32_t varIndex = std::any_cast<uint32_t>(oprand->value);
            auto iter = this->lowedVars.find(varIndex);
            return iter->second;
        }
        else if (oprand->kind == OprandKind::returnSlot) {
            newOprand = Register::eax();
        }
        return newOprand;
    }

    std::shared_ptr<Oprand> spillVar(uint32_t varIndex, std::shared_ptr<Oprand>& reg, std::vector<std::shared_ptr<Inst>>& newInsts) {
        std::shared_ptr<Oprand> address;
        auto it = this->spilledVars2Address.find(varIndex);
        if(it != this->spilledVars2Address.end()){
            address = this->spilledVars2Address[varIndex];
        }
        else{
            this->spillOffset += 4;
            auto rbp = Register::rbp();
            address = std::make_shared<MemAddress>(rbp, -1 * this->spillOffset);
            this->spilledVars2Address.insert({varIndex, address});
            this->spilledVars2Reg.insert({varIndex, reg});
        }

        auto inst = std::make_shared<Inst_2>(AsmOpCode::movl, reg, address);
        inst->comment = "spill\tvar" + std::to_string(varIndex);
        newInsts.push_back(inst);

        this->loweredVars.insert({varIndex, address});
        return address;
    }

    std::shared_ptr<Oprand> reloadVar(uint32_t varIndex, std::vector<std::shared_ptr<Inst>>& newInsts) {
        auto it = this->loweredVars.find(varIndex);
        if (it == this->loweredVars.end()) {
            dbg("Error: can't find varIndex: " + std::to_string(varIndex));
            return nullptr;
        }

        auto oprand = it->second;
        if (oprand->kind == OprandKind::memory){
            auto iter = this->spilledVars2Reg.find(varIndex);
            if (iter == this->spilledVars2Reg.end()) {
                dbg("Error: can't find varIndex: " + std::to_string(varIndex));
                return nullptr;
            }

            auto reg = iter->second;
            //查看该reg是否正在被其他变量占用
            for (auto& item: this->loweredVars){
                auto& oprand1 = item.second;
                if (oprand1 == reg){
                    this->spillVar(varIndex, oprand1, newInsts);
                    break;
                }
            }
            this->assignRegToVar(varIndex, reg);

            auto inst = std::make_shared<Inst_2>(AsmOpCode::movl, oprand, reg);
            inst->comment = "reload\tvar" + std::to_string(varIndex);
            newInsts.push_back(inst);

            return reg;
        }
        return nullptr;
    }

    void assignRegToVar(uint32_t varIndex, std::shared_ptr<Oprand>& reg) {
        //更新usedCalleeProtectedRegs
        auto it1 = std::find(Register::calleeProtected32.begin(), Register::calleeProtected32.end(), reg);
        auto it2 = std::find(this->usedCalleeProtectedRegs.begin(), this->usedCalleeProtectedRegs.end(), reg);
        if(it1 != Register::calleeProtected32.end() && it2 == this->usedCalleeProtectedRegs.end()){
            this->usedCalleeProtectedRegs.push_back(reg);
        }
        //更新loweredVars
        this->loweredVars.insert({varIndex, reg});
    }

    std::shared_ptr<Oprand> spillARegister(std::vector<std::shared_ptr<Inst>>& newInsts) {
        for (auto& item: this->loweredVars){
            auto varIndex = item.first;
            auto oprand = item.second;

            if (oprand->kind == OprandKind::regist &&
                this->reservedRegisters.find(oprand) != this->reservedRegisters.end()){
                this->spillVar(varIndex, oprand, newInsts);
            }
        }

        //理论上，不会到达这里。
        return nullptr;
    }

    std::shared_ptr<Oprand> getFreeRegister(std::vector<uint32_t>& liveVars) {
        std::shared_ptr<Oprand> result;

        //1.从空余的寄存器中寻找一个。
        std::set<std::shared_ptr<Oprand>> allocatedRegisters;
        for (auto& item: this->loweredVars) {
            auto varIndex = item.first;
            auto oprand = item.second;

            if(oprand->kind == OprandKind::regist){
                allocatedRegisters.insert(oprand);
            }
            else{
                auto iter = this->spilledVars2Reg.find(varIndex);
                if (iter == this->spilledVars2Reg.end()) {
                    dbg("Error: get spilledVars2Reg failed, varIndex: " + std::to_string(varIndex));
                }

                allocatedRegisters.insert(iter->second);
            }
        }

        for (auto reg: Register::registers32){
            if (allocatedRegisters.find(reg) == allocatedRegisters.end() &&
                this->reservedRegisters.find(reg) == this->reservedRegisters.end()) {
                result = reg;
                break;
            }
        }

        return result;

        //2.从已分配的varIndex里面找一个
        // if (result == null){
        //     for (let varIndex of this->loweredVars.keys()){
        //         // todo 下面的逻辑是不安全的，在存在cfg的情况下，不能简单的判断变量是否真的没用了。
        //         if (liveVars.indexOf(varIndex) == -1){
        //             let oprand = this->loweredVars.get(varIndex) as Oprand;
        //             if (oprand.kind == OprandKind.register && this->reservedRegisters.indexOf(oprand as Register)==-1){
        //                 result = oprand as Register;
        //                 this->loweredVars.delete(varIndex);
        //                 break;
        //             }
        //         }
        //     }
        // }
    }

    void saveCalleeProtectedRegs(std::vector<std::shared_ptr<Inst>>& newInsts) {
        for (uint32_t i = 0; i  < this->usedCalleeProtectedRegs.size(); i++) {
            auto reg64 = std::find(Register::calleeProtected32.begin(),Register::calleeProtected32.end(), this->usedCalleeProtectedRegs[i]);
            newInsts.push_back(std::make_shared<Inst_1>(AsmOpCode::pushq, *reg64));
        }
    }

    void restoreCalleeProtectedRegs(std::vector<std::shared_ptr<Inst>>& newInsts) {
        for (int32_t i = this->usedCalleeProtectedRegs.size() - 1; i >= 0; i--) {
            auto reg64 = std::find(Register::calleeProtected32.begin(),Register::calleeProtected32.end(), this->usedCalleeProtectedRegs[i]);
            newInsts.push_back(std::make_shared<Inst_1>(AsmOpCode::popq, *reg64));
        }
    }
};

/**
 * 控制流图
 */
class CFG{
public:
    //基本块的列表。第一个和最后一个BasicBlock是图的root。
    std::vector<std::shared_ptr<BasicBlock>> bbs;

    //每个BasicBlock输出的边
    std::map<std::shared_ptr<BasicBlock>, std::vector<std::shared_ptr<BasicBlock>>> edgesOut;

    //每个BasicBlock输入的边
    std::map<std::shared_ptr<BasicBlock>, std::vector<std::shared_ptr<BasicBlock>>> edgesIn;

    CFG(std::vector<std::shared_ptr<BasicBlock>>& bbs): bbs(bbs) {
        this->buildCFG();
    }

    void buildCFG(){

        //构建edgesOut;
        for (int32_t i = 0; i < static_cast<int32_t>(this->bbs.size()) - 1; i++){ //最后一个基本块不用分析
            auto bb = this->bbs[i];
            auto next = (i + 1 == static_cast<int32_t>(this->bbs.size())) ? nullptr : this->bbs[i+1];

            std::vector<std::shared_ptr<BasicBlock>> toBBs;

            this->edgesOut.insert({bb, toBBs});
            auto lastInst = bb->insts.back();
            if (OpCodeHelper::isJump(lastInst->op)){
                auto jumpInst = std::dynamic_pointer_cast<Inst_1>(lastInst);
                auto destBB = std::any_cast<std::shared_ptr<BasicBlock>>(jumpInst->oprand->value);
                toBBs.push_back(destBB);
                //如果是条件分枝，那么还要加上下面紧挨着的BasicBlock
                if (jumpInst->op != AsmOpCode::jmp){
                    toBBs.push_back(next);
                }
            }
            else{ //如果最后一条语句不是跳转语句，则连接到下一个BB
                toBBs.push_back(next);
            }

        }


        //构建反向的边:edgesIn
        for (const auto& item: this->edgesOut){
            const auto& bb = item.first;
            const auto& toBBs = item.second;
            for (auto& toBB: toBBs){
                auto& fromBBs = this->edgesIn[toBB];
                fromBBs.push_back(bb);
            }
        }

    }

    std::string toString() {
        std::string str;
        str += "bbs:\n";
        for (auto& bb: this->bbs){
            str += "\t" + bb->getName() + "\n";
        }

        str += "edgesOut:\n";
        for (auto& item: this->edgesOut){
            auto& bb = item.first;
            auto& toBBs = item.second;

            str += "\t" + bb->getName()+"->\n";
            for (auto& bb2: toBBs){
                str += "\t\t" + bb2->getName() + "\n";
            }
        }

        str += "edgesIn:\n";
        for (auto& item: this->edgesIn){
            auto& bb = item.first;
            auto& fromBBs = item.second;

            str += "\t"+bb->getName()+"<-\n";
            for (auto& bb2: fromBBs){
                str += "\t\t" + bb2->getName() + "\n";
            }
        }
        return str;
    }
};


class LivenessAnalyzer{
public:
    std::shared_ptr<AsmModule> asmModule;

    LivenessAnalyzer(std::shared_ptr<AsmModule>& asmModule): asmModule(asmModule) {}

    std::shared_ptr<LivenessResult> execute() {
        auto result = std::make_shared<LivenessResult>();

        for (auto& item: this->asmModule->fun2Code){
            auto& bbs = item.second;
            this->analyzeFunction(bbs, result);
        }

        return result;
    }

    void analyzeFunction(std::vector<std::shared_ptr<BasicBlock>>& bbs, std::shared_ptr<LivenessResult>& result) {
        auto cfg = std::make_shared<CFG>(bbs);
        Print(cfg->toString());

        //做一些初始化工作
        for (auto& bb: bbs){
            result->initialVars[bb].insert({});
        }

        //持续遍历图，直到没有BasicBlock的活跃变量需要被更新
        std::set<std::shared_ptr<BasicBlock>> bbsToDo;
        bbsToDo.insert(bbs.front());
        while (!bbsToDo.empty()){
            auto bb = *(bbsToDo.begin());
            bbsToDo.erase(bb);
            this->analyzeBasicBlock(bb, result);
            //取出第一行的活跃变量集合，作为对前面的BasicBlock的输入
            std::set<uint32_t> liveVars;
            if (!bb->insts.empty()) {
                liveVars = result->liveVars[bb->insts.front()];
            }


            auto fromBBs = cfg->edgesIn.find(bb);
            if (fromBBs != cfg->edgesIn.end()){
                for (auto& bb2: fromBBs->second){
                    auto liveVars2 = result->initialVars[bb2];
                    //如果能向上面的BB提供不同的活跃变量，则需要重新分析bb2
                    if (!this->isSubsetOf(liveVars, liveVars2)){
                        if (bbsToDo.find(bb2) == bbsToDo.end()) {
                            bbsToDo.insert(bb2);
                        }

                        auto unionVars = this->unionOf(liveVars, liveVars2);
                        result->initialVars.insert({bb2, unionVars});
                    }
                }
            }

        }

    }


    /**
     * 给基本块做活跃性分析。
     * 算法：从基本块的最后一条指令倒着做分析。
     * @param bb
     * @param result
     */
    bool analyzeBasicBlock(std::shared_ptr<BasicBlock>& bb, std::shared_ptr<LivenessResult>& result) {
        bool changed = false;

        //找出BasicBlock初始的集合
        auto vars = result->initialVars[bb];  //克隆一份

        //为每一条指令计算活跃变量集合
        for (auto it = bb->insts.rbegin(); it != bb->insts.rend(); ++it){
            auto inst = *it;
            if (inst->numOprands == 1){
                auto inst_1 = std::dynamic_pointer_cast<Inst_1>(inst);
                //变量声明伪指令，从liveVars集合中去掉该变量
                if (inst_1->op == AsmOpCode::declVar){
                    if (!isType<uint32_t>(inst_1->oprand->value)) {
                        dbg("Error: analyzeBasicBlock expect uint32_t, but " + std::string(inst_1->oprand->value.type().name()));
                        return false;
                    }

                    uint32_t varIndex = std::any_cast<uint32_t>(inst_1->oprand->value);
                    auto iter = vars.find(varIndex);
                    if (iter != vars.end()) {
                        vars.erase(iter);
                    }
                }
                //查看指令中引用了哪个变量，就加到liveVars集合中去
                else{
                    this->updateLiveVars(inst, inst_1->oprand, vars);
                }
            }
            else if (inst->numOprands == 2){
                auto inst_2 = std::dynamic_pointer_cast<Inst_2>(inst);
                this->updateLiveVars(inst, inst_2->oprand1, vars);
                this->updateLiveVars(inst, inst_2->oprand2, vars);
            }

            result->liveVars.insert({inst, vars});
            //vars 重复利用，用于下一条指令
        }

        return changed;
    }

    /**
     * set1是不是set2的子集
     * @param set1
     * @param set2
     */
    bool isSubsetOf(const std::set<uint32_t>& set1, const std::set<uint32_t> set2) {
        if(set1.size() <= set2.size()){
            for (auto n:  set1){
                if (set2.find(n) == set2.end()){
                    return false;
                }
            }
            return true;
        }
        else{
            return false;
        }
    }

    /**
     * 返回set1和set2的并集
     * @param set1
     * @param set2
     */
    std::set<uint32_t> unionOf(const std::set<uint32_t>& set1, const std::set<uint32_t>& set2) {
        std::set<uint32_t> result;
        std::set_union(set1.begin(), set1.end(), set2.begin(), set2.end(), std::inserter(result, result.begin()));
        return result;
    }

    /**
     * 把操作数用到的变量增加到当前指令的活跃变量集合里面。
     * @param inst
     * @param oprand
     * @param vars
     */
    void updateLiveVars(std::shared_ptr<Inst>& inst, std::shared_ptr<Oprand>& oprand, std::set<uint32_t>& vars){
        if (oprand->kind == OprandKind::varIndex){
            if (!isType<uint32_t>(oprand->value)) {
                dbg("Error: updateLiveVars expect uint32_t, but " + std::string(oprand->value.type().name()));
                return;
            }

            uint32_t varIndex = std::any_cast<uint32_t>(oprand->value);
            if (vars.find(varIndex)== vars.end()){
                vars.insert(varIndex);
            }
        }
        else if (oprand->kind == OprandKind::function){
            auto functionOprand = std::dynamic_pointer_cast<FunctionOprand>(oprand);
            for (auto& arg: functionOprand->args){
                this->updateLiveVars(inst, arg, vars);
            }
        }
    }
};

std::string compileToAsm(AstNode& node, bool verbose = true);

#endif