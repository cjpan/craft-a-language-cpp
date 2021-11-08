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

template<typename T>
bool CheckType(const std::any& val, T t) {
    return isType<T>(val) && std::any_cast<T>(val) == t;
}

std::string Print(const std::string& str, Color color = Color::Red);

void PrintAny(const std::any& a);
#endif