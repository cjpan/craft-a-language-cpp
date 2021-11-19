#include "interpretor.h"


std::map<Op,
        std::map<std::type_index,
            std::map<std::type_index,
                Interpretor::BinaryFunction>>> Interpretor::binaryOp = {

    //{Op::Plus, { std::type_index(typeid(int32_t)), {std::type_index(typeid(int32_t)), PlusIntInt} } };

};

std::once_flag Interpretor::flag;

template<typename T1, typename T2>
inline void InsertBinaryOpFunc(Op op, const Interpretor::BinaryFunction& func) {
    //Interpretor::binaryOp.insert(op, {});
    auto& inner = Interpretor::binaryOp[op];
    auto& innerLeft = inner[std::type_index(typeid(int32_t))];
    innerLeft[std::type_index(typeid(int32_t))] = func;
}

void Interpretor::InitBinaryFunction() {
    InsertBinaryOpFunc<int32_t, int32_t>(Op::Plus, PlusIntInt<int32_t, int32_t>); //'+'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::Minus, MinusIntInt<int32_t, int32_t>); //'-'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::Multiply, MultiplyIntInt<int32_t, int32_t>); //'*'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::Divide, DivideIntInt<int32_t, int32_t>); //'/'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::Modulus, ModulusIntInt<int32_t, int32_t>); //'%'

    InsertBinaryOpFunc<int32_t, int32_t>(Op::G, GreatIntInt<int32_t, int32_t>); //'>'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::GE, GEIntInt<int32_t, int32_t>); //'>='
    InsertBinaryOpFunc<int32_t, int32_t>(Op::L, LessIntInt<int32_t, int32_t>); //'<'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::LE, LEIntInt<int32_t, int32_t>); //'<='
    InsertBinaryOpFunc<int32_t, int32_t>(Op::EQ, EQIntInt<int32_t, int32_t>); //'=='
    InsertBinaryOpFunc<int32_t, int32_t>(Op::NE, NEIntInt<int32_t, int32_t>); //'!='
}

std::optional<Interpretor::BinaryFunction> Interpretor::GetBinaryFunction(Op op, const std::any& l, const std::any& r) {
    auto type1 = std::type_index(l.type());
    auto type2 = std::type_index(r.type());

    auto it = Interpretor::binaryOp.find(op);
    if (it == Interpretor::binaryOp.end()) {
        dbg("Unsupported binary, Op: " + toString(op));
        return std::nullopt;
    }

    auto itLeft = it->second.find(type1);
    if (itLeft == it->second.end()) {
        dbg("Unsupported binary, leftType: " + std::string(l.type().name()));
        return std::nullopt;
    }

    auto itRight = itLeft->second.find(type2);
    if (itRight == itLeft->second.end()) {
        dbg("Unsupported binary, rightType: " + std::string(r.type().name()));
        return std::nullopt;
    }

    return itRight->second;
}