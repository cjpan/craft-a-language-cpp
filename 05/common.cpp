#include "common.h"

#include "dbg.h"

#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

std::string Print(const std::string& str, Color color) {
    static std::map<Color, std::string> colors {
        {Color::Reset, "\033[0m"},
        {Color::Red, "\033[1;31m"},
        {Color::Yellow, "\033[1;33m"},
        {Color::Green, "\033[0;32m"},
        {Color::Blue, "\033[0;34m"},
    };

    //printf(colors[color].c_str());
    printf("%s\n", str.c_str());
    //printf(colors[Color::Reset].c_str());

    return str + "\n";
}

template<class T, class F>
inline std::pair<const std::type_index, std::function<void(std::any const&)>>
    to_any_visitor(F const &f)
{
    return {
        std::type_index(typeid(T)),
        [g = f](std::any const &a)
        {
            if constexpr (std::is_void_v<T>)
                g();
            else
                g(std::any_cast<T const&>(a));
        }
    };
}

static std::unordered_map<
    std::type_index, std::function<void(std::any const&)>>
any_visitor {
    to_any_visitor<void>([]{ Print("{}"); }),
    to_any_visitor<int32_t>([](int32_t x){ Print(std::to_string(x)); }),
    to_any_visitor<uint32_t>([](uint32_t x){ Print(std::to_string(x)); }),
    to_any_visitor<float>([](float x){ Print(std::to_string(x)); }),
    to_any_visitor<double>([](double x){ Print(std::to_string(x)); }),
    to_any_visitor<char const*>([](char const *s){ Print(s); }),
    // ... add more handlers for your types ...
};

void PrintAny(const std::any& a) {
    if (const auto it = any_visitor.find(std::type_index(a.type()));
        it != any_visitor.cend()) {
        it->second(a);
    } else {
        dbg("Unregistered type " + std::string(a.type().name()));
    }
}
