#ifndef __operatorimpl_cpp__
#define __operatorimpl_cpp__

#include "operatorimpl.hpp"

namespace alioth {

bool OperatorImpl::is( cnode n )const {
    return n == OPERATORIMPL or n == IMPLEMENTATION;
}

}

#endif