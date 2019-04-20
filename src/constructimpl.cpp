#ifndef __constructimpl_cpp__
#define __constructimpl_cpp__

#include "constructimpl.hpp"

namespace alioth {

bool ConstructImpl::is( cnode n ) const {
    return n == IMPLEMENTATION or n == CONSTRUCTIMPL;
}

}

#endif