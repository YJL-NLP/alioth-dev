#ifndef __token_cpp__
#define __token_cpp__

#include "token.hpp"
#include "xengine.hpp"

using namespace std;
namespace alioth {

token::token(VT v ):id(v),in(VN::TERMINAL),bl(0),bc(0),el(0),ec(0) {

}

token::token(VN n):token(VT::R_ERR) {
    in = n;
}
token::token(const string& str):token(VT::LABEL) {
    tx = str;
}

token::operator std::string()const {
    return Xengine::written(*this);
}

token::operator bool()const {
    return !is(VT::R_ERR);
}

bool token::is( VT v )const {
    return in == VN::TERMINAL and id == v;
}

bool token::is( VN v )const {
    return in == v;
}

bool token::is( CT v )const {
    switch( v ) {
        case CT::BASIC_TYPE:
            return is(
                VT::INT8,VT::INT16,VT::INT32,VT::INT64,
                VT::UINT8,VT::UINT16,VT::UINT32,VT::UINT64,
                VT::FLOAT32,VT::FLOAT64,
                VT::BOOL,
                VT::NIL);  //2019/03/28 将nil视为基础数据类型之一
        case CT::CONSTANT:
            return is(
                VT::iCHAR,VT::iSTRING,
                VT::iFALSE,VT::iTRUE,
                VT::iNULL, //VT::NIL, [2019/03/28] 将nil从常量中剔除,从此引用不存在空值
                VT::iFLOAT,
                VT::iINTEGERb,VT::iINTEGERh,VT::iINTEGERn,VT::iINTEGERo
            );
        case CT::ASSIGN:
            return is(
                VT::ASSIGN,
                VT::ASSIGN_bAND,VT::ASSIGN_bOR,VT::ASSIGN_bXOR,
                VT::ASSIGN_SHL,VT::ASSIGN_SHR,
                VT::ASSIGN_PLUS,VT::ASSIGN_MINUS,
                VT::ASSIGN_MUL,VT::ASSIGN_DIV,VT::ASSIGN_MOL
            );
        case CT::RELATION:
            return is(
                VT::LT,VT::LE,
                VT::GT,VT::GE,
                VT::EQ,VT::NE
            );
        case CT::OPERATOR: 
            return is(
                VT::MEMBER,
                VT::WHERE,
                VT::INCRESS,VT::DECRESS,
                VT::NOT,
                VT::bREV,
                VT::SHL,VT::SHR,
                VT::bAND,
                VT::bXOR,
                VT::MOL,VT::MUL,VT::DIV,
                VT::PLUS,VT::MINUS,
                VT::RANGE,
                CT::RELATION,
                VT::AND,
                VT::OR,
                CT::ASSIGN
            );
        case CT::PREFIX:
            return is(
                VT::PLUS,VT::MINUS,
                VT::bAND,VT::MUL,
                VT::INCRESS,VT::DECRESS,
                VT::bREV
            );
        case CT::SUFFIX:
            return is(
                VT::INCRESS,VT::DECRESS
            );
        case CT::INFIX:
            return is(
                VT::MEMBER,
                VT::WHERE,
                VT::SHL,VT::SHR,
                VT::bAND,
                VT::bXOR,
                VT::bOR,
                VT::MOL,VT::MUL,VT::DIV,
                VT::PLUS,VT::MINUS,
                VT::RANGE,
                CT::RELATION,
                VT::AND,
                VT::OR,
                CT::ASSIGN
            );
        case CT::ELETYPE:
            return is(
                VT::VAR,VT::PTR,VT::REF,VT::VAL
            );
        case CT::IMPLEMENTATION:
            return is(
                VN::BRANCH,VN::LOOP,VN::CONTROL,VN::BLOCK,VN::EXPRESSION
            );
        default:
            return false;
    }
}

}
#endif