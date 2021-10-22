#ifndef __SCOPE_H_
#define __SCOPE_H_

#include "symbol.h"
#include <string>
#include <map>
#include <memory>

class Scope{
    //以名称为key存储符号
    std::map<std::string, std::shared_ptr<Symbol>> name2sym;
public:
    //上级作用域
    Scope* enclosingScope {nullptr}; //顶级作用域的上一级是null

    Scope(Scope* enclosingScope): enclosingScope(enclosingScope){
    }

    /**
     * 把符号记入符号表（作用域）
     * @param name
     * @param sym
     */
    void enter(const std::string& name, std::shared_ptr<Symbol> sym){
        this->name2sym.insert({name, sym});
    }

    /**
     * 查询是否有某名称的符号
     * @param name
     */
    bool hasSymbol(const std::string& name){
        return this->name2sym.count(name) > 0;
    }

    /**
     * 根据名称查找符号。
     * @param name 符号名称。
     * @returns 根据名称查到的Symbol。如果没有查到，则返回null。
     */
    std::shared_ptr<Symbol> getSymbol(const std::string& name) {
        auto sym = this->name2sym.find(name);
        if (sym != this->name2sym.end()){
            return sym->second;
        }
        else{
            return nullptr;
        }
    }

    /**
     * 级联查找某个符号。
     * 先从本作用域查找，查不到就去上一级作用域，依此类推。
     * @param name
     */
    std::shared_ptr<Symbol> getSymbolCascade(const std::string& name) {
        auto sym = this->getSymbol(name);
        if (sym != nullptr){
            return sym;
        }
        else if (this->enclosingScope != nullptr){
            return this->enclosingScope->getSymbolCascade(name);
        }
        else{
            return nullptr;
        }
    }
};

#endif