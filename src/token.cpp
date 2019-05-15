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
                VT::VOID);
        case CT::CONSTANT:
            return is(
                VT::iCHAR,VT::iSTRING,
                VT::iFALSE,VT::iTRUE,
                VT::iNULL, //VT::VOID, [2019/03/28] 将nil从常量中剔除,从此引用不存在空值
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
                VT::BITREV,
                VT::SHL,VT::SHR,
                VT::BITAND,
                VT::BITXOR,
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
                VT::BITAND,VT::MUL,
                VT::INCRESS,VT::DECRESS,
                VT::BITREV
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
                VT::BITAND,
                VT::BITXOR,
                VT::BITOR,
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
                VT::OBJ,VT::PTR,VT::REF,VT::REL
            );
        case CT::IMPLEMENTATION:
            return is(
                VN::BRANCH,VN::LOOP,VN::CONTROL,VN::BLOCK,VN::EXPRESSION
            );
        case CT::MF_ABSTRACT: return tx == "abstract";
        case CT::MF_REV: return tx == "rev";
        case CT::MF_ISM: return tx == "ism";
        case CT::MF_PREFIX: return tx == "prefix";
        case CT::MF_SUFFIX: return tx == "suffix";
        case CT::MF_ATOMIC: return tx == "atomic";
        case CT::MF_RAW: return tx == "raw";
        case CT::LB_SCTOR: return tx == "sctor";
        case CT::LB_LCTOR: return tx == "lctor";
        case CT::LB_CCTOR: return tx == "cctor";
        case CT::LB_MCTOR: return tx == "mctor";
        case CT::LB_DTOR: return tx == "dtor";
        case CT::LB_MEMBER: return tx == "member" or is(VT::MEMBER);
        case CT::LB_WHERE: return tx == "where" or is(VT::WHERE);
        case CT::LB_MOVE: return tx == "move";
        case CT::LB_NEGATIVE: return tx == "negative";
        case CT::LB_BITREV: return tx == "bitrev" or is(VT::BITREV);
        case CT::LB_INCREASE: return tx == "increase" or is(VT::INCRESS);
        case CT::LB_DECREASE: return tx == "decrease" or is(VT::DECRESS);
        case CT::LB_INDEX: return tx == "index";
        case CT::LB_ADD: return tx == "add" or is(VT::PLUS);
        case CT::LB_SUB: return tx == "sub" or is(VT::MINUS);
        case CT::LB_MUL: return tx == "mul" or is(VT::MUL);
        case CT::LB_DIV: return tx == "div" or is(VT::DIV);
        case CT::LB_MOL: return tx == "mol" or is(VT::MOL);
        case CT::LB_BITAND: return tx == "bitand" or is(VT::BITAND);
        case CT::LB_BITOR: return tx == "bitor" or is(VT::BITOR);
        case CT::LB_BITXOR: return tx == "bitxor" or is(VT::BITXOR);
        case CT::LB_SHL: return tx == "shl" or is(VT::SHL);
        case CT::LB_SHR: return tx == "shr" or is(VT::SHR);
        case CT::LB_LT: return tx == "lt" or is(VT::LT);
        case CT::LB_GT: return tx == "gt" or is(VT::GT);
        case CT::LB_LE: return tx == "le" or is(VT::LE);
        case CT::LB_GE: return tx == "ge" or is(VT::GE);
        case CT::LB_EQ: return tx == "eq" or is(VT::EQ);
        case CT::LB_NE: return tx == "ne" or is(VT::NE);
        case CT::LB_ASSIGN: return tx == "assign" or is(VT::ASSIGN);
        case CT::OPL:
            return is(
                VN::OPL_INDEX,CT::OPL_ASSIGN,
                VN::OPL_SCTOR, VN::OPL_LCTOR, VN::OPL_CCTOR, VN::OPL_MCTOR, VN::OPL_DTOR, VN::OPL_MEMBER, VN::OPL_WHERE, VN::OPL_MOVE,VN::OPL_AS,
                VN::OPL_NEGATIVE, VN::OPL_BITREV, VN::OPL_INCREASE, VN::OPL_DECREASE,VN::OPL_NOT,
                VN::OPL_ADD, VN::OPL_SUB, VN::OPL_MUL, VN::OPL_DIV, VN::OPL_MOL,
                VN::OPL_BITAND, VN::OPL_BITOR, VN::OPL_BITXOR, VN::OPL_SHL, VN::OPL_SHR,
                VN::OPL_LT, VN::OPL_GT, VN::OPL_LE, VN::OPL_GE, VN::OPL_EQ, VN::OPL_NE,
                VN::OPL_AND,VN::OPL_OR,VN::OPL_XOR
            );
        case CT::OPL_ASSIGN:
            return is(
                VN::OPL_ASSIGN,
                VN::OPL_ASSIGN_ADD, VN::OPL_ASSIGN_SUB, VN::OPL_ASSIGN_MUL, VN::OPL_ASSIGN_DIV, VN::OPL_ASSIGN_MOL,
                VN::OPL_ASSIGN_SHL, VN::OPL_ASSIGN_SHR, VN::OPL_ASSIGN_BITAND, VN::OPL_ASSIGN_BITOR, VN::OPL_ASSIGN_BITXOR
            );
        case CT::OPL_SPECIAL:
            return is(
                VN::OPL_SCTOR, VN::OPL_LCTOR, VN::OPL_CCTOR, VN::OPL_MCTOR, VN::OPL_DTOR, VN::OPL_WHERE, VN::OPL_MOVE, VN::OPL_MEMBER, VN::OPL_AS
            );
        case CT::OPL_MONO:
            return is(
                VN::OPL_NEGATIVE, VN::OPL_BITREV, VN::OPL_INCREASE, VN::OPL_DECREASE, VN::OPL_NOT
            );
        case CT::OPL_BINO:
            return is(
                CT::OPL_ASSIGN,
                VN::OPL_ADD,VN::OPL_SUB,VN::OPL_MUL,VN::OPL_DIV,VN::OPL_MOL,
                VN::OPL_BITAND,VN::OPL_BITOR,VN::OPL_BITXOR,VN::OPL_SHL,VN::OPL_SHR,
                VN::OPL_LT,VN::OPL_GT,VN::OPL_LE,VN::OPL_GE,VN::OPL_EQ,VN::OPL_NE,
                VN::OPL_AND,VN::OPL_OR,VN::OPL_XOR
            );
        case CT::PP_ON: return tx == "on";
        case CT::PP_THEN: return tx == "then";
        default:
            return false;
    }
}

}
#endif