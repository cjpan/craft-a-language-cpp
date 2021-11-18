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

enum OpCode{
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
    ireturn  = 0xac,
    vreturn   = 0xb1,
    invokestatic= 0xb8, //调用函数

    //自行扩展的操作码
    sadd     = 0x61,    //字符串连接
    sldc     = 0x13,    //把字符串常量入栈。字符串放在常量区，用两个操作数记录下标。
};

std::string toString(OpCode op);

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

    std::vector<uint8_t>  anyToCode(const std::any& val) {
        if (val.has_value() && isType<std::vector<uint8_t>>(val)) {
            return std::any_cast<std::vector<uint8_t>>(val);
        }
        return {};
    }

   void concatCodeWithAny(std::vector<uint8_t>& code, const std::any& val) {
        if (val.has_value() && isType<std::vector<uint8_t>>(val)) {
            auto vec = std::any_cast<std::vector<uint8_t>>(val);
            code.insert(code.end(), vec.begin(), vec.end());
        }
        return;
    }

    std::any visitProg(Prog& prog, std::string prefix) override {
        this->functionSym = prog.sym;
        if ( this->functionSym != nullptr){
            this->m->consts.push_back(this->functionSym);
            this->m->_main = this->functionSym;
            auto byteCode = this->anyToCode(this->visitBlock(prog, prefix));
            this->functionSym->byteCode = byteCode;
        }

        return this->m;
    }

    std::any visitBlock(Block& block, std::string prefix) override {
        // console.log("visitBlock in BCGenerator" );
        std::vector<uint8_t> ret;
        for(auto& x: block.stmts){
            this->inExpression = false; //每个语句开始的时候，重置
            auto code = this->visit(*x);
            if (code.has_value()){  //在visitFunctionDecl的时候，会返回std::any为空，则跳过
                auto vec = this->anyToCode(code);
                this->addOffsetToJumpOp(vec, vec.size());
                ret.insert(ret.end(), vec.begin(), vec.end());
                //dbg("=====================ret size:" + std::to_string(ret.size()));
            }
        }
        return ret;
    }

    std::any visitFunctionDecl(FunctionDecl& functionDecl, std::string prefix) override {
        //1.设置当前的函数符号
        auto lastFunctionSym = this->functionSym;
        this->functionSym = functionDecl.sym;

        //添加到Module
        this->m->consts.push_back(this->functionSym);

        //2.为函数体生成代码
        auto code1 = this->visit(*functionDecl.callSignature);
        auto code2 = this->visit(*functionDecl.body);

        auto vec1 = this->anyToCode(code1);
        auto vec2 = this->anyToCode(code2);
        this->addOffsetToJumpOp(vec2, vec1.size());

        if(this->functionSym != nullptr){
            this->concatCodeWithAny(vec1, vec2);
            this->functionSym->byteCode = vec1;
        }

        //3.恢复当前函数
        this->functionSym = lastFunctionSym;

        return std::any();
    }

    std::any visitVariableDecl(VariableDecl& variableDecl, std::string prefix) override {
        std::vector<uint8_t> code;
        if (variableDecl.init != nullptr){
            //获取初始化部分的Code
            auto ret = this->visit(*variableDecl.init);
            this->concatCodeWithAny(code, ret);
            //生成变量赋值的指令
            auto code1 = this->setVariableValue(variableDecl.sym);
            this->concatCodeWithAny(code, code1);
        }
        return code;
    }

    std::any visitReturnStatement(ReturnStatement& returnStatement, std::string prefix) override {
        // console.log("visitReturnStatement in BCGenerator" );
        std::vector<uint8_t> code;
        //1.为return后面的表达式生成代码
        if(returnStatement.exp != nullptr){
            auto code1 = this->visit(*returnStatement.exp);
            // console.log(code1);
            this->concatCodeWithAny(code, code1);
            code.push_back(OpCode::ireturn);

            return code;
        }
        else{
            //2.生成return代码，返回值是void
            code.push_back(OpCode::vreturn);
            return code;
        }
    }

    std::any visitFunctionCall(FunctionCall& functionCall, std::string prefix) override {
        // console.log("in AstVisitor.visitFunctionCall "+ functionCall.name);
        std::vector<uint8_t> code;
        //1.依次生成与参数计算有关的指令，也就是把参数压到计算栈里
        for(auto& param: functionCall.arguments){
            auto code1 = this->visit(*param);
            this->concatCodeWithAny(code, code1);
        }

        //2.生成invoke指令
        // console.log(functionCall.sym);

        //本地变量的下标
        auto compare = [&functionCall](const std::any& e) {
            return isType<std::shared_ptr<FunctionSymbol>>(e) &&
                std::any_cast<std::shared_ptr<FunctionSymbol>>(e)->name == functionCall.sym->name;
        };

        auto iter = find_if(this->m->consts.begin(), this->m->consts.end(), compare);
        if (iter ==  this->m->consts.end()) {
            dbg("Error: Can find functionCall.sym, set val failed!");
            return std::any();
        }

        uint16_t index = static_cast<uint16_t>(iter - this->m->consts.begin());

        // console.log(this->module);
        code.push_back(OpCode::invokestatic);
        code.push_back(index>>8);
        code.push_back(index);

        return code;
    }

    std::any visitVariable(Variable& v, std::string prefix) override {
        if (v.isLeftValue){
            return v.sym;
        }
        else{
            auto sym = std::dynamic_pointer_cast<VarSymbol>(v.sym);
            if (sym == nullptr) {
                dbg("Error: visitVariable get sym is nullptr");
            }
            return this->getVariableValue(sym);
        }
    }

    std::any visitIntegerLiteral(IntegerLiteral& integerLiteral, std::string prefix) override {
        // console.log("visitIntegerLiteral in BC");
        std::vector<uint8_t> code;
        int32_t value = integerLiteral.value;
        //0-5之间的数字，直接用快捷指令
        if (value >= 0 && value <= 5) {
            switch (value) {
                case 0:
                    code.push_back(OpCode::iconst_0);
                    break;
                case 1:
                    code.push_back(OpCode::iconst_1);
                    break;
                case 2:
                    code.push_back(OpCode::iconst_2);
                    break;
                case 3:
                    code.push_back(OpCode::iconst_3);
                    break;
                case 4:
                    code.push_back(OpCode::iconst_4);
                    break;
                case 5:
                    code.push_back(OpCode::iconst_5);
                    break;
            }
        }

        //如果是8位整数，用bipush指令，直接放在后面的一个字节的操作数里就行了
        else if (value >= -128 && value <128){
            code.push_back(OpCode::bipush);
            code.push_back(value);
        }

        //如果是16位整数，用sipush指令
        else if (value >= -32768 && value <32768){
            code.push_back(OpCode::sipush);
            //要拆成两个字节
            code.push_back(value >> 8);
            code.push_back(value & 0x00ff);
        }

        //大于16位的，采用ldc指令，从常量池中去取
        else{
            code.push_back(OpCode::ldc);
            //把value值放入常量池。
            this->m->consts.push_back(value);
            code.push_back(this->m->consts.size() - 1);
        }
        // console.log(ret);
        return code;
    }

    std::any visitBinary(Binary& bi, std::string prefix) override {
        std::vector<uint8_t> code;

        this->inExpression = true;
        auto ret1 = this->visit(*bi.exp1);
        auto ret2 = this->visit(*bi.exp2);

        auto code1 = this->anyToCode(ret1);
        auto code2 = this->anyToCode(ret2);

        uint16_t address1 = 0;
        uint16_t address2 = 0;
        uint8_t tempCode = 0;

        ////1.处理赋值
        if (bi.op == Op::Assign){
            // 左值的情况，返回符号
            dbg("Error: current not support Op::Assign!");
            return code;
        }
        ////2.处理其他二元运算
        else{
            //加入左子树的代码
            code = code1;
            //加入右子树的代码
            this->concatCodeWithAny(code, code2);
            //加入运算符的代码
            switch(bi.op){
                case Op::Plus: //'+'
                    if (bi.theType != nullptr && *bi.theType == *SysTypes::String()){
                        code.push_back(OpCode::sadd);
                    }
                    else{
                        code.push_back(OpCode::iadd);
                    }
                    break;
                case Op::Minus: //'-'
                    code.push_back(OpCode::isub);
                    break;
                case Op::Multiply: //'*'
                    code.push_back(OpCode::imul);
                    break;
                case Op::Divide: //'/'
                    code.push_back(OpCode::idiv);
                    break;
                case Op::G:  //'>'
                case Op::GE: //'>='
                case Op::L:  //'<'
                case Op::LE: //'<='
                case Op::EQ: //'=='
                case Op::NE: //'!='
                    if (bi.op ==Op::G){
                        tempCode = OpCode::if_icmple;
                    }
                    else if (bi.op ==Op::GE){
                        tempCode = OpCode::if_icmplt;
                    }
                    else if (bi.op ==Op::L){
                        tempCode = OpCode::if_icmpge;
                    }
                    else if (bi.op ==Op::LE){
                        tempCode = OpCode::if_icmpgt;
                    }
                    else if (bi.op ==Op::EQ){
                        tempCode = OpCode::if_icmpne;
                    }
                    else if (bi.op ==Op::NE){
                        tempCode = OpCode::if_icmpeq;
                    }

                    address1 = code.size() + 7;
                    address2 = address1 + 1;
                    code.push_back(tempCode);
                    code.push_back(address1>>8);
                    code.push_back(address1);
                    code.push_back(OpCode::iconst_1);
                    code.push_back(OpCode::igoto);
                    code.push_back(address2>>8);
                    code.push_back(address2);
                    code.push_back(OpCode::iconst_0);
                    break;
                default:
                    dbg("Unsupported binary operation: " + toString(bi.op));
                    return {};
            }
        }

        return code;
    }

    std::vector<uint8_t> getVariableValue(std::shared_ptr<VarSymbol>& sym) {
        std::vector<uint8_t> code;
        if (sym != nullptr){
            //本地变量的下标
            auto compare = [&sym](const std::shared_ptr<Symbol> & var) {
                return sym->name == var->name;
            };
            auto iter = std::find_if(this->functionSym->vars.begin(), this->functionSym->vars.end(), compare);
            if (iter == this->functionSym->vars.end()) {
                dbg("Error: Can find variable, get val failed!");
            }

            auto index = iter - this->functionSym->vars.begin();
            //根据不同的下标生成指令，尽量生成压缩指令
            switch (index){
                case 0:
                    code.push_back(OpCode::iload_0);
                    break;
                case 1:
                    code.push_back(OpCode::iload_1);
                    break;
                case 2:
                    code.push_back(OpCode::iload_2);
                    break;
                case 3:
                    code.push_back(OpCode::iload_3);
                    break;
                default:
                    code.push_back(OpCode::iload);
                    code.push_back(static_cast<uint8_t>(index));
                    break;
            }
        }
        return code;
    }

    std::vector<uint8_t> setVariableValue(std::shared_ptr<VarSymbol>& sym) {
        std::vector<uint8_t> code;
        if (sym != nullptr){
            //本地变量的下标
            auto compare = [&sym](const std::shared_ptr<Symbol> & var) {
                return sym->name == var->name;
            };
            auto iter = std::find_if(this->functionSym->vars.begin(), this->functionSym->vars.end(), compare);
            if (iter == this->functionSym->vars.end()) {
                dbg("Error: Can find variable, set val failed!");
            }

            auto index = iter - this->functionSym->vars.begin();
            //根据不同的下标生成指令，尽量生成压缩指令
            switch (index){
                case 0:
                    code.push_back(OpCode::istore_0);
                    break;
                case 1:
                    code.push_back(OpCode::istore_1);
                    break;
                case 2:
                    code.push_back(OpCode::istore_2);
                    break;
                case 3:
                    code.push_back(OpCode::istore_3);
                    break;
                default:
                    code.push_back(OpCode::istore);
                    code.push_back(static_cast<uint8_t>(index));
                    break;
            }
        }
        return code;
    }

    void addOffsetToJumpOp(std::vector<uint8_t>& code, uint32_t offset) {
        if (offset == 0) return;  //短路

        uint32_t codeIndex = 0;
        while(codeIndex < code.size()){
            switch(code[codeIndex]){
                //纯指令，后面不带操作数
                case OpCode::iadd:
                case OpCode::sadd:
                case OpCode::isub:
                case OpCode::imul:
                case OpCode::idiv:
                case OpCode::iconst_0:
                case OpCode::iconst_1:
                case OpCode::iconst_2:
                case OpCode::iconst_3:
                case OpCode::iconst_4:
                case OpCode::iconst_5:
                case OpCode::istore_0:
                case OpCode::istore_1:
                case OpCode::istore_2:
                case OpCode::istore_3:
                case OpCode::iload_0:
                case OpCode::iload_1:
                case OpCode::iload_2:
                case OpCode::iload_3:
                case OpCode::ireturn:
                case OpCode::vreturn:
                case OpCode::lcmp:
                    codeIndex++;
                    break;

                //指令后面带1个字节的操作数
                case OpCode::iload:
                case OpCode::istore:
                case OpCode::bipush:
                case OpCode::ldc:
                case OpCode::sldc:
                    codeIndex +=2;
                    break;

                //指令后面带2个字节的操作数
                case OpCode::iinc:
                case OpCode::invokestatic:
                case OpCode::sipush:
                    codeIndex +=3;
                    break;

                //跳转语句，需要给跳转指令加上offset
                case OpCode::if_icmpeq:
                case OpCode::if_icmpne:
                case OpCode::if_icmpge:
                case OpCode::if_icmpgt:
                case OpCode::if_icmple:
                case OpCode::if_icmplt:
                case OpCode::ifeq:
                case OpCode::ifne:
                case OpCode::ifge:
                case OpCode::ifgt:
                case OpCode::ifle:
                case OpCode::iflt:
                case OpCode::igoto:
                {
                    int8_t byte1 = code[codeIndex+1];
                    int8_t byte2 = code[codeIndex+2];
                    int16_t address = (byte1<<8|byte2) + offset;
                    code[codeIndex+1] = static_cast<int8_t>(address>>8);
                    code[codeIndex+2] = static_cast<int8_t>(address);
                    codeIndex += 3;
                    break;
                }


                default:
                    dbg("unrecognized Op Code in addOffsetToJumpOp: "+ std::to_string(code[codeIndex]));
                    return;
            }
        }

        return;
    }

};

