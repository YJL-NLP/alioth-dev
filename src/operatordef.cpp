#ifndef __operatordef_cpp__
#define __operatordef_cpp__

#include "operatordef.hpp"

namespace alioth {

bool OperatorDef::is( cnode n )const {
    return n == OPERATORDEF;
}

}

#endif