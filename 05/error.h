#ifndef __ERROR_H_
#define __ERROR_H_

#include <stdint.h>
#include <string>
#include "dbg.h"

struct Position {
    uint32_t begin = 1; //开始于哪个字符，从1开始计数
    uint32_t end = 1;   //结束于哪个字符
    uint32_t line = 1;  //所在的行号，从1开始
    uint32_t col = 1;   //所在的列号，从1开始

    Position() {}
    Position(uint32_t begin, uint32_t end, uint32_t line,uint32_t col):
        begin(begin), end(end), line(line), col(col) {
    }

    Position(const Position& other) {
        this->begin = other.begin;
        this->end = other.end;
        this->line = other.line;
        this->col = other.col;
    }

    std::string toString() {
        return "(ln: " + std::to_string(this->line) +
        ", col: " + std::to_string(this->col) +
        ", pos: " + std::to_string(this->begin) + ")";
    }
};

class CompilerError{
    std::string msg;
    const Position& beginPos; //在源代码中的第一个Token的位置
    // endPos:Position;   //在源代码中的最后一个Token的位置
    bool isWarning{false};  //如果是警告级，设为true。否则为错误级。
public:
    CompilerError(const std::string& msg, const Position& beginPos,
        /* endPos:Position, */ bool isWarning = false):
            msg(msg), beginPos(beginPos), isWarning(isWarning)
    {
    }
};

#endif