struct VMStackFrame{
    //对应的函数，用来找到代码
    std::shared_ptr<FunctionSymbol> funtionSym;

    //返回地址
    uint32_t returnIndex = 0;

    //本地变量
    std::vector<int32_t> localVars;

    //操作数栈
    std::vector<std::any> oprandStack;

    VMStackFrame(std::shared_ptr<FunctionSymbol>& funtionSym): funtionSym(funtionSym){
        this->localVars.resize(funtionSym->vars.size());
    }
};

class VM{
public:
    std::vector<std::shared_ptr<VMStackFrame>> callStack;

    VM(){
        std::call_once(flag, [](){
            dbg("VM::InitBinaryFunction: called once");
            InitBinaryFunction();
        });
    }

    using BinaryFunction = std::function<std::any(const std::any&, const std::any&)>;
    static std::map<OpCode,
                    std::map<std::type_index,
                        std::map<std::type_index,
                            BinaryFunction>>> binaryOp;
    static void InitBinaryFunction();
    static std::optional<BinaryFunction> GetBinaryFunction(OpCode op, const std::any& l, const std::any& r);
    static std::once_flag flag;

    /**
     * 运行一个模块。
     * @param bcModule
     */
    int32_t execute(const BCModule& bcModule){

        //找到入口函数
        std::shared_ptr<FunctionSymbol> functionSym;
        if (bcModule._main == nullptr){
            dbg("Can not find main function.");
            return -1;
        }
        else{
            functionSym = bcModule._main;
        }

        //创建栈桢
        auto frame = std::make_shared<VMStackFrame>(functionSym);
        this->callStack.push_back(frame);

        //当前运行的代码
        std::vector<uint8_t> code;
        if (!functionSym->byteCode.empty()){
            code = functionSym->byteCode;
        }
        else{
            dbg("Can not find code for "+ frame->funtionSym->name);
            return -1;
        }

        //当前代码的位置
        uint32_t codeIndex = 0;

        //一直执行代码，直到遇到return语句
        uint8_t opCode = code[codeIndex];

        //临时变量
        int8_t byte1 = 0;
        int8_t byte2 = 0;
        std::any vleft;
        std::any vright;

        int32_t tmpLocal = 0;

        std::any anyTmp;
        uint32_t constIndex = 0;
        int32_t numValue = 0;
        std::string strValue;

        while(true){
            switch (opCode){
                case OpCode::iconst_0:
                    frame->oprandStack.push_back(static_cast<int8_t>(0));
                    opCode = code[++codeIndex];
                    break;
                case OpCode::iconst_1:
                    frame->oprandStack.push_back(static_cast<int8_t>(1));
                    opCode = code[++codeIndex];
                    break;
                case OpCode::iconst_2:
                    frame->oprandStack.push_back(static_cast<int8_t>(2));
                    opCode = code[++codeIndex];
                    break;
                case OpCode::iconst_3:
                    frame->oprandStack.push_back(static_cast<int8_t>(3));
                    opCode = code[++codeIndex];
                    break;
                case OpCode::iconst_4:
                    frame->oprandStack.push_back(static_cast<int8_t>(4));
                    opCode = code[++codeIndex];
                    break;
                case OpCode::iconst_5:
                    frame->oprandStack.push_back(static_cast<int8_t>(5));
                    opCode = code[++codeIndex];
                    break;
                case OpCode::bipush:  //取出1个字节
                    frame->oprandStack.push_back(static_cast<int8_t>(code[++codeIndex]));
                    opCode = code[++codeIndex];
                    break;
                case OpCode::sipush:  //取出2个字节
                    byte1 = code[++codeIndex];
                    byte2 = code[++codeIndex];
                    frame->oprandStack.push_back((byte1<<8)|byte2);
                    opCode = code[++codeIndex];
                    break;

                case OpCode::ldc:   //从常量池加载
                    constIndex = code[++codeIndex];
                    anyTmp = bcModule.consts[constIndex];

                    if (!isType<int32_t>(anyTmp)) {
                        dbg("Error: ldc value type not int32 at: " + std::to_string(codeIndex - 1));
                        return -2;
                    }
                    numValue = std::any_cast<int32_t>(anyTmp);
                    frame->oprandStack.push_back(numValue);
                    opCode = code[++codeIndex];
                    break;


                case OpCode::sldc:   //从常量池加载字符串
                    constIndex = code[++codeIndex];
                    anyTmp = bcModule.consts[constIndex];
                    if (!isType<std::string>(anyTmp)) {
                        dbg("Error: sldc value type not string at: " + std::to_string(codeIndex - 1));
                        return -2;
                    }
                    strValue = std::any_cast<std::string>(anyTmp);
                    frame->oprandStack.push_back(strValue);
                    opCode = code[++codeIndex];
                    break;
                case OpCode::iload:
                    frame->oprandStack.push_back(frame->localVars[code[++codeIndex]]);
                    opCode = code[++codeIndex];
                    break;
                case OpCode::iload_0:
                    frame->oprandStack.push_back(frame->localVars[0]);
                    opCode = code[++codeIndex];
                    break;
                case OpCode::iload_1:
                    frame->oprandStack.push_back(frame->localVars[1]);
                    opCode = code[++codeIndex];
                    break;
                case OpCode::iload_2:
                    frame->oprandStack.push_back(frame->localVars[2]);
                    opCode = code[++codeIndex];
                    break;
                case OpCode::iload_3:
                    frame->oprandStack.push_back(frame->localVars[3]);
                    opCode = code[++codeIndex];
                    break;

                case OpCode::istore:
                    anyTmp = frame->oprandStack.back();
                    frame->oprandStack.pop_back();
                    if (!isType<int32_t>(anyTmp)) {
                        dbg("Error: istore value type not int32_t at: " + std::to_string(codeIndex - 1));
                        return -2;
                    }
                    tmpLocal = std::any_cast<int32_t>(anyTmp);
                    frame->localVars[code[++codeIndex]] = tmpLocal;
                    opCode = code[++codeIndex];
                    break;

                case OpCode::istore_0:
                {
                    anyTmp = frame->oprandStack.back();
                    frame->oprandStack.pop_back();
                    if (isType<int32_t>(anyTmp)) {
                        tmpLocal = std::any_cast<int32_t>(anyTmp);
                    } else if (isType<int8_t>(anyTmp)) {
                        tmpLocal = std::any_cast<int8_t>(anyTmp);
                    } else {
                        dbg("Error: istore value type not int32_t/int8_t at: " + std::string(anyTmp.type().name()));
                        return -2;
                    }

                    frame->localVars[0] = tmpLocal;
                    opCode = code[++codeIndex];
                    break;
                }
                case OpCode::istore_1:
                    anyTmp = frame->oprandStack.back();
                    frame->oprandStack.pop_back();
                    if (!isType<int32_t>(anyTmp)) {
                        dbg("Error: istore value type not int32_t at: " + std::to_string(codeIndex - 1));
                        return -2;
                    }
                    tmpLocal = std::any_cast<int32_t>(anyTmp);

                    frame->localVars[1] = tmpLocal;
                    opCode = code[++codeIndex];
                    break;
                case OpCode::istore_2:
                    anyTmp = frame->oprandStack.back();
                    frame->oprandStack.pop_back();
                    if (!isType<int32_t>(anyTmp)) {
                        dbg("Error: istore value type not int32_t at: " + std::to_string(codeIndex - 1));
                        return -2;
                    }
                    tmpLocal = std::any_cast<int32_t>(anyTmp);

                    frame->localVars[2] = tmpLocal;
                    opCode = code[++codeIndex];
                    break;
                case OpCode::istore_3:
                    anyTmp = frame->oprandStack.back();
                    frame->oprandStack.pop_back();
                    if (!isType<int32_t>(anyTmp)) {
                        dbg("Error: istore value type not int32_t at: " + std::to_string(codeIndex - 1));
                        return -2;
                    }
                    tmpLocal = std::any_cast<int32_t>(anyTmp);

                    frame->localVars[3] = tmpLocal;
                    opCode = code[++codeIndex];
                    break;

                case OpCode::iadd:
                case OpCode::isub:
                case OpCode::imul:
                case OpCode::idiv:
                {
                    vright = frame->oprandStack.back();
                    frame->oprandStack.pop_back();
                    vleft = frame->oprandStack.back();
                    frame->oprandStack.pop_back();

                    auto func = VM::GetBinaryFunction(static_cast<OpCode>(opCode), vleft, vright);
                    if (!func) {
                        dbg("Unsupported binary operation: " + toString(static_cast<OpCode>(opCode)));
                        return -2;
                    }

                    auto ret = func.value()(vleft, vright);
                    frame->oprandStack.push_back(ret);
                    opCode = code[++codeIndex];
                    break;
                }

/*
                case OpCode::sadd:
                case OpCode::iinc:
                    auto varIndex = code[++codeIndex];
                    auto offset = code[++codeIndex];
                    frame->localVars[varIndex] = frame->localVars[varIndex]+offset;
                    opCode = code[++codeIndex];
                    break;
*/

                case OpCode::ireturn:
                case OpCode::vreturn:
                {
                    //确定返回值
                    std::any retValue;
                    if(opCode == OpCode::ireturn){
                        retValue = frame->oprandStack.back();
                        frame->oprandStack.pop_back();
                    }

                    //弹出栈桢，返回到上一级函数，继续执行
                    this->callStack.pop_back();
                    if (this->callStack.empty()){ //主程序返回，结束运行
                        return 0;
                    }
                    else { //返回到上一级调用者
                        frame = this->callStack.back();
                        //设置返回值到上一级栈桢
                        // frame.retValue = retValue;
                        if(opCode == OpCode::ireturn){
                            frame->oprandStack.push_back(retValue);
                        }
                        //设置新的code、codeIndex和oPCode
                        if (!frame->funtionSym->byteCode.empty()){
                            //切换到调用者的代码
                            code = frame->funtionSym->byteCode;
                            //设置指令指针为返回地址，也就是调用该函数的下一条指令
                            codeIndex = frame->returnIndex;
                            opCode = code[codeIndex];
                            break;
                        }
                        else{
                            dbg("Can not find code for " + frame->funtionSym->name);
                            return -1;
                        }
                    }
                    break;
                }

                case OpCode::invokestatic:
                {
                    //从常量池找到被调用的函数
                    byte1 = code[++codeIndex];
                    byte2 = code[++codeIndex];
                    auto sym = bcModule.consts[byte1<<8|byte2];
                    if (!isType<std::shared_ptr<FunctionSymbol>>(sym)) {
                        dbg("Error: invokestatic expect is std::shared_ptr<FunctionSymbol>, index: " + std::to_string(byte1<<8|byte2));
                        return -2;
                    }

                    functionSym = std::any_cast<std::shared_ptr<FunctionSymbol>>(sym);

                    //对于内置函数特殊处理
                    if(functionSym->name == "println"){
                        dbg("VM call println");
                        //取出一个参数
                        auto param = frame->oprandStack.back();
                        frame->oprandStack.pop_back();
                        opCode = code[++codeIndex];
                        PrintAny(param);   //打印显示
                    }
                    else if(functionSym->name == "tick"){
                        opCode = code[++codeIndex];
                        // TODO add some time
                        int32_t value = 1024;
                        frame->oprandStack.push_back(value);
                    }
                    else if(functionSym->name == "integer_to_string"){
                        opCode = code[++codeIndex];
                        anyTmp =frame->oprandStack.back();
                        frame->oprandStack.pop_back();

                        if (!isType<int32_t>(anyTmp)) {
                            dbg("Error: invokestatic integer_to_string expect int32_t, but: " + std::string(anyTmp.type().name()));
                        }

                        numValue = std::any_cast<int32_t>(anyTmp);

                        frame->oprandStack.push_back(std::to_string(numValue));
                    }
                    else{
                        //设置返回值地址，为函数调用的下一条指令
                        frame->returnIndex = codeIndex + 1;

                        //创建新的栈桢
                        auto lastFrame = frame;
                        frame = std::make_shared<VMStackFrame>(functionSym);
                        this->callStack.push_back(frame);

                        //传递参数
                        auto paramCount = functionSym->getNumParams();
                        for(int32_t i = paramCount -1; i>= 0; i--){
                            auto tmp = lastFrame->oprandStack.back();
                            lastFrame->oprandStack.pop_back();

                            if (!isType<int8_t>(tmp)) {
                                dbg("Error: invokestatic param expect int8_t, but: " + std::string(tmp.type().name()));
                            }

                            frame->localVars[i] = std::any_cast<int8_t>(tmp);
                        }

                        //设置新的code、codeIndex和oPCode
                        if (!frame->funtionSym->byteCode.empty()){
                            //切换到被调用函数的代码
                            code = frame->funtionSym->byteCode;
                            //代码指针归零
                            codeIndex = 0;
                            opCode = code[codeIndex];
                            break;
                        }
                        else{
                            dbg("Can not find code for "+ frame->funtionSym->name);
                            return -1;
                        }
                    }
                    break;
                }

/*
                case OpCode::ifeq:
                    byte1 = code[++codeIndex];
                    byte2 = code[++codeIndex];
                    if(frame->oprandStack.pop() == 0){
                        codeIndex = byte1<<8|byte2;
                        opCode = code[codeIndex];
                    }
                    else{
                        opCode = code[++codeIndex];
                    }
                    break;
                case OpCode::ifne:
                    byte1 = code[++codeIndex];
                    byte2 = code[++codeIndex];
                    if(frame->oprandStack.pop() != 0){
                        codeIndex = byte1<<8|byte2;
                        opCode = code[codeIndex];
                    }
                    else{
                        opCode = code[++codeIndex];
                    }
                    break;
                case OpCode::if_icmplt:
                    byte1 = code[++codeIndex];
                    byte2 = code[++codeIndex];
                    vright = frame->oprandStack.pop();
                    vleft = frame->oprandStack.pop();
                    if(vleft < vright){
                        codeIndex = byte1<<8|byte2;
                        opCode = code[codeIndex];
                    }
                    else{
                        opCode = code[++codeIndex];
                    }
                    break;
                case OpCode::if_icmpge:
                    byte1 = code[++codeIndex];
                    byte2 = code[++codeIndex];
                    vright = frame->oprandStack.pop();
                    vleft = frame->oprandStack.pop();
                    if(vleft >= vright){
                        codeIndex = byte1<<8|byte2;
                        opCode = code[codeIndex];
                    }
                    else{
                        opCode = code[++codeIndex];
                    }
                    break;
                case OpCode::if_icmpgt:
                    byte1 = code[++codeIndex];
                    byte2 = code[++codeIndex];
                    vright = frame->oprandStack.pop();
                    vleft = frame->oprandStack.pop();
                    if(vleft > vright){
                        codeIndex = byte1<<8|byte2;
                        opCode = code[codeIndex];
                    }
                    else{
                        opCode = code[++codeIndex];
                    }
                    break;
                case OpCode::if_icmple:
                    byte1 = code[++codeIndex];
                    byte2 = code[++codeIndex];
                    vright = frame->oprandStack.pop();
                    vleft = frame->oprandStack.pop();
                    if(vleft <= vright){
                        codeIndex = byte1<<8|byte2;
                        opCode = code[codeIndex];
                    }
                    else{
                        opCode = code[++codeIndex];
                    }
                    break;
                case OpCode::goto:
                    byte1 = code[++codeIndex];
                    byte2 = code[++codeIndex];
                    codeIndex = byte1<<8|byte2;
                    opCode = code[codeIndex];
                    break;
*/
                default:
                    dbg("Unknown or Unsupported op code: "+ toString(static_cast<OpCode>(opCode)));
                    return -2;
            }
        }

        return 0;
    }

};

