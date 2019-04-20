#ifndef __branch_cpp__
#define __branch_cpp__

#include "branchimpl.hpp"
#include "yengine.hpp"

namespace alioth{
    
bool BranchImpl::is( cnode c ) const{
    return c == BRANCHIMPL or c == IMPLEMENTATION;
}

}

#endif
