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
#ifndef __TYPES_H_
#define __TYPES_H_

#include <stdint.h>
#include <string>
#include <any>
#include <vector>
#include <memory>

/**
 * 类型体系
 */
class Type{
public:
    std::string name;

    Type(const std::string& name): name(name){}

    /**
     * 类型中是否包含void。
     */
    virtual bool hasVoid() = 0;

    virtual std::string toString() = 0;
};

bool operator==(const Type& ls, const Type& rs) {
    if (ls.name == rs.name) {
        return true;
    }
    return false;
}

class SimpleType: public Type{
    std::vector<Type*> upperTypes;
public:
    SimpleType(const std::string& name, std::vector<Type*> upperTypes = {}):
        Type(name), upperTypes(upperTypes){
    }

    bool hasVoid() {
        return false;
    }

    std::string toString() {
        std::string upperTypeNames = "[";
        for (auto ut: this->upperTypes){
            upperTypeNames += ut->name +", ";
        }
        upperTypeNames += "]";
        return "SimpleType {name: " + this->name + ", upperTypes: " + upperTypeNames+ "}";
    }
};

class SysTypes {
public:
    //所有类型的父类型
    static SimpleType Any;

    //基础类型
    static SimpleType String;
    static SimpleType Number;
    static SimpleType Boolean;

    //所有类型的子类型
    static SimpleType Null;
    static SimpleType Undefined;

    //函数没有任何返回值的情况
    //如果作为变量的类型，则智能赋值为null和undefined
    static SimpleType Void;

    //两个Number的子类型
    static SimpleType Integer;
    static SimpleType Decimal;

    static isSysType(const Type& t){
        return t == SysTypes::Any || t == SysTypes::String || t == SysTypes::Number ||
               t == SysTypes::Boolean || t == SysTypes::Null || t == SysTypes::Undefined ||
               t == SysTypes::Void || t == SysTypes::Integer || t == SysTypes::Decimal;
    }
};

//所有类型的父类型
SimpleType SysTypes::Any{"any", {}};

//基础类型
SimpleType SysTypes::String{"string",{&SysTypes::Any}};
SimpleType SysTypes::Number{"number",{&SysTypes::Any}};
SimpleType SysTypes::Boolean{"boolean", {&SysTypes::Any}};

//所有类型的子类型
SimpleType SysTypes::Null{"null"};
SimpleType SysTypes::Undefined{"undefined"};

//函数没有任何返回值的情况
//如果作为变量的类型，则智能赋值为null和undefined
SimpleType SysTypes::Void{"void"};

//两个Number的子类型
SimpleType SysTypes::Integer{"integer", {&SysTypes::Number}};
SimpleType SysTypes::Decimal{"decimal", {&SysTypes::Number}};


class FunctionType: public Type{
public:
    Type* returnType;
    std::vector<Type*>  paramTypes;
    static int32_t index; //序号，用于给函数类型命名
    FunctionType(Type* returnType = &SysTypes::Void, const std::vector<Type*>& paramTypes = {},
        std::string name = ""): Type("@function"), returnType(returnType), paramTypes(paramTypes){
        //用一个非法字符@，避免与已有的符号名称冲突
        if (name != "") {
            this->name += name;
        } else {
            this->name = "@function"+ std::to_string(FunctionType::index++);
        }

    }

    bool hasVoid() {
        return this->returnType->hasVoid();
    }

    std::string toString(){
        std::string paramTypeNames = "[";
        for (auto ut: this->paramTypes){
            paramTypeNames += ut->name +", ";
        }
        paramTypeNames += "]";
        return "FunctionType {name: " + this->name + ", returnType: " + this->returnType->name + ", paramTypes: " + paramTypeNames+ "}";
    }
};

int32_t FunctionType::index = {0};


#endif
