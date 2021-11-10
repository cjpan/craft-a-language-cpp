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
                    if (*bi.theType == *SysTypes::String()){
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
                    uint8_t byte1 = code[codeIndex+1];
                    uint8_t byte2 = code[codeIndex+2];
                    uint8_t address = (byte1<<8|byte2) + offset;
                    code[codeIndex+1] = address>>8;
                    code[codeIndex+2] = address;
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
    std::vector<uint8_t> localVars;

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