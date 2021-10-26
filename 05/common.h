#ifndef __COMMON_H_
#define __COMMON_H_

#include <string>

enum class Color {
    Reset,
    Red,
    Yellow,
    Green,
    Blue,

};

void Print(const std::string& str, Color color = Color::Red);

#endif