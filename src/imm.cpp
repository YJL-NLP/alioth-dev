#ifndef __imm_cpp__
#define __imm_cpp__

#include "imm.hpp"
#include "classdef.hpp"

namespace alioth {

imm::imm( immt T, Value* V, anything P, agent<imm> H ):t(T),v(V),p(P),h(H){}

$imm imm::address( Value* addr, $eproto proto, agent<imm> host ) {return new imm(adr,addr,(anything)proto,host);}
$imm imm::object( Value* obj, $eproto proto ) {return new imm(val,obj,(anything)proto);}
$eproto imm::eproto()const{ return ($eproto)p; }

$imm imm::entity( Value* addr, $ClassDef def ) {return new imm(ety,addr,(anything)def); }
$ClassDef imm::metacls()const{ return ($ClassDef)p; }

$imm imm::function( Value* fp, $MethodDef prototype, agent<imm> host ) { return new imm(fun,fp,(anything)prototype,host); }
$MethodDef imm::prototype()const{ return ($MethodDef)p; }

Value* imm::raw()const { return v; }

Value* imm::asobject( IRBuilder<>& builder )const {
    if( !v ) return nullptr;
    auto ret = v;
    switch( t ) {
        default: return nullptr;
        case ety: return builder.CreateLoad(v);
        case adr: ret = builder.CreateLoad(ret);[[fallthrough]];
        case val: {
            auto proto = ($eproto)p;
            if( !proto ) return nullptr;
            if( proto->elmt == REF or proto->elmt == REL ) ret = builder.CreateLoad(ret);
            return ret;
        } 
    }
}

Value* imm::asaddress( IRBuilder<>& builder )const {
    if( !v ) return nullptr;
    auto proto = ($eproto)p;
    switch( t ) {
        default: return nullptr;
        case ety: return v;
        case val:
            if( proto->elmt == REF or proto->elmt == REL ) return v;
            else return nullptr;
        case adr:
            if( proto->elmt == REF or proto->elmt == REL ) return builder.CreateLoad(v);
            else return v;
    }
}

Value* imm::asparameter( IRBuilder<>& builder, $eproto req )const {
    if( !v ) return nullptr;
    auto proto = ($eproto)p;
    switch( t ) {
        default: return nullptr;
        case ety:case val: return v;
        case adr: 
            if( proto->elmt == OBJ and proto->dtype->is(typeuc::CompositeType) ) return v;
            else return builder.CreateLoad(v);
    }
}

Value* imm::asfunction()const {
    if( t == fun ) return v;
    else return nullptr;
}

}

#endif