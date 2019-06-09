#ifndef __imm_cpp__
#define __imm_cpp__

#include "imm.hpp"
#include "classdef.hpp"

namespace alioth {

imm::imm( immt T, Value* V, anything P, agent<imm> H ):t(T),v(V),p(P),h(H){}

$imm imm::address( Value* addr, $eproto proto, agent<imm> host ) {return new imm(adr,addr,(anything)proto,host);}
$imm imm::object( Value* obj, $eproto proto ) {return new imm(val,obj,(anything)proto);}
$eproto imm::eproto()const{ auto pro = ($eproto)p; return pro?pro->copy():pro; }

$imm imm::entity( Value* addr, $ClassDef def ) {return new imm(ety,addr,(anything)def); }
$ClassDef imm::metacls()const{ return ($ClassDef)p; }

$imm imm::function( Value* fp, $MethodDef prototype, agent<imm> host ) { return new imm(fun,fp,(anything)prototype,host); }
$MethodDef imm::prototype()const{ return ($MethodDef)p; }

$imm imm::member( Value* fp, $OperatorDef member, agent<imm> host ) { return new imm(mem,fp,(anything)member,host); }
$OperatorDef imm::member()const{ return t==mem?($OperatorDef)p:nullptr; }

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
bool imm::hasaddress()const{
    if( !v ) return false;
    auto proto = ($eproto)p;
    switch( t ) {
        default: return false;
        case ety: return true;
        case val:
            if( proto->elmt == REF or proto->elmt == REL ) return true;
            else return false;
        case adr:
            if( proto->elmt == REF or proto->elmt == REL ) return true;
            else return true;
    }
}

Value* imm::asparameter( IRBuilder<>& builder, etype e )const {
    if( !v ) return nullptr;
    auto proto = ($eproto)p;
    switch( t ) {
        default: return nullptr;
        case ety:
            if( e == PTR )
                return nullptr;
            else
                return v;
        case val: 
            if( e == REF or e == REL )
                return nullptr;
            else
                return v;
        case adr: 
            if( e == OBJ )
                if( proto->dtype->is(typeuc::CompositeType) ) return v;
                else return builder.CreateLoad(v);
            else if( e == PTR )
                if( proto->dtype->is(typeuc::PointerType) ) return builder.CreateLoad(v);
                else return nullptr;
            else if( e == REF )
                return v;
            else if( e == REL )
                return v;
            else
                return nullptr;
    }
}

Value* imm::asfunction()const {
    if( t == fun or t == mem ) return v;
    else return nullptr;
}

}

#endif