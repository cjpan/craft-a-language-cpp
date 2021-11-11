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