#ifndef __flowctrlimpl_cpp__
#define __flowctrlimpl_cpp__

#include "flowctrlimpl.hpp"

namespace alioth {

bool FlowCtrlImpl::is( cnode c ) const {
    return c == CONTROLIMPL or c == IMPLEMENTATION;
}

}

#endif