#ifndef __typeuc_cpp__
#define __typeuc_cpp__

#include "typeuc.hpp"
#include "classdef.hpp"
#include "nameuc.hpp"

namespace alioth {

typeuc::typeuc( TypeID _id ):id(_id) {}
typeuc::typeuc( $typeuc s, bool constrainted ):id(constrainted?ConstraintedPointerType:UnconstraintedPointerType), sub(s) {}
typeuc::typeuc( $ClassDef def ):id(CompositeType),sub(def){}
typeuc::typeuc( const nameuc& nm ):id(NamedType),name(nm) {}

$typeuc typeuc::GetUnknownType() { return new typeuc(UnknownType); }
$typeuc typeuc::GetNamedType( const nameuc& nm ) { return new typeuc(nm); }
$typeuc typeuc::GetVoidType() { return new typeuc(VoidType); }
$typeuc typeuc::GetBasicDataType( TypeID _id ) {
    if( (_id&BasicType) == BasicType and _id != BasicType ) return new typeuc(_id);
    else return nullptr;
}
$typeuc typeuc::GetPointerType( $typeuc _sub, bool constrainted ){
    if( !_sub or _sub->is(UnknownType) ) return nullptr;
    return new typeuc(_sub,constrainted);
}
$typeuc typeuc::GetCompositeType( $ClassDef def ) {
    if( !def ) return nullptr;
    return new typeuc(def);
}

$typeuc typeuc::getPointerTo( bool constrainted ) {
    return GetPointerType( this, constrainted );
}

int typeuc::getPointerDeepth()const {
    if( auto subt = ($typeuc)sub; subt and is(PointerType) ) return 1 + subt->getPointerDeepth();
    return 0;
}

void typeuc::setScope( $scope sc ) {
    name.setScope(sc);
    if( auto subt = ($typeuc)sub; subt ) subt->setScope(sc);
}

}

#endif