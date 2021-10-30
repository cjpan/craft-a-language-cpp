#ifndef __COMMON_H_
#define __COMMON_H_

#include <string>
#include <type_traits>
#include <any>

enum class Color {
    Reset,
    Red,
    Yellow,
    Green,
    Blue,

};

template<typename T>
bool isType(const std::any& a) {
    return typeid(T) == a.type();
}

std::string Print(const std::string& str, Color color = Color::Red);

#endif