class BCModuleWriter {
public:
    std::vector<std::shared_ptr<Type>> types; //保存该模块所涉及的类型


    bool CanAddTypes(const std::shared_ptr<Type>& t) {

        if (SysTypes::isSysType(t)){
            return false;
        }

        auto it = find(this->types.begin(), this->types.end(), t);
        if (it != this->types.end()){
            return false;
        }

        return true;
    }
    /**
     * 从bcModule生成字节码
     * @param bcModule
     */
    std::vector<uint8_t> write(const BCModule& bcModule) {
        std::vector<uint8_t> bc2;
        this->types.clear();  //重置状态变量

        // dbg(std::string("bcModule.consts size: ") + std::to_string(bcModule.consts.size()));
        //写入常量
        uint8_t numConsts = 0;
        for(auto& c: bcModule.consts){
            if (isType<int32_t>(c)){
                bc2.push_back(1); //代表接下来是一个number；
                bc2.push_back(std::any_cast<int32_t>(c));
                numConsts++;
            }
            else if (isType<std::string>(c)){
                bc2.push_back(2); //代表接下来是一个string；
                this->writeString(bc2, std::any_cast<std::string>(c));
                numConsts++;
            }
            else if (isType<std::shared_ptr<FunctionSymbol>>(c)){
                auto functionSym = std::any_cast<std::shared_ptr<FunctionSymbol>>(c);
                if (built_ins.find(functionSym->name) == built_ins.end()){ //不用写入系统函数
                    bc2.push_back(3); //代表接下来是一个FunctionSymbol.
                    auto tmp = this->writeFunctionSymbol(functionSym);
                    bc2.insert(bc2.end(), tmp.begin(), tmp.end());
                    numConsts++;
                }
            }
            else{
                dbg("Unsupported const in BCModuleWriter, type: " + std::string(c.type().name()));
            }
        }

        //写入类型
        std::vector<uint8_t> bc1;
        this->writeString(bc1,"types");
        bc1.push_back(this->types.size());
        for (auto t: this->types){
            std::vector<uint8_t> tmp;
            if (t->isFunctionType()){
                tmp = this->writeFunctionType(t);
            }
            else if (t->isSimpleType()){
                tmp = this->writeSimpleType(t);
            }
            else{
                dbg("Unsupported type in BCModuleWriter: " + t->name);
            }
            bc1.insert(bc1.end(), tmp.begin(), tmp.end());
        }

        this->writeString(bc1, "consts");
        bc1.push_back(numConsts);

        bc1.insert(bc1.end(), bc2.begin(), bc2.end());

        return bc1;
    }

