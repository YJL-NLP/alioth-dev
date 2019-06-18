#ifndef __imm_cpp__
#define __imm_cpp__

#include "imm.hpp"
#include "classdef.hpp"
#include "sengine.hpp"

namespace alioth {

imm::imm( immt T, Value* V, anything P, agent<imm> H ):t(T),v(V),p(P),h(H){}

bool imm::is( immt arg ) const { return arg == t; }
imm::immt imm::is()const{ return t; }

$imm imm::element( Value* addr, $eproto proto, agent<imm> host ) {return new imm(ele,addr,(anything)proto,host);}
$imm imm::instance( Value* obj, $eproto proto ) {return new imm(ins,obj,(anything)proto);}
$eproto imm::eproto()const { 
    switch( t ) {
        case ele:case ins:{
            auto pro = ($eproto)p; 
            return pro?pro->copy():pro;
        } break;
        case fun: {
            auto pro = ($MethodDef)p;
            if( pro ) return pro->rproto->copy();
            return nullptr;
        } break;
        case mem: {
            auto pro = ($OperatorDef)p;
            if( pro ) return pro->rproto->copy();
            return nullptr;
        } break;
    }
}

$imm imm::entity( Value* addr, $ClassDef def ) {return new imm(ele,addr,(anything)eproto::MakeUp(def->getScope(),OBJ,typeuc::GetEntityType(def))); }

$imm imm::function( Value* fp, $MethodDef prototype, agent<imm> host ) { return new imm(fun,fp,(anything)prototype,host); }
$MethodDef imm::prototype()const{ return ($MethodDef)p; }

$imm imm::member( Value* fp, $OperatorDef member, agent<imm> host ) { return new imm(mem,fp,(anything)member,host); }
$OperatorDef imm::member()const{ return t==mem?($OperatorDef)p:nullptr; }

Value* imm::raw()const { return v; }

Value* imm::asunit( IRBuilder<>& builder, Sengine& sengine )const {
    if( !v ) return nullptr;
    auto ret = v;
    switch( t ) {
        default: return nullptr;
        case ele: ret = builder.CreateLoad(ret);[[fallthrough]];
        case ins: {
            auto proto = ($eproto)p;
            if( !proto ) return nullptr;
            if( proto->elmt == REF or proto->elmt == REL ) ret = builder.CreateLoad(ret);
            return ret;
        } break;
        case mem: {
            auto fp = (Function*)v;
            auto proto = ($OperatorDef)p;
            if( proto->size() ) return nullptr;
            vector<Value*> args = {h->asaddress(builder,sengine)};
            auto ri = sengine.generateCall( builder, fp, args, proto->rproto );
            return ri->asunit(builder,sengine);
        } break;
    }
}

Value* imm::asaddress( IRBuilder<>& builder, Sengine& sengine )const {
    if( !v ) return nullptr;
    auto proto = ($eproto)p;
    switch( t ) {
        default: return nullptr;
        case ins:
            if( proto->elmt == REF or proto->elmt == REL ) return v;
            else return nullptr;
        case ele:
            if( proto->elmt == REF or proto->elmt == REL ) return builder.CreateLoad(v);
            else return v;
        case mem: {
            auto fp = (Function*)v;
            auto proto = ($OperatorDef)p;
            if( proto->size() ) return nullptr;
            vector<Value*> args = {h->asaddress(builder,sengine)};
            auto ri = sengine.generateCall( builder, fp, args, proto->rproto );
            return ri->asaddress(builder,sengine);
        }
    }
}
bool imm::hasaddress()const{
    if( !v ) return false;
    auto proto = ($eproto)p;
    switch( t ) {
        default: return false;
        case ins:
            if( proto->elmt == REF or proto->elmt == REL ) return true;
            else return false;
        case ele:
            if( proto->elmt == REF or proto->elmt == REL ) return true;
            else return true;
    }
}

Value* imm::asparameter( IRBuilder<>& builder, Sengine& sengine, etype e )const {
    if( !v ) return nullptr;
    auto proto = ($eproto)p;
    switch( t ) {
        default: return nullptr;
        case ins: 
            if( e == REF or e == REL )
                return nullptr;
            else
                return v;
        case ele: 
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
        case mem:{
            auto fp = (Function*)v;
            auto proto = ($OperatorDef)p;
            if( proto->size() ) return nullptr;
            vector<Value*> args = {h->asaddress(builder,sengine)};
            auto ri = sengine.generateCall( builder, fp, args, proto->rproto );
            return ri->asparameter(builder,sengine,e);
        }

    }
}

Value* imm::asfunction()const {
    if( t == fun or t == mem ) return v;
    else return nullptr;
}

}

#endif