#include "interpretor.h"

std::any PlusIntInt(const std::any& l, const std::any& r) {
    int32_t v1 = std::any_cast<int32_t>(l);
    int32_t v2 = std::any_cast<int32_t>(r);
    std::any ret = v1 + v2;

    return ret;
}

std::any MinusIntInt(const std::any& l, const std::any& r) {
    int32_t v1 = std::any_cast<int32_t>(l);
    int32_t v2 = std::any_cast<int32_t>(r);
    std::any ret = v1 - v2;

    return ret;
}

std::any MultiplyIntInt(const std::any& l, const std::any& r) {
    int32_t v1 = std::any_cast<int32_t>(l);
    int32_t v2 = std::any_cast<int32_t>(r);
    std::any ret = v1 * v2;

    return ret;
}

std::any DivideIntInt(const std::any& l, const std::any& r) {
    int32_t v1 = std::any_cast<int32_t>(l);
    int32_t v2 = std::any_cast<int32_t>(r);
    std::any ret = v1 / v2;

    return ret;
}

std::any ModulusIntInt(const std::any& l, const std::any& r) {
    int32_t v1 = std::any_cast<int32_t>(l);
    int32_t v2 = std::any_cast<int32_t>(r);
    std::any ret = v1 % v2;

    return ret;
}

std::any GreatIntInt(const std::any& l, const std::any& r) {
    int32_t v1 = std::any_cast<int32_t>(l);
    int32_t v2 = std::any_cast<int32_t>(r);
    std::any ret = v1 > v2;

    return ret;
}

std::any GEIntInt(const std::any& l, const std::any& r) {
    int32_t v1 = std::any_cast<int32_t>(l);
    int32_t v2 = std::any_cast<int32_t>(r);
    std::any ret = v1 >= v2;

    return ret;
}

std::any LessIntInt(const std::any& l, const std::any& r) {
    int32_t v1 = std::any_cast<int32_t>(l);
    int32_t v2 = std::any_cast<int32_t>(r);
    std::any ret = v1 < v2;

    return ret;
}

std::any LEIntInt(const std::any& l, const std::any& r) {
    int32_t v1 = std::any_cast<int32_t>(l);
    int32_t v2 = std::any_cast<int32_t>(r);
    std::any ret = v1 <= v2;

    return ret;
}

std::any EQIntInt(const std::any& l, const std::any& r) {
    int32_t v1 = std::any_cast<int32_t>(l);
    int32_t v2 = std::any_cast<int32_t>(r);
    std::any ret = v1 == v2;

    return ret;
}

std::any NEIntInt(const std::any& l, const std::any& r) {
    int32_t v1 = std::any_cast<int32_t>(l);
    int32_t v2 = std::any_cast<int32_t>(r);
    std::any ret = v1 != v2;

    return ret;
}


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
    InsertBinaryOpFunc<int32_t, int32_t>(Op::Plus, PlusIntInt); //'+'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::Minus, MinusIntInt); //'-'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::Multiply, MultiplyIntInt); //'*'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::Divide, DivideIntInt); //'/'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::Modulus, DivideIntInt); //'%'

    InsertBinaryOpFunc<int32_t, int32_t>(Op::G, GreatIntInt); //'>'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::GE, GEIntInt); //'>='
    InsertBinaryOpFunc<int32_t, int32_t>(Op::L, LessIntInt); //'<'
    InsertBinaryOpFunc<int32_t, int32_t>(Op::LE, LEIntInt); //'<='
    InsertBinaryOpFunc<int32_t, int32_t>(Op::EQ, EQIntInt); //'=='
    InsertBinaryOpFunc<int32_t, int32_t>(Op::NE, NEIntInt); //'!='
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