#ifndef __modulesignature_cpp__
#define __modulesignature_cpp__

#include "modulesignature.hpp"

namespace alioth {

ModuleSignature::ModuleSignature( const std::string& pname ) : name(pname) {

}

bool ModuleSignature::is( cnode type ) const {
    return type == SIGNATURE;
}

}

#endif