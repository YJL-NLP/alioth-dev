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

    METHOD,
    CLASS,
    ENUM,
    VAR,
    PTR,
    REF,
    VAL,

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
    LIST,

    BRANCH,
    LOOP,

    CLASS_TEMPLATE_PARAMLIST,
    CLASS_BASE_CLASSES,
    CLASS_LAYOUT_ITEM,
    CLASS_PREDICATE,
};

}

#endif