#ifndef __insblockimpl__
#define __insblockimpl__

#include "implementation.hpp"

namespace alioth {

struct InsBlockImpl : public implementation {

    public:
        implementations impls;

    public:
        bool is( cnode ) const override;
        
};

using $InsBlockImpl = agent<InsBlockImpl>;

}

#endif