#include "asm_x86-64.h"

std::map<AsmOpCode, std::string> asmOpCodeToString {

    {AsmOpCode::jmp    ,"jmp"},
    {AsmOpCode::je     ,"je"},
    {AsmOpCode::jne    ,"jne"},
    {AsmOpCode::jle    ,"jle"},
    {AsmOpCode::jl     ,"jl"},
    {AsmOpCode::jge    ,"jge"},
    {AsmOpCode::jg     ,"jg"},


    {AsmOpCode::sete   ,"sete"},
    {AsmOpCode::setne  ,"setne"},
    {AsmOpCode::setl   ,"setl"},
    {AsmOpCode::setle  ,"setle"},
    {AsmOpCode::setg   ,"setg"},
    {AsmOpCode::setge  ,"setge"},


    {AsmOpCode::movq   ,"movq"},
    {AsmOpCode::addq   ,"addq"},
    {AsmOpCode::subq   ,"subq"},
    {AsmOpCode::mulq   ,"mulq"},
    {AsmOpCode::imulq  ,"imulq"},
    {AsmOpCode::divq   ,"divq"},
    {AsmOpCode::idivq  ,"idivq"},
    {AsmOpCode::negq   ,"negq"},
    {AsmOpCode::incq   ,"incq"},
    {AsmOpCode::decq   ,"decq"},
    {AsmOpCode::xorq   ,"xorq"},
    {AsmOpCode::orq    ,"orq"},
    {AsmOpCode::andq   ,"andq"},
    {AsmOpCode::notq   ,"notq"},
    {AsmOpCode::leaq   ,"leaq"},
    {AsmOpCode::callq  ,"callq"},
    {AsmOpCode::retq   ,"retq"},
    {AsmOpCode::pushq  ,"pushq"},
    {AsmOpCode::popq   ,"popq"},
    {AsmOpCode::cmpq   ,"cmpq"},


    {AsmOpCode::movl   ,"movl"},
    {AsmOpCode::addl   ,"addl"},
    {AsmOpCode::subl   ,"subl"},
    {AsmOpCode::mull   ,"mull"},
    {AsmOpCode::imull  ,"imull"},
    {AsmOpCode::divl   ,"divl"},
    {AsmOpCode::idivl  ,"idivl"},
    {AsmOpCode::negl   ,"negl"},
    {AsmOpCode::incl   ,"incl"},
    {AsmOpCode::decl   ,"decl"},
    {AsmOpCode::xorl   ,"xorl"},
    {AsmOpCode::orl    ,"orl"},
    {AsmOpCode::andl   ,"andl"},
    {AsmOpCode::notl   ,"notl"},
    {AsmOpCode::leal   ,"leal"},
    {AsmOpCode::calll  ,"calll"},
    {AsmOpCode::retl   ,"retl"},
    {AsmOpCode::pushl  ,"pushl"},
    {AsmOpCode::popl   ,"popl"},
    {AsmOpCode::cmpl   ,"cmpl"},


    {AsmOpCode::movw   ,"movw"},
    {AsmOpCode::addw   ,"addw"},
    {AsmOpCode::subw   ,"subw"},
    {AsmOpCode::mulw   ,"mulw"},
    {AsmOpCode::imulw  ,"imulw"},
    {AsmOpCode::divw   ,"divw"},
    {AsmOpCode::idivw  ,"idivw"},
    {AsmOpCode::negw   ,"negw"},
    {AsmOpCode::incw   ,"incw"},
    {AsmOpCode::decw   ,"decw"},
    {AsmOpCode::xorw   ,"xorw"},
    {AsmOpCode::orw    ,"orw"},
    {AsmOpCode::andw   ,"andw"},
    {AsmOpCode::notw   ,"notw"},
    {AsmOpCode::leaw   ,"leaw"},
    {AsmOpCode::callw  ,"callw"},
    {AsmOpCode::retw   ,"retw"},
    {AsmOpCode::pushw  ,"pushw"},
    {AsmOpCode::popw   ,"popw"},
    {AsmOpCode::cmpw   ,"cmpw"},


    {AsmOpCode::movb   ,"movb"},
    {AsmOpCode::addb   ,"addb"},
    {AsmOpCode::subb   ,"subb"},
    {AsmOpCode::mulb   ,"mulb"},
    {AsmOpCode::imulb  ,"imulb"},
    {AsmOpCode::divb   ,"divb"},
    {AsmOpCode::idivb  ,"idivb"},
    {AsmOpCode::negb   ,"negb"},
    {AsmOpCode::incb   ,"incb"},
    {AsmOpCode::decb   ,"decb"},
    {AsmOpCode::xorb   ,"xorb"},
    {AsmOpCode::orb    ,"orb"},
    {AsmOpCode::andb   ,"andb"},
    {AsmOpCode::notb   ,"notb"},
    {AsmOpCode::leab   ,"leab"},
    {AsmOpCode::callb  ,"callb"},
    {AsmOpCode::retb   ,"retb"},
    {AsmOpCode::pushb  ,"pushb"},
    {AsmOpCode::popb   ,"popb"},
    {AsmOpCode::cmpb   ,"cmpb"},

};

std::map<OprandKind, std::string> oprandKindToString {

    {OprandKind::varIndex   ,    "varIndex"},
    {OprandKind::returnSlot ,    "returnSlot"},
    {OprandKind::bb         ,    "bb"},
    {OprandKind::function   ,    "function"},
    {OprandKind::stringConst,    "stringConst"},

    {OprandKind::regist     ,    "regist"},
    {OprandKind::memory     ,    "memory"},
    {OprandKind::immediate  ,    "immediate"},

    {OprandKind::flag       ,    "flag"},

};


std::string toString(AsmOpCode op) {
    auto iter = asmOpCodeToString.find(op);
    if (iter == asmOpCodeToString.end()) {
        return std::string("Unknow AsmOpCode");
    }

    return iter->second;
}

std::string toString(OprandKind kind) {
    auto iter = oprandKindToString.find(kind);
    if (iter == oprandKindToString.end()) {
        return std::string("Unknow OprandKind");
    }

    return iter->second;
}

const uint32_t Register::numAvailableRegs = 13;

std::vector<std::shared_ptr<Oprand>> Register::registers32 {
    Register::edi(),

};

std::string compileToAsm(AstNode& node, bool verbose){

    auto asmGenerator = AsmGenerator();

    //生成LIR
    auto val = asmGenerator.visit(node);
    if (!val.has_value() || !isType<std::shared_ptr<AsmModule>>(val)) {
        dbg("Error: asmGenerator.visit error.");
        return "";
    }

    auto asmModule = std::any_cast<std::shared_ptr<AsmModule>>(val);
    if (verbose){
        dbg("before Lower:");
        Print(asmModule->toString());
    }

    /*
    //Lower
    let lower = new Lower(asmModule);
    lower.lowerModule();

    let asm = asmModule.toString();
    if (verbose){
        console.log("在Lower之后：");
        console.log(asm);
    }

    return asm;
    */

    return "";
}