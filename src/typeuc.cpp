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
$typeuc typeuc::GetBasicDataType( VT vt ) {
    switch( vt ) {
        case VT::INT8:      return GetBasicDataType(Int8);
        case VT::INT16:     return GetBasicDataType(Int16);
        case VT::INT32:     return GetBasicDataType(Int32);
        case VT::INT64:     return GetBasicDataType(Int64);
        case VT::UINT8:     return GetBasicDataType(Uint8);
        case VT::UINT16:    return GetBasicDataType(Uint16);
        case VT::UINT32:    return GetBasicDataType(Uint32);
        case VT::UINT64:    return GetBasicDataType(Uint64);
        case VT::FLOAT32:   return GetBasicDataType(Float32);
        case VT::FLOAT64:   return GetBasicDataType(Float64);
        case VT::BOOL:      return GetBasicDataType(BooleanType);
        case VT::VOID:       return GetBasicDataType(VoidType);
        default:            return nullptr;
    }
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