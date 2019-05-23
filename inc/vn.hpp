#ifndef __vn__
#define __vn__


namespace alioth {
/**
 * @enum-class VN : 非终结符ID
 * @desc : 用于区分不同非终结符的ID
 */
enum class VN {
    TERMINAL,
    MODULE,
    DEPENDENCY,

    NAMEUC_ATOM,
    NAMEUC,

    ATTRIBUTE,
    OPERATOR,
    METHOD,
    CLASS,
    ENUM,

    PARAM,
    COMMA_PARAM,
    PARAM_LIST,
    FINAL_PARAM,

    TYPEUC,
    CONST,
    PROTO,

    ELEMENT,
    EXPRESSION,
    CONTROL,
    BLOCK,
    CONSTRUCTOR,
    LIST,

    BRANCH,
    LOOP,

    CLASS_TEMPLATE_PARAMLIST,
    CLASS_BASE_CLASSES,
    CLASS_PREDICATE,

    OPL_SCTOR,
    OPL_LCTOR,
    OPL_CCTOR,
    OPL_MCTOR,
    OPL_DTOR,
    OPL_AS,
    OPL_MEMBER,
    OPL_WHERE,
    OPL_MOVE,
    OPL_NEGATIVE,
    OPL_BITREV,
    OPL_INCREASE,
    OPL_DECREASE,
    OPL_INDEX,
    OPL_ADD,
    OPL_SUB,
    OPL_MUL,
    OPL_DIV,
    OPL_MOL,
    OPL_BITAND,
    OPL_BITOR,
    OPL_BITXOR,
    OPL_SHL,
    OPL_SHR,
    OPL_LT,
    OPL_GT,
    OPL_LE,
    OPL_GE,
    OPL_EQ,
    OPL_NE,
    OPL_AND,
    OPL_OR,
    OPL_XOR,
    OPL_NOT,
    OPL_ASSIGN,
    OPL_ASSIGN_ADD,
    OPL_ASSIGN_SUB,
    OPL_ASSIGN_MUL,
    OPL_ASSIGN_DIV,
    OPL_ASSIGN_MOL,
    OPL_ASSIGN_SHL,
    OPL_ASSIGN_SHR,
    OPL_ASSIGN_BITAND,
    OPL_ASSIGN_BITOR,
    OPL_ASSIGN_BITXOR,

    RAW,
};

}

#endif