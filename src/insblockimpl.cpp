#ifndef __insblockimpl_cpp__
#define __imsblockimpl_cpp__

#include "insblockimpl.hpp"

namespace alioth {

bool InsBlockImpl::is( cnode c ) const {
    return c == BLOCKIMPL or c == IMPLEMENTATION;
}

}

#endif