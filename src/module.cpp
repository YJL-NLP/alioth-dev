#ifndef __module_cpp__
#define __module_cpp__

#include "module.hpp"

namespace alioth {

bool module::is( cnode n ) const {
    return n == MODULE or n == DEFINITION;
}

anything module::getModule() {
    return this;
}

}

#endif