    std::vector<uint8_t> writeVarSymbol(std::shared_ptr<Symbol>& s) {
        std::vector<uint8_t> bc;

        auto sym = std::dynamic_pointer_cast<VarSymbol>(s);

        //写入变量名称
        this->writeString(bc, sym->name);

        //写入类型名称
        this->writeString(bc, sym->theType->name);
        if (this->CanAddTypes(sym->theType)) {
            this->types.push_back(sym->theType);
        }

        dbg(sym->theType->name);
        PrintHex(bc);
        return bc;
    }

    std::vector<uint8_t> writeFunctionSymbol(std::shared_ptr<FunctionSymbol>& sym) {
        std::vector<uint8_t> bc;

        //写入函数名称
        this->writeString(bc, sym->name);

        //写入类型名称
        this->writeString(bc, sym->theType->name);
        if (this->CanAddTypes(sym->theType)) {
            this->types.push_back(sym->theType);
        }

        //写入操作数栈最大的大小
        bc.push_back(sym->opStackSize);

        //写入本地变量个数
        bc.push_back(sym->vars.size());

        //逐一写入变量
        //TODO：其实具体变量的信息不是必需的。
        for (auto v: sym->vars){
            auto tmp = this->writeVarSymbol(v);
            bc.insert(bc.end(), tmp.begin(), tmp.end());
        }

        //写入函数函数体的字节码
        if(sym->byteCode.empty()){ //内置函数
            bc.push_back(0);
        }
        else{  //自定义函数
            bc.push_back(sym->byteCode.size());
            PrintHex(sym->byteCode);
            bc.insert(bc.end(), sym->byteCode.begin(), sym->byteCode.end());
        }
        return bc;
    }

    std::vector<uint8_t> writeSimpleType(std::shared_ptr<Type>& t) {
        std::vector<uint8_t> bc;
        if (SysTypes::isSysType(t)) {
            return bc;
        }

        // TODO in future
        dbg("Error: current not support yet!");

        return bc;
    }

    std::vector<uint8_t> writeFunctionType(std::shared_ptr<Type>& type) {
        std::vector<uint8_t> bc;

        auto t = std::dynamic_pointer_cast<FunctionType>(type);
        bc.push_back(static_cast<uint8_t>(2)); //代表FunctionType

        //写入类型名称
        this->writeString(bc, t->name);

        //写入返回值名称
        this->writeString(bc, t->returnType->name);

        //写入参数数量
        bc.push_back(static_cast<uint8_t>(t->paramTypes.size()));

        //写入参数的类型名称
        for (auto pt: t->paramTypes){
            this->writeString(bc, pt->name);
            auto it = find(this->types.begin(), this->types.end(), pt);
            if (it == this->types.end()){
                this->types.push_back(pt);
            }
        }

        return bc;
    }



    /**
     * 把字符串添加的字节码数组中
     * @param bc
     * @param str
     */
    void writeString(std::vector<uint8_t>& bc, const std::string& str){
        //写入字符串的长度
        bc.push_back(static_cast<uint8_t>(str.size()));
        for (auto c : str){
            bc.push_back(static_cast<uint8_t>(c));
        }
    }
};


