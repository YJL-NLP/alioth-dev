#ifndef __attrdef_cpp__
#define __attrdef_cpp__

#include "attrdef.hpp"

namespace alioth {

bool AttrDef::is( cnode n ) const {
    return n == DEFINITION or n == ATTRIBUTEDEF;
}

}

#endif