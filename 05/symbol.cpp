#include "symbol.h"
#include "types.h"

#include <map>
#include <string>


std::string SymKindtoString(SymKind kind) {
    static std::map<SymKind, std::string> kindString {
        {SymKind::Variable,  "SymKind::Variable"},
        {SymKind::Function,  "SymKind::Function"},
        {SymKind::Class,     "SymKind::Class"},
        {SymKind::Interface, "SymKind::Interface"},
        {SymKind::Parameter, "SymKind::Parameter"},
        {SymKind::Prog,      "SymKind::Prog"},

    };
    auto it = kindString.find(kind);
    if (it != kindString.end()) {
        return it->second;
    }
    return "";
}


std::vector<Type*> FUN_println_parms{&SysTypes::String};
FunctionType FUN_println_type(&SysTypes::Void, FUN_println_parms);

std::shared_ptr<Symbol>  FUN_println_varSymbol = std::make_shared<VarSymbol>("a", SysTypes::String);
FunctionSymbol FUN_println("println", FUN_println_type, {FUN_println_varSymbol});

FunctionType FUN_tick_type(&SysTypes::Integer, {});
FunctionSymbol FUN_tick("tick", FUN_tick_type, {});

std::vector<Type*> FUN_integer_to_string_parms{&SysTypes::Integer};
FunctionType FUN_integer_to_string_type(&SysTypes::String, FUN_integer_to_string_parms);
std::shared_ptr<Symbol>  FUN_integer_to_string_varSymbol = std::make_shared<VarSymbol>("a", SysTypes::Integer);
FunctionSymbol FUN_integer_to_string("integer_to_string", FUN_integer_to_string_type, {FUN_integer_to_string_varSymbol});

std::map<std::string, FunctionSymbol&> built_ins {
    {"println", FUN_println},
    {"tick", FUN_tick},
    {"integer_to_string", FUN_integer_to_string},
};