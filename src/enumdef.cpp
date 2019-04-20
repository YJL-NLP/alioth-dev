#ifndef __enumdef_cpp__
#define __enumdef_cpp__

#include "enumdef.hpp"

namespace alioth {

bool EnumDef::is( cnode c ) const {
    return c == ENUMDEF or c == DEFINITION;
}

}

#endif