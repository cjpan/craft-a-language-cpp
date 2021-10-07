#include <stdint.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <map>

#include <type_traits>
#include <any>
#include <functional>
#include <iomanip>

#include <typeindex>
#include <typeinfo>

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
        to_any_visitor<void>([]{ std::cout << "{}" << std::endl; }),
        to_any_visitor<int>([](int x){ std::cout << x << std::endl; }),
        to_any_visitor<unsigned>([](unsigned x){ std::cout << x << std::endl; }),
        to_any_visitor<float>([](float x){ std::cout << x << std::endl; }),
        to_any_visitor<double>([](double x){ std::cout << x << std::endl; }),
        to_any_visitor<char const*>([](char const *s)
            { std::cout << std::quoted(s) << std::endl; }),
        // ... add more handlers for your types ...
    };

inline void printAny(const std::any& a)
{
    if (!a.has_value()) {
        std::cout << "Not has value "<< std::endl;
        return;
    }

    if (const auto it = any_visitor.find(std::type_index(a.type()));
        it != any_visitor.cend()) {
        it->second(a);
    } else {
        std::cout << "Unregistered type "<< std::quoted(a.type().name()) << std::endl;
    }
}


template<class T1, class T2, class F>
std::pair<std::tuple<std::string, std::type_index, std::type_index>, std::function<std::any(std::any const&, std::any const&)>>
    to_any_binary(const std::string& op, F const &f)
{
    return {
        {op, std::type_index(typeid(T1)), std::type_index(typeid(T2))},
        [g = f](std::any const& a, std::any const& b) -> std::any
        {
            return g(std::any_cast<T1 const&>(a), std::any_cast<T2 const&>(b));
        }
    };
}

static std::map<std::tuple<std::string, std::type_index, std::type_index>, std::function<std::any(std::any const&, std::any const&)>>
    any_binary {
        to_any_binary<int, int>("-",
                [](int a, int b)-> std::any {
                    return a - b;
                }
        ),
        to_any_binary<int, int>("+",
                [](int a, int b)-> std::any {
                    return a + b;
                }
        ),

        to_any_binary<int, int>("*",
                [](int a, int b)-> std::any {
                    return a * b;
                }
        ),

        to_any_binary<int, int>("/",
                [](int a, int b)-> std::any {
                    return a / b;
                }
        ),

    };


inline std::any binaryOperation(const std::string& op, const std::any& a, const std::any& b)
{
    auto key = std::make_tuple(op, std::type_index(a.type()), std::type_index(b.type()));
    if (const auto it = any_binary.find(key);
        it != any_binary.cend()) {
        return it->second(a, b);
    } else {
        std::cout << "Unregistered type: "<< "(" + op + ", " << std::quoted(a.type().name()) <<  ", " <<
        std::quoted(b.type().name()) << ")"<< std::endl;
        return std::any();
    }
}

template<class T1, class T2>
std::any testFunc(std::any const& l, std::any const&r) {
    auto lv = std::any_cast<T1 const&>(l);
    auto rv = std::any_cast<T2 const&>(r);

    return lv + rv;
}

int main() {
    std::any a = 4;
    std::any b = 2;
    auto ret = testFunc<int, int>(a, b);
    printAny(ret);


    auto func =  to_any_binary<int, int>("-",
                [](int a, int b)-> std::any {
                    return a - b;
                }
        );



    auto s = func.second(a, b);
    printAny(s);

    auto s1 = binaryOperation("*", a, b);
    printAny(s1);
    auto s2 = binaryOperation("/", a, b);
    printAny(s2);


    std::any f = 0.2;
    auto s3 = binaryOperation("/", a, f);
    printAny(s3);

    return 0;
}