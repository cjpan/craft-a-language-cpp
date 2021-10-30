#include "common.h"

#include <stdio.h>
#include <iostream>
#include <string>
#include <map>


std::string Print(const std::string& str, Color color) {
    static std::map<Color, std::string> colors {
        {Color::Reset, "\033[0m"},
        {Color::Red, "\033[1;31m"},
        {Color::Yellow, "\033[1;33m"},
        {Color::Green, "\033[0;32m"},
        {Color::Blue, "\033[0;34m"},
    };

    printf(colors[color].c_str());
    printf("%s\n", str.c_str());
    printf(colors[Color::Reset].c_str());

    return str + "\n";
}
