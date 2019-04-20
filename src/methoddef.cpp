#ifndef __methoddef_cpp__
#define __methoddef_cpp__

#include "methoddef.hpp"

namespace alioth {

bool MethodDef::is( cnode c )const {
    return c == METHODDEF or c == DEFINITION;
}

}

#endif