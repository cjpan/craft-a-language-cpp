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


std::vector<std::shared_ptr<Type>> FUN_println_parms{SysTypes::String()};
std::shared_ptr<Type> FUN_println_type = std::make_shared<FunctionType>(SysTypes::Void(), FUN_println_parms);

std::shared_ptr<Type> FUN_println_varSymbol_theType = SysTypes::String();
std::shared_ptr<Symbol>  FUN_println_varSymbol = std::make_shared<VarSymbol>("a", FUN_println_varSymbol_theType);
std::vector<std::shared_ptr<Symbol>> FUN_println_varsSymbol = {FUN_println_varSymbol};

std::shared_ptr<FunctionSymbol>  FUN_println = std::make_shared<FunctionSymbol>("println", FUN_println_type, FUN_println_varsSymbol);


////////////////////////////
std::vector<std::shared_ptr<Type>> FUN_tick_parms;
std::shared_ptr<Type> FUN_tick_type = std::make_shared<FunctionType>(SysTypes::Void(), FUN_tick_parms);

std::shared_ptr<FunctionSymbol>  FUN_tick = std::make_shared<FunctionSymbol>("tick", FUN_tick_type);

////////////////////////////
std::vector<std::shared_ptr<Type>> FUN_integer_to_string_parms{SysTypes::Integer()};
std::shared_ptr<Type> FUN_integer_to_string_type = std::make_shared<FunctionType>(SysTypes::String(), FUN_integer_to_string_parms);

std::shared_ptr<Type> FUN_integer_to_string_theType = SysTypes::Integer();
std::shared_ptr<Symbol>  FUN_integer_to_string_varSymbol = std::make_shared<VarSymbol>("a", FUN_integer_to_string_theType);
std::vector<std::shared_ptr<Symbol>> FUN_integer_to_string_varsSymbol = {FUN_integer_to_string_varSymbol};

std::shared_ptr<FunctionSymbol>  FUN_integer_to_string = std::make_shared<FunctionSymbol>("integer_to_string", FUN_integer_to_string_type, FUN_integer_to_string_varsSymbol);

std::map<std::string, std::shared_ptr<FunctionSymbol>> built_ins {
    {"println", FUN_println},
    {"tick", FUN_tick},
    {"integer_to_string", FUN_integer_to_string},
};
