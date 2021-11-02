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

    virtual bool isSimpleType() = 0;
    virtual bool isFunctionType() = 0;

    virtual bool LE(Type& type2) = 0;

    static Type& getUpperBound(Type& type1, Type& type2);
};

bool operator==(const Type& ls, const Type& rs);

class SimpleType: public Type{
    std::vector<Type*> upperTypes;
    std::vector<std::shared_ptr<Type>> upperTypes_;
public:
    SimpleType(const std::string& name, std::vector<Type*> upperTypes = {}):
        Type(name), upperTypes(upperTypes){
    }

    SimpleType(int hold, const std::string& name, std::vector<std::shared_ptr<Type>> upperTypes_ = {}):
        Type(name), upperTypes_(upperTypes_){
    }

    bool hasVoid() override;

    std::string toString() override {
        std::string upperTypeNames = "[";
        for (auto ut: this->upperTypes){
            upperTypeNames += ut->name +", ";
        }
        upperTypeNames += "]";
        return "SimpleType {name: " + this->name + ", upperTypes: " + upperTypeNames+ "}";
    }

    bool isSimpleType() override {
        return true;
    }
    bool isFunctionType() override {
        return false;
    }

    bool LE(Type& type2);
};

class SysTypes {
public:
    //所有类型的父类型
    static SimpleType Any;

    static std::shared_ptr<Type> AnyType() {
        static std::shared_ptr<Type> type = std::make_shared<SimpleType>(0, "any");
        return type;
    }

    static std::shared_ptr<Type> StringType() {
        auto any = AnyType();
        std::vector<std::shared_ptr<Type>> vec{any};
        static std::shared_ptr<Type> type = std::make_shared<SimpleType>(0, "string", vec);
        return type;
    }

    static std::shared_ptr<Type> NumberType() {
        auto any = AnyType();
        std::vector<std::shared_ptr<Type>> vec{any};
        static std::shared_ptr<Type> type = std::make_shared<SimpleType>(0, "number", vec);
        return type;
    }

    static std::shared_ptr<Type> BooleanType() {
        auto any = AnyType();
        std::vector<std::shared_ptr<Type>> vec{any};
        static std::shared_ptr<Type> type = std::make_shared<SimpleType>(0, "boolean", vec);
        return type;
    }

    static std::shared_ptr<Type> NullType() {
        static std::shared_ptr<Type> type = std::make_shared<SimpleType>(0, "null");
        return type;
    }

    static std::shared_ptr<Type> UndefinedType() {
        static std::shared_ptr<Type> type = std::make_shared<SimpleType>(0, "undefined");
        return type;
    }

    static std::shared_ptr<Type> VoidType() {
        static std::shared_ptr<Type> type = std::make_shared<SimpleType>(0, "void");
        return type;
    }

    static std::shared_ptr<Type> IntegerType() {
        auto number = NumberType();
        std::vector<std::shared_ptr<Type>> vec{number};
        static std::shared_ptr<Type> type = std::make_shared<SimpleType>(0, "integer", vec);
        return type;
    }

    static std::shared_ptr<Type> DecimalType() {
        auto number = NumberType();
        std::vector<std::shared_ptr<Type>> vec{number};
        static std::shared_ptr<Type> type = std::make_shared<SimpleType>(0, "decimal", vec);
        return type;
    }

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

    bool hasVoid() override {
        return this->returnType->hasVoid();
    }

    bool isSimpleType() override {
        return false;
    }
    bool isFunctionType() override {
        return true;
    }

    std::string toString() override {
        std::string paramTypeNames = "[";
        for (auto ut: this->paramTypes){
            paramTypeNames += ut->name +", ";
        }
        paramTypeNames += "]";
        return "FunctionType {name: " + this->name + ", returnType: " + this->returnType->name + ", paramTypes: " + paramTypeNames+ "}";
    }

    bool LE(Type& type2) override {
        if (type2 == SysTypes::Any){
             return true;
        }
        else if (*this == type2){
            return true;
        }
        // else if (Type.isUnionType(type2)){
            // let t = type2 as UnionType;
            // if (t.types.indexOf(this)!=-1){
                // return true;
            // }
            // else{
                // return false;
            // }
        // }
        else{
            return false;
        }

        return false;
    }
};




#endif