/**
 * 从字节码生成BCModule
 */
class BCModuleReader {
    //读取字节码时的下标
    uint32_t index = 0;

    //解析出来的所有类型
    std::map<std::string, std::shared_ptr<Type>> types;

    std::map<std::string, std::any> typeInfos;
public:

    std::shared_ptr<BCModule> read(const std::vector<uint8_t>& bc) {
        //重置状态变量
        this->index = 0;
        this->types.clear();

        auto bcModule = std::make_shared<BCModule>();

        //1.读取类型
        //1.1加入系统内置类型
        this->addSystemTypes();

        //1.2从字节码中读取类型
        auto str = this->readString(bc);
        if(str != "types") {
            dbg("Error: must read 'types', but: " + str);
            return nullptr;
        }

        auto numTypes = bc[this->index++];
        for (uint8_t i = 0; i < numTypes; i++){
            auto typeKind = bc[this->index++];
            switch(typeKind){
                case 1:
                    this->readSimpleType(bc);
                    break;
                case 2:
                    this->readFunctionType(bc);
                    break;
                case 3:
                    this->readUnionType(bc);
                    break;
                default:
                    dbg("Unsupported type kind: " + std::to_string(typeKind));
            }
        }
        this->buildTypes();

        //2.读取常量
        str = this->readString(bc);
        if(str != "consts") {
            dbg("Error: must read 'consts', but: " + str);
            return nullptr;
        }

        auto numConsts = bc[this->index++];
        for (uint8_t i = 0; i< numConsts; i++){
            auto constType = bc[this->index++];
            if (constType == 1){
                bcModule->consts.push_back(bc[this->index++]);
            }
            else if (constType == 2){
                auto str = this->readString(bc);
                bcModule->consts.push_back(str);
            }
            else if (constType == 3){
                auto functionSym = this->readFunctionSymbol(bc);
                bcModule->consts.push_back(functionSym);
                if (functionSym->name == "main"){
                    bcModule->_main = functionSym;
                }
            }
            else{
                dbg("Unsupported const type: " + std::to_string(constType));
            }
        }

        return bcModule;
    }

    void addSystemTypes() {
        this->types.insert({"any", SysTypes::Any()});
        this->types.insert({"number", SysTypes::Number()});
        this->types.insert({"string", SysTypes::String()});
        this->types.insert({"integer", SysTypes::Integer()});
        this->types.insert({"decimal", SysTypes::Decimal()});
        this->types.insert({"boolean", SysTypes::Boolean()});
        this->types.insert({"null", SysTypes::Null()});
        this->types.insert({"undefined", SysTypes::Undefined()});
        this->types.insert({"void", SysTypes::Void()});
    }

    void buildTypes() {
        for(auto& item: this->typeInfos){
            auto typeName = item.first;
            auto t = this->types[typeName];

            if (t->isSimpleType()){
                dbg("Error: BCModuleReader SimpleType not support!");
                // todo
            }
            else if (t->isFunctionType()){
                auto funtionType = std::dynamic_pointer_cast<FunctionType>(t);

                std::pair< std::string, std::vector<std::string> > val = std::any_cast< std::pair< std::string, std::vector<std::string> > >(this->typeInfos[typeName]);
                auto returnType = val.first;
                auto paramTypes = val.second;

                funtionType->returnType = this->types[returnType];
                for (const auto& utName: paramTypes){
                    auto ut = this->types[utName];
                    funtionType->paramTypes.push_back(ut);
                }
            }
            // not support
            // else if (t->isUnionType()){
                // dbg("Error: BCModuleReader UnionType not support!");
            // }
            else{
                dbg("Unsupported type in BCModuleReader: " + t->name);
            }
        }

        this->typeInfos.clear();
    }

    std::string readString(const std::vector<uint8_t>& bc) {
        uint8_t len = bc[this->index++];
        std::string str = "";
        for (uint8_t i = 0; i< len; i++){
            str += static_cast<char>(bc[this->index++]);
        }
        return str;
    }

    void readSimpleType(const std::vector<uint8_t>& bc) {
        auto typeName = this->readString(bc);
        auto numUpperTypes = bc[this->index++];
        std::vector<std::string> upperTypes;
        for (uint8_t i = 0; i < numUpperTypes; i++){
            upperTypes.push_back(this->readString(bc));
        }

        std::shared_ptr<Type> t = std::make_shared<SimpleType>(typeName);
        this->types.insert({typeName, t});
        this->typeInfos.insert({typeName, upperTypes});
    }

    void readFunctionType(const std::vector<uint8_t>& bc) {
        auto typeName = this->readString(bc);
        auto returnType = this->readString(bc);
        auto numParams = bc[this->index++];
        std::vector<std::string> paramTypes;
        for (uint8_t i = 0; i< numParams; i++){
            paramTypes.push_back(this->readString(bc));
        }

        std::vector<std::shared_ptr<Type>> parms;
        std::shared_ptr<Type> t = std::make_shared<FunctionType>(SysTypes::Any(), parms, typeName);
        this->types.insert({typeName, t});
        this->typeInfos.insert({typeName, std::make_pair(returnType, paramTypes)});
    }

    void readUnionType(const std::vector<uint8_t>& bc) {
        dbg("Error: BCModuleReader::readUnionType not support!");
    }

    std::shared_ptr<FunctionSymbol> readFunctionSymbol(const std::vector<uint8_t>& bc) {
        //函数名称
        auto functionName = this->readString(bc);

        //读取类型名称
        auto typeName = this->readString(bc);
        auto functionType = this->types[typeName];

        //操作数栈的大小
        auto opStackSize = bc[this->index++];

        //变量个数
        auto numVars = bc[this->index++];

        //读取变量
        std::vector<std::shared_ptr<Symbol>> vars;
        for (uint8_t i = 0; i < numVars; i++){
            vars.push_back(this->readVarSymbol(bc));
        }

        //读取函数体的字节码
        auto numByteCodes = bc[this->index++];
        std::vector<uint8_t> byteCodes;
        if (numByteCodes != 0){  //系统函数0
            byteCodes.insert(byteCodes.end(), bc.begin() + this->index, bc.begin() + this->index + numByteCodes);
            this->index += numByteCodes;
        }

        //创建函数符号
        auto functionSym = std::make_shared<FunctionSymbol>(functionName, functionType);
        functionSym->vars = vars;
        functionSym->opStackSize = opStackSize;
        functionSym->byteCode = byteCodes;

        return functionSym;
    }

    std::shared_ptr<Symbol> readVarSymbol(const std::vector<uint8_t>& bc) {
        //变量名称
        auto varName = this->readString(bc);

        //类型名称
        auto typeName = this->readString(bc);
        auto varType = this->types[typeName];

        return std::make_shared<VarSymbol>(varName,varType);
    }

};

#endif