#ifndef __eproto_cpp__
#define __eproto_cpp__

#include "eproto.hpp"

namespace alioth {

$eproto eproto::MakeUp( $scope scope, etype tele, $typeuc tdat, const token& fconst ) {
    if( !scope or !tdat ) return nullptr;
    $eproto ret = new eproto;
    ret->phrase = tdat->phrase;
    ret->cons = fconst;
    ret->dtype = tdat;
    ret->elmt = tele==UDF?tdat->is(typeuc::PointerType)?PTR:OBJ:tele;
    ret->setScope(scope);
    return ret;
}

$eproto eproto::copy()const {
    return MakeUp(
        dtype->name.getScope(),
        elmt,
        dtype,
        cons
    );
}

void eproto::setScope( $scope sc ) {
    dtype->setScope(sc);
}

}

#endif