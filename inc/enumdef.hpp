#ifndef __enumdef__
#define __enumdef__

#include "definition.hpp"

namespace alioth {

struct EnumDef : public definition {

    public:
        tokens items;

    public:
        bool is( cnode ) const override;
        
};

using $EnumDef = agent<EnumDef>;

}

#endif