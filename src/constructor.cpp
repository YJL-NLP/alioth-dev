#ifndef __constructor_cpp__
#define __constructor_cpp__

#include "constructorimpl.hpp"

namespace alioth {

bool ConstructorImpl::is( cnode c ) const {
    return c == IMPLEMENTATION or c == CONSTRUCTORIMPL;
}

}

#endif