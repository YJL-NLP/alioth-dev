#ifndef __implementation__
#define __implementation__

#include "node.hpp"

namespace alioth {

struct implementation : public node {

};

using $implementation = agent<implementation>;
using implementations = chainz<$implementation>;

}

#endif