#ifndef __PARSE_H_
#define __PARSE_H_

#include "scanner.h"
#include "error.h"
#include "scope.h"
#include "types.h"
#include "symbol.h"
#include "dbg.h"
#include "common.h"

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <any>
#include <stdint.h>

class Parser{
public:
    Scanner& scanner;
    Parser(Scanner& scanner): scanner(scanner){
    }

    std::vector<CompilerError> errors;   //语法错误
    std::vector<CompilerError> warnings; //语法报警

    void addError(const std::string msg, Position pos){
        this->errors.push_back(CompilerError(msg,pos,false));
        dbg("@" + pos.toString() +" : " + msg);
    }

    void addWarning(const std::string msg, Position pos){
        this->warnings.push_back(CompilerError(msg,pos,true));
        dbg("@" + pos.toString() +" : " + msg);
    }

};

#endif