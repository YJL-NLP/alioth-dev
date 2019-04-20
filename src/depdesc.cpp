#ifndef __depdesc_cpp__
#define __depdesc_cpp__

#include "depdesc.hpp"
#include "jsonz.hpp"
#include "modulesignature.hpp"
#include "modesc.hpp"
#include "xengine.hpp"

namespace alioth {

token depdesc::literal()const {
    if( alias.is(VT::iTHIS) ) return alias;
    else if( alias.is(VT::LABEL) ) return alias;
    return name;
}

string depdesc::from( bool tr ) {
    string ret;
    if( mfrom.is(VT::MEMBER) ) ret = ".";
    else if( mfrom.is(VT::LABEL) ) ret = mfrom;
    else if( mfrom.is(VT::iSTRING,VT::iCHAR) ) ret = Xengine::extractText(mfrom);
    else ret = "";

    if( ret == "." and tr ) {
        if( self ) ret = self->program;
    }
    
    return move(ret);
}

bool depdesc::is( cnode c ) const {
    return c == DEPENDENCY;
}

}

#endif