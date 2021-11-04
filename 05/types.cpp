#include "types.h"

#include <algorithm>
#include <memory>

bool operator==(const Type& ls, const Type& rs) {
    if (ls.name == rs.name) {
        return true;
    }
    return false;
}

std::shared_ptr<Type> Type::getUpperBound(std::shared_ptr<Type>& type1, std::shared_ptr<Type>& type2) {
    if(*type1 == *SysTypes::Any() || *type2 == *SysTypes::Any()){
        return SysTypes::Any();
    }
    else{
        if (type1->LE(type2)){
            return type2;
        }
        else if (type2->LE(type1)){
            return type1;
        }
        else{
            //return new UnionType([type1,type2]);
            return SysTypes::Any();
        }
    }
    return nullptr;
}

bool SimpleType::hasVoid() {
    if (*this == *SysTypes::Void()) {
        return true;
    } else {
        for (auto t: upperTypes) {
            if (t->hasVoid()) {
                return true;
            }
        }
    }
    return false;
}

bool SimpleType::LE(std::shared_ptr<Type>& type2) {

    if (*type2 == *SysTypes::Any()){
        return true;
    }
    else if (*this == *SysTypes::Any()){
        return false;
    }
    else if (*this == *type2){
        return true;
    }
    else if (type2->isSimpleType()){
        auto equal = [&](const std::shared_ptr<Type>& type) {
            return *type2 == *type;
        };
        if(std::find_if(this->upperTypes.begin(), this->upperTypes.end(), equal) != this->upperTypes.end()){
            return true;
        }
        else{
            //看看所有的父类型中，有没有一个是type2的子类型
            for (auto upperType: this->upperTypes){
                if (upperType->LE(type2)){
                    return true;
                }
            }
            return false;
        }
    }
    // else if (Type.isUnionType(type2)){
        // let t = type2 as UnionType;
        // if (t.types.indexOf(this)!=-1){
            // return true;
        // }
        // else{ //是联合类型中其中一个类型的子类型就行
            // for (let t2 of t.types){
                // if (this.LE(t2)){
                    // return true;
                // }
            // }
            // return false;
        // }
    // }
    else{
        return false;
    }
}


int32_t FunctionType::index = {0};
