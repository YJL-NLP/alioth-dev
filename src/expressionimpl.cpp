#ifndef __expressionimpl_cpp__
#define __expressionimpl_cpp__

#include "expressionimpl.hpp"

namespace alioth {

bool ExpressionImpl::is( cnode c )const {
    return c == EXPRESSIONIMPL or c == IMPLEMENTATION;
}

}

#endif