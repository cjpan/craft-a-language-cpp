#ifndef __COMMON_H_
#define __COMMON_H_

#include <string>
#include <type_traits>
#include <any>
#include <vector>

#include <iostream>
#include <sstream>

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

template<typename T1, typename T2>
std::any PlusIntInt(const std::any& l, const std::any& r) {
    T1 v1 = std::any_cast<T1>(l);
    T2 v2 = std::any_cast<T2>(r);
    std::any ret = v1 + v2;

    return ret;
}

template<typename T1, typename T2>
std::any MinusIntInt(const std::any& l, const std::any& r) {
    T1 v1 = std::any_cast<T1>(l);
    T2 v2 = std::any_cast<T2>(r);
    std::any ret = v1 - v2;

    return ret;
}

template<typename T1, typename T2>
std::any MultiplyIntInt(const std::any& l, const std::any& r) {
    T1 v1 = std::any_cast<T1>(l);
    T2 v2 = std::any_cast<T2>(r);
    std::any ret = v1 * v2;

    return ret;
}

template<typename T1, typename T2>
std::any DivideIntInt(const std::any& l, const std::any& r) {
    T1 v1 = std::any_cast<T1>(l);
    T2 v2 = std::any_cast<T2>(r);
    std::any ret = v1 / v2;

    return ret;
}

template<typename T1, typename T2>
std::any ModulusIntInt(const std::any& l, const std::any& r) {
    T1 v1 = std::any_cast<T1>(l);;
    T2 v2 = std::any_cast<T2>(r);
    std::any ret = v1 % v2;

    return ret;
}

template<typename T1, typename T2>
std::any GreatIntInt(const std::any& l, const std::any& r) {
    T1 v1 = std::any_cast<T1>(l);;
    T2 v2 = std::any_cast<T2>(r);
    std::any ret = v1 > v2;

    return ret;
}

template<typename T1, typename T2>
std::any GEIntInt(const std::any& l, const std::any& r) {
    T1 v1 = std::any_cast<T1>(l);;
    T2 v2 = std::any_cast<T2>(r);
    std::any ret = v1 >= v2;

    return ret;
}

template<typename T1, typename T2>
std::any LessIntInt(const std::any& l, const std::any& r) {
    T1 v1 = std::any_cast<T1>(l);;
    T2 v2 = std::any_cast<T2>(r);
    std::any ret = v1 < v2;

    return ret;
}

template<typename T1, typename T2>
std::any LEIntInt(const std::any& l, const std::any& r) {
    T1 v1 = std::any_cast<T1>(l);;
    T2 v2 = std::any_cast<T2>(r);
    std::any ret = v1 <= v2;

    return ret;
}

template<typename T1, typename T2>
std::any EQIntInt(const std::any& l, const std::any& r) {
    T1 v1 = std::any_cast<T1>(l);
    T2 v2 = std::any_cast<T2>(r);
    std::any ret = v1 == v2;

    return ret;
}

template<typename T1, typename T2>
std::any NEIntInt(const std::any& l, const std::any& r) {
    T1 v1 = std::any_cast<T1>(l);
    T2 v2 = std::any_cast<T2>(r);
    std::any ret = v1 != v2;

    return ret;
}

std::string Print(const std::string& str, Color color = Color::Red);
std::string PrintHex(const std::vector<uint8_t>& byteCode, Color color = Color::Red);

template<typename T>
std::string PrintSeq(const T& seq, Color color = Color::Red) {
    std::stringstream os;
    os << "[";
    bool start = true;
    for(const auto& elem: seq){
        if (start) {
            start = false;
        } else {
            os << " ";
        }

        os << elem;
    }
    os << "]";

    std::string str = os.str();
    return Print(str, color);
}

void PrintAny(const std::any& a);
#endif