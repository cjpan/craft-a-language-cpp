#include "vm.h"


std::string toString(OpCode op) {
    static std::map<OpCode, std::string> opCodeToString = {
        {OpCode::iconst_0    ,           "iconst_0"    },
        {OpCode::iconst_1    ,           "iconst_1"    },
        {OpCode::iconst_2    ,           "iconst_2"    },
        {OpCode::iconst_3    ,           "iconst_3"    },
        {OpCode::iconst_4    ,           "iconst_4"    },
        {OpCode::iconst_5    ,           "iconst_5"    },
        {OpCode::bipush      ,           "bipush"      },
        {OpCode::sipush      ,           "sipush"      },
        {OpCode::ldc         ,           "ldc"         },
        {OpCode::iload       ,           "iload"       },
        {OpCode::iload_0     ,           "iload_0"     },
        {OpCode::iload_1     ,           "iload_1"     },
        {OpCode::iload_2     ,           "iload_2"     },
        {OpCode::iload_3     ,           "iload_3"     },
        {OpCode::istore      ,           "istore"      },
        {OpCode::istore_0    ,           "istore_0"    },
        {OpCode::istore_1    ,           "istore_1"    },
        {OpCode::istore_2    ,           "istore_2"    },
        {OpCode::istore_3    ,           "istore_3"    },
        {OpCode::iadd        ,           "iadd"        },
        {OpCode::isub        ,           "isub"        },
        {OpCode::imul        ,           "imul"        },
        {OpCode::idiv        ,           "idiv"        },
        {OpCode::iinc        ,           "iinc"        },
        {OpCode::lcmp        ,           "lcmp"        },
        {OpCode::ifeq        ,           "ifeq"        },
        {OpCode::ifne        ,           "ifne"        },
        {OpCode::iflt        ,           "iflt"        },
        {OpCode::ifge        ,           "ifge"        },
        {OpCode::ifgt        ,           "ifgt"        },
        {OpCode::ifle        ,           "ifle"        },
        {OpCode::if_icmpeq   ,           "if_icmpeq"   },
        {OpCode::if_icmpne   ,           "if_icmpne"   },
        {OpCode::if_icmplt   ,           "if_icmplt"   },
        {OpCode::if_icmpge   ,           "if_icmpge"   },
        {OpCode::if_icmpgt   ,           "if_icmpgt"   },
        {OpCode::if_icmple   ,           "if_icmple"   },
        {OpCode::igoto       ,           "igoto"       },
        {OpCode::ireturn     ,           "ireturn"     },
        {OpCode::vreturn     ,           "vreturn"     },
        {OpCode::invokestatic,           "invokestatic"},

        {OpCode::sadd        ,           "sadd"        },
        {OpCode::sldc        ,           "sldc"        },
    };

    auto iter = opCodeToString.find(op);
    if (iter == opCodeToString.end()) {
        char tmp[8] = {0};
        snprintf(tmp, sizeof(tmp), "%02x", static_cast<uint8_t>(op));
        return std::string(tmp);
    }

    return iter->second;
}

std::map<OpCode,
        std::map<std::type_index,
            std::map<std::type_index,
                VM::BinaryFunction>>> VM::binaryOp = {

    //{OpCode::Plus, { std::type_index(typeid(int32_t)), {std::type_index(typeid(int32_t)), PlusIntInt} } };

};

std::once_flag VM::flag;

template<typename T1, typename T2>
inline void InsertBinaryOpFunc(OpCode op, const VM::BinaryFunction& func) {
    //VM::binaryOp.insert(op, {});
    auto& inner = VM::binaryOp[op];
    auto& innerLeft = inner[std::type_index(typeid(T1))];
    innerLeft[std::type_index(typeid(T2))] = func;
}

void VM::InitBinaryFunction() {
    InsertBinaryOpFunc<int32_t, int32_t>(OpCode::iadd, PlusIntInt<int32_t, int32_t>); //'+'
    InsertBinaryOpFunc<int8_t, int32_t>(OpCode::iadd, PlusIntInt<int8_t, int32_t>); //'+'
    InsertBinaryOpFunc<int32_t, int8_t>(OpCode::iadd, PlusIntInt<int32_t, int8_t>); //'+'
    InsertBinaryOpFunc<int8_t, int8_t>(OpCode::iadd, PlusIntInt<int8_t, int8_t>); //'+'

    InsertBinaryOpFunc<int32_t, int32_t>(OpCode::isub, MinusIntInt<int32_t, int32_t>); //'-'
    InsertBinaryOpFunc<int8_t, int32_t>(OpCode::isub, MinusIntInt<int8_t, int32_t>); //'-'
    InsertBinaryOpFunc<int32_t, int8_t>(OpCode::isub, MinusIntInt<int32_t, int8_t>); //'-'
    InsertBinaryOpFunc<int8_t, int8_t>(OpCode::isub, MinusIntInt<int8_t, int8_t>); //'-'

    InsertBinaryOpFunc<int32_t, int32_t>(OpCode::imul, MultiplyIntInt<int32_t, int32_t>); //'*'
    InsertBinaryOpFunc<int8_t, int32_t>(OpCode::imul, MultiplyIntInt<int8_t, int32_t>); //'*'
    InsertBinaryOpFunc<int32_t, int8_t>(OpCode::imul, MultiplyIntInt<int32_t, int8_t>); //'*'
    InsertBinaryOpFunc<int8_t, int8_t>(OpCode::imul, MultiplyIntInt<int8_t, int8_t>); //'*'

    InsertBinaryOpFunc<int32_t, int32_t>(OpCode::idiv, DivideIntInt<int32_t, int32_t>); //'/'
    InsertBinaryOpFunc<int8_t, int32_t>(OpCode::idiv, DivideIntInt<int8_t, int32_t>); //'/'
    InsertBinaryOpFunc<int32_t, int8_t>(OpCode::idiv, DivideIntInt<int32_t, int8_t>); //'/'
    InsertBinaryOpFunc<int8_t, int8_t>(OpCode::idiv, DivideIntInt<int8_t, int8_t>); //'/'
}

std::optional<VM::BinaryFunction> VM::GetBinaryFunction(OpCode op, const std::any& l, const std::any& r) {
    auto type1 = std::type_index(l.type());
    auto type2 = std::type_index(r.type());

    auto it = VM::binaryOp.find(op);
    if (it == VM::binaryOp.end()) {
        dbg("Unsupported binary, OpCode: " + toString(op));
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