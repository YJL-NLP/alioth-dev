#ifndef __methodimpl_cpp__
#define __methodimpl_cpp__

#include "methodimpl.hpp"

namespace alioth {

bool MethodImpl::is( cnode c )const {
    return c == METHODIMPL or c == IMPLEMENTATION;
}

}

#endif