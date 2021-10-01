// MIT License

// Copyright (c) 2021 caofeixiang_hw

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <memory>



void compileAndRun(const std::string& program) {
/*
    {
        CharStream charStream(program);
        Tokenizer tokenizer(charStream);
        while(tokenizer.peek().kind!=TokenKind::Eof){
            auto t = tokenizer.next();
            std::cout << t << std::endl;
        }
    }


    std::cout << "program start use tokens" << std::endl;
    CharStream charStream(program);
    Tokenizer tokenizer(charStream);

    std::cout << "ast after parser: " << std::endl;
    auto prog = Parser(tokenizer).parseProg();
    prog->dump("");

    std::cout << "ast after resolved: " << std::endl;
    RefResolver().visitProg(prog);
    prog->dump("");

    std::cout << "run prog: " << std::endl;
    Intepretor().visitProg(prog);
*/
}


static std::string ReadFile(const std::string& filename) {
    std::ifstream ifile(filename.c_str());
    if (!ifile.is_open()) {
        std::cout << "Open file: [" << filename << "] failed." << std::endl;
        return "";
    }

    std::ostringstream buf;
    char ch;
    while (buf && ifile.get(ch)) {
        buf.put(ch);
    }
    ifile.close();
    return buf.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
         std::cout << (std::string("Usage: ") + argv[0] + " FILENAME");
         return 0;
    }

    std::string program = ReadFile(argv[1]);
    std::cout << ("source code:") << std::endl;
    std::cout << (program) << std::endl;

    compileAndRun(program);

    return 0;
}