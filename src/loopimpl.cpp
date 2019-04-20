#ifndef __loop_cpp__
#define __loop_cpp__

#include "loopimpl.hpp"

namespace alioth{

bool LoopImpl::is( cnode c ) const {
    return c == IMPLEMENTATION or c == LOOPIMPL;
}

}

#endif