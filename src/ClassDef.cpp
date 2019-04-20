#ifndef __classdef_cpp__
#define __classdef_cpp__

#include "classdef.hpp"

namespace alioth {

bool ClassDef::is( cnode c ) const {
    return c == CLASSDEF or c == DEFINITION;
}

}

#endif