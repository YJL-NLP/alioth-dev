#ifndef __sengine_cpp__
#define __sengine_cpp__

#include "sengine.hpp"
#include "manager.hpp"
#include "xengine.hpp"
#include "methodimpl.hpp"
#include "insblockimpl.hpp"
#include "constructimpl.hpp"
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include "llvm/IR/GlobalVariable.h"
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/Optional.h>
#include <llvm/Support/Host.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cctype>
#include <stdexcept>

namespace alioth {

bool Sengine::performDefinitionSemanticValidation( $modesc desc ) {

    if( mrepo.count(desc) == 0 ) return false;
    auto& mod = mrepo[desc];
    bool fine = true;

    for( auto& dep : desc->deps ) {
        //[条件不应成立] if( mrepo.count(dep->dest) != 1 ) return false;

        if( dep->alias.is(VT::iTHIS) ) {
            auto& dest = mrepo[dep->dest];
            mod->external += dest->internal;
            mod->extmeta += dest->metadefs;
        }
    }

    for( auto& def : mod->internal ) {
        if( auto cdef = ($ClassDef)def; cdef ) 
            fine = performDefinitionSemanticValidation(cdef) and fine;
        /*else if( auto edef = ($EnumDef)def; edef )
            fine = performDefinitionSemanticValidation(edef) and fine;
        */
    }

    vector<Type*> members;
    for( auto def : mod->metadefs ) {
        if( auto adef = ($AttrDef)def; adef ) {
            if( auto mty = performDefinitionSemanticValidation(adef); mty ) members.push_back(mty);
            else fine = false;
        } else if( auto mdef = ($MethodDef)def; mdef ) {
            fine = performDefinitionSemanticValidation(mdef) and fine;
        }
    }
    if( members.empty() ) members.push_back(Type::getInt32Ty(mctx));

    auto symbol = generateGlobalUniqueName(($node)mod,Meta);
    /*auto& modT = mmetaT[mod] =*/ 
    if( mnamedT.count(symbol) == 0 ) mnamedT[symbol] = StructType::create(mctx,symbol);
    auto lty = (StructType*)mnamedT[symbol];
    lty->setBody(members);

    return fine;
}

bool Sengine::performDefinitionSemanticValidation( $ClassDef clas ) {

    bool fine = true;
    map<string,$definition> nameT;

    auto perform = [&]( auto all, auto symbol, Type*& slot ) {
        if( !slot ) slot = StructType::create(mctx,symbol);
        vector<Type*> members;
        for( auto def : all ) {
            if( nameT.count((string)def->name) ) {
                auto prev = nameT[(string)def->name];
                mlogrepo(def->getDocPath())(Lengine::E2001,def->name,prev->getDocPath(),prev->name);
                fine = false;
            } else if( auto adef = ($AttrDef)def; adef ) {
                auto mty = performDefinitionSemanticValidation(adef);
                nameT[(string)def->name] = def;
                members.push_back(mty);
                if( !mty ) fine = false;
            } else if( auto mdef = ($MethodDef)def; mdef ) {
                auto symbol = generateGlobalUniqueName(($node)mdef);
                if( nameT.count(symbol) ) {
                    auto prev = nameT[symbol];
                    mlogrepo(mdef->getDocPath())(Lengine::E2001,def->name,prev->getDocPath(),prev->name);
                    fine = false;
                } else {
                    fine = performDefinitionSemanticValidation(mdef) and fine;
                }
            } else if( auto odef = ($OperatorDef)def; odef ) {
                //[TODO]: 检查重复
                fine = performDefinitionSemanticValidation(odef) and fine;
            }
        }
        ((StructType*)slot)->setBody(members);
    };

    auto instS = generateGlobalUniqueName(($node)clas);
    auto metaS = generateGlobalUniqueName(($node)clas,Meta);

    perform( clas->metadefs, metaS, mnamedT[metaS] );
    perform( clas->instdefs, instS, mnamedT[instS] );

    for( auto def : clas->internal ) {
        if( auto cdef = ($ClassDef)def; cdef ) fine = performDefinitionSemanticValidation(cdef) and fine;
        /*else if( auto edef = ($EnumDef)def; edef ) fine = performDefinitionSemanticValidation(edef) and fine;*/
    }
    return fine;
}

bool Sengine::performImplementationSemanticValidation( $ClassDef clas ) {
    auto tsymbol = generateGlobalUniqueName( ($node)clas, Meta );
    auto esymbol = generateGlobalUniqueName( ($node)clas, Entity );
    auto ty = (StructType*)mnamedT[tsymbol];
    if( !ty ) return false;
    if( ty->getNumElements() == 0 ) return true;

    new GlobalVariable(*mcurmod,ty,false,GlobalValue::ExternalLinkage,ConstantStruct::getNullValue(ty),esymbol);

    return true;
}

bool Sengine::performDefinitionSemanticValidation( $MethodDef method ) {
    vector<Type*> pts;
    bool fine = true;

    auto rtp = generateTypeUsageAsReturnValue(method->rproto,pts);
    if( !rtp ) fine = false;

    auto tss = generateGlobalUniqueName(method->getScope(),method->meta?Meta:None);
    if( mnamedT.count(tss) == 0 ) mnamedT[tss] = StructType::create(mctx,tss);
    pts.push_back(mnamedT[tss]->getPointerTo());
    
    for( auto par : *method ) {
        auto t = generateTypeUsageAsParameter(par->proto);
        if( !t ) fine = false;
        pts.push_back( t );
    }

    if( !fine ) return false;
    
    auto ft = FunctionType::get(rtp,pts,false);
    auto fs = generateGlobalUniqueName(($node)method);
    mnamedT[fs] = ft;
    return ft != nullptr;
}

bool Sengine::performDefinitionSemanticValidation( $OperatorDef opdef ) {
    
    vector<Type*> pts;
    bool fine = true;
    if( opdef->action ) {
        if( opdef->action.is(VT::DELETE) ) {
            if( !opdef->name.is(VN::OPL_SCTOR,VN::OPL_CCTOR,VN::OPL_MCTOR) ) 
                {mlogrepo(opdef->getDocPath())(Lengine::E2037,opdef->name);return false;}
            return true;
        } else if( opdef->action.is(VT::DEFAULT) ) {
            if( !opdef->name.is(VN::OPL_ASSIGN) ) {mlogrepo(opdef->getDocPath())(Lengine::E2038,opdef->name);return false;}

            if( !determineElementPrototype(opdef->rproto) ) fine = false;
            if( opdef->rproto->elmt != REF or opdef->rproto->dtype->sub != opdef->getScope() ) 
                {mlogrepo(opdef->getDocPath())(Lengine::E2041,opdef->rproto->phrase);fine = false;}
            if( opdef->size() != 1 or !determineElementPrototype((*opdef)[0]->proto) ) 
                {mlogrepo(opdef->getDocPath())(Lengine::E2042,opdef->phrase,opdef->name);fine = false;} 
            else if( auto proto = (*opdef)[0]->proto; 
                !proto->dtype->is(typeuc::CompositeType) or
                proto->dtype->sub != opdef->getScope() or
                not ((proto->elmt == REF and (bool)proto->cons) or (proto->elmt == REL and !(bool)proto->cons) )
            ) {mlogrepo(opdef->getDocPath())(Lengine::E2043,(*opdef)[0]->phrase,opdef->name);fine = false;}
            if( !fine ) return false;
            //[TODO]: 产生默认运算符体
            return true;
        } else {
            return false;
        }
    }

    //[TODO]: 检查其他细节。
    if( opdef->rproto and !determineElementPrototype(opdef->rproto) ) fine = false;
    for( auto par : *opdef ) if( !determineElementPrototype(par->proto) ) fine = false;
     
    auto rtp = generateTypeUsageAsReturnValue(opdef->rproto,pts);
    if( opdef->rproto and !rtp ) fine = false;

    if( opdef->name.is(CT::OPL_ASSIGN) ) {
        if( opdef->rproto->elmt != REF or opdef->rproto->dtype->sub != opdef->getScope() )
            {mlogrepo(opdef->getDocPath())(Lengine::E2041,opdef->rproto->phrase);fine = false;}
        if( opdef->size() > 1 )
            {mlogrepo(opdef->getDocPath())(Lengine::E2044,opdef->phrase);fine = false;}
        if( opdef->size() < 1 ) 
            {mlogrepo(opdef->getDocPath())(Lengine::E2046,opdef->phrase);fine = false;}
        if( opdef->modifier ) 
            {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}
        if( opdef->constraint ) 
            {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}
    } else if( opdef->name.is(VN::OPL_INDEX) ) {
        if( opdef->size() > 1 )
            {mlogrepo(opdef->getDocPath())(Lengine::E2044,opdef->phrase);fine = false;}
        if( opdef->size() < 1 ) 
            {mlogrepo(opdef->getDocPath())(Lengine::E2046,opdef->phrase);fine = false;}
        if( opdef->modifier ) 
            {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}
    } else if( opdef->name.is(CT::OPL_BINO) ) {
        if( opdef->size() > 1 )
            {mlogrepo(opdef->getDocPath())(Lengine::E2044,opdef->phrase);fine = false;}
        if( opdef->size() < 1 ) 
            {mlogrepo(opdef->getDocPath())(Lengine::E2046,opdef->phrase);fine = false;}
        if( opdef->modifier.is(CT::MF_PREFIX,CT::MF_SUFFIX) ) 
            {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}
    } else if( opdef->name.is(CT::OPL_MONO) ) {
        if( opdef->size() > 1 )
            {mlogrepo(opdef->getDocPath())(Lengine::E2044,opdef->phrase);fine = false;}
        if( opdef->size() < 1 ) 
            {mlogrepo(opdef->getDocPath())(Lengine::E2046,opdef->phrase);fine = false;}
        if( opdef->modifier.is(CT::MF_ISM,CT::MF_REV) ) 
            {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}
    } else if( opdef->name.is(CT::OPL_SPECIAL) ) {
        if( opdef->modifier ) 
            {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}

        if( opdef->name.is(VN::OPL_SCTOR ) ) {
            if( opdef->constraint ) 
                {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}
        } else if( opdef->name.is(VN::OPL_LCTOR) ) {
            if( opdef->constraint ) 
                {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}
        } else if( opdef->name.is(VN::OPL_CCTOR) ) {
            if( opdef->constraint ) 
                {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}
            if( opdef->size() != 1 )
                {mlogrepo(opdef->getDocPath())(Lengine::E2042,opdef->phrase,opdef->name);fine = false;}
            else if( auto proto = (*opdef)[0]->proto; proto->elmt != REF or !(bool)proto->cons or proto->dtype->sub != opdef->getScope() )
                {mlogrepo(opdef->getDocPath())(Lengine::E2042,opdef->phrase,opdef->name);fine = false;}
        } else if( opdef->name.is(VN::OPL_MCTOR) ) {
            if( opdef->constraint ) 
                {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}
            if( opdef->size() != 1 )
                {mlogrepo(opdef->getDocPath())(Lengine::E2042,opdef->phrase,opdef->name);fine = false;}
            else if( auto proto = (*opdef)[0]->proto; proto->elmt != REL or proto->dtype->sub != opdef->getScope() )
                {mlogrepo(opdef->getDocPath())(Lengine::E2042,opdef->phrase,opdef->name);fine = false;}
        } else if( opdef->name.is(VN::OPL_DTOR) ) {
            if( opdef->constraint )
                {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}
            if( opdef->size() != 0 )
                {mlogrepo(opdef->getDocPath())(Lengine::E2042,opdef->phrase,opdef->name);fine = false;}
        } else if( opdef->name.is(VN::OPL_MOVE) ) {
            if( opdef->constraint ) 
                {mlogrepo(opdef->getDocPath())(Lengine::E2045,opdef->modifier);fine = false;}
            if( opdef->size() != 1 )
                {mlogrepo(opdef->getDocPath())(Lengine::E2042,opdef->phrase,opdef->name);fine = false;}
            else if( auto proto = (*opdef)[0]->proto; proto->elmt != PTR or (($typeuc)proto->dtype->sub)->sub != opdef->getScope() )
                {mlogrepo(opdef->getDocPath())(Lengine::E2042,opdef->phrase,opdef->name);fine = false;}
        } else if( opdef->name.is(VN::OPL_MEMBER) ) {
            if( opdef->size() != 0 )
                {mlogrepo(opdef->getDocPath())(Lengine::E2042,opdef->phrase,opdef->name);fine = false;}
        } else if( opdef->name.is(VN::OPL_WHERE) ) {
            if( opdef->size() != 0 )
                {mlogrepo(opdef->getDocPath())(Lengine::E2042,opdef->phrase,opdef->name);fine = false;}
        } else if( opdef->name.is(VN::OPL_AS) ) {
            if( opdef->size() > 0 )
                {mlogrepo(opdef->getDocPath())(Lengine::E2046,opdef->phrase);fine = false;}
            if( opdef->rproto->dtype->is(typeuc::VoidType) or opdef->rproto->dtype->is(typeuc::UnknownType) or opdef->rproto->dtype->sub == opdef->getScope() ) {
                {mlogrepo(opdef->getDocPath())(Lengine::E2047,opdef->rproto->phrase);fine = false;}
            }
        }
    }
    if( !fine ) return false;

    auto tss = generateGlobalUniqueName(opdef->getScope(),None);
    if( mnamedT.count(tss) == 0 ) mnamedT[tss] = StructType::create(mctx,tss);
    pts.push_back(mnamedT[tss]->getPointerTo());
    
    for( auto par : *opdef ) {
        auto t = generateTypeUsageAsParameter(par->proto);
        if( !t ) fine = false;
        pts.push_back( t );
    }
    
    auto ft = FunctionType::get(rtp,pts,false);
    auto fs = generateGlobalUniqueName(($node)opdef);
    mnamedT[fs] = ft;
    return ft != nullptr;
}

bool Sengine::performImplementationSemanticValidation( $MethodImpl method ) {
    
    auto def = requestPrototype(($implementation)method);
    if( !def ) {
        mlogrepo(method->getDocPath())(Lengine::E2029,method->name);
        for( auto par : *method ) {
            if( par->proto->dtype->is(typeuc::UnsolvableType) )
                mlogrepo(method->getDocPath())(Lengine::E2040,par->proto->dtype->phrase);
        }
        return false;
    }
    auto fs = generateGlobalUniqueName(($node)method);
    auto ft = (FunctionType*)mnamedT[fs];
    auto fp = mcurmod->getFunction(fs);
    if( !fp ) fp = Function::Create(ft,GlobalValue::ExternalLinkage,fs,mcurmod.get());


    auto ebb = BasicBlock::Create(mctx,"",fp);
    auto builder = IRBuilder<>(ebb);
    auto arg = fp->arg_begin();

    for( auto par : *method ) {
        arg += 1;
        arg->setName( (string)par->name );
        if( par->proto->elmt == OBJ and par->proto->dtype->is(typeuc::CompositeType) ) {
            mlocalV[par] = arg;
        } else {
            auto addr = builder.CreateAlloca(arg->getType());
            builder.CreateStore(arg,addr);
            mlocalV[par] = addr;
        }
    }

    flag_terminate = false;
    if( !performImplementationSemanticValidation( method->body, builder ) ) return false;

    if( !flag_terminate ) {
        token token = method->body->impls.size()?
            method->body->impls[-1]->phrase:
            method->body->phrase;
        mlogrepo(method->getDocPath())(Lengine::E2056,token,method->name);
        return false;
    }

    return true;
}

bool Sengine::performImplementationSemanticValidation( $implementation impl, IRBuilder<>& builder, Position pos ) {
    if( flag_terminate ){
        mlogrepo(impl->getDocPath())(Lengine::E2033,impl->phrase);
        return false;
    } 
    bool ret = true;
    if( auto ct = ($FlowCtrlImpl)impl; ct ) ret = performImplementationSemanticValidation( ct, builder);
    else if( auto ex = ($ExpressionImpl)impl; ex ) ret = performImplementationSemanticValidation( ex, builder, pos);
    else if( auto el = ($ConstructImpl)impl; el ) ret = performImplementationSemanticValidation( el, builder);
    else if( auto br = ($BranchImpl)impl; br ) ret = performImplementationSemanticValidation( br, builder);
    else if( auto lp = ($LoopImpl)impl; lp ) ret = performImplementationSemanticValidation( lp, builder);
    else if( auto bk = ($InsBlockImpl)impl; bk ) ret = performImplementationSemanticValidation( bk, builder );
    return ret;
}

bool Sengine::performImplementationSemanticValidation( $InsBlockImpl  impl ,IRBuilder<>& builder ){
    if( flag_terminate ){
        mlogrepo(impl->getDocPath())(Lengine::E2033,impl->phrase);
        return false;
    } 
    bool ret = true;

    for( auto imp : impl->impls )
        ret = performImplementationSemanticValidation( imp, builder ) and ret;

    return ret;
}

bool Sengine::performImplementationSemanticValidation( $FlowCtrlImpl impl, llvm::IRBuilder<>& builder ) {
    if( flag_terminate ){
        mlogrepo(impl->getDocPath())(Lengine::E2033,impl->phrase);
        return false;
    } 
    auto proto = requestPrototype(($implementation)impl);
    switch( impl->action ) {
        case RETURN: {
            if( impl->expr ) {
                auto v = performImplementationSemanticValidation( impl->expr, builder, AsRetVal ); if( !v ) return false;
                //[TODO] : generateBackendIR for <leave> method

                if( auto rv = insureEquivalent(proto->rproto, v, builder, Returning ); rv ) {
                    builder.CreateRet(rv->asobject(builder));
                    flag_terminate = true;
                } else {
                    mlogrepo(impl->getDocPath())(Lengine::E2054,impl->expr->phrase);
                    return false;
                }
            } else if( !proto->rproto->dtype->is(typeuc::VoidType) ) {
                mlogrepo(impl->getDocPath())(Lengine::E2057,impl->phrase,proto->rproto->dtype->phrase);
            } else {
                builder.CreateRetVoid();
                flag_terminate = true;
            }
            return true;
        } break;
        //[TODO] : case BREAK: generateBackendIR for <leave> block
        default:
            return true;
    }
}

$imm Sengine::performImplementationSemanticValidation( $ExpressionImpl impl, IRBuilder<>& builder, Position pos ) {
    if( !impl or flag_terminate ) return nullptr;
    switch( impl->type ) {
        default: return nullptr;
        case ExpressionImpl::NAMEUSAGE: if( auto s = processNameusageExpression( impl, builder, pos ); s.size() ) return s[0]; else return nullptr;
        case ExpressionImpl::MEMBER: if( auto s = processMemberExpression( impl, builder, pos ); s.size() ) return s[0]; else return nullptr;
        case ExpressionImpl::ASSIGN: return processAssignExpression( impl, builder, pos );
        case ExpressionImpl::VALUE: return processValueExpression( impl, builder, pos );
        case ExpressionImpl::SUFFIX: return processCalcExpression( impl, builder, pos );
        case ExpressionImpl::PREFIX: return processCalcExpression( impl, builder, pos );
        case ExpressionImpl::INFIX: return processCalcExpression( impl, builder, pos );
        case ExpressionImpl::CALL: return processCallExpression( impl, builder, pos );
        case ExpressionImpl::CONVERT: return processConvertExpression( impl, builder, pos );
    }
}

imms Sengine::processNameusageExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos ) {
    if( impl->type != ExpressionImpl::NAMEUSAGE ) return {};

    imms ret;
    
    auto eve = request( impl->name, NormalClass );
    if( eve.size() == 0 ) {
        mlogrepo(impl->getDocPath())(Lengine::E2004,impl->name[-1].name);
        return {};
    }
    for( auto e : eve ) {
        if( auto ce = ($ClassDef)e; ce ) {
            auto symbolE = generateGlobalUniqueName(($node)e,Entity);
            auto symbolT = generateGlobalUniqueName(($node)e,Meta);
            auto gt = (StructType*)mnamedT[symbolT];
            auto gv = mcurmod->getOrInsertGlobal(symbolE, gt);
            ret << imm::entity( gv, ce );
        } else if( auto cm = ($ConstructImpl)e; cm ) {
            if( mlocalV.count(cm) ) ret << imm::address( mlocalV[cm], cm->proto );
        } else if( auto ad = ($AttrDef)e; ad ) {
            auto sc = ($ClassDef)ad->getScope();
            auto symbolT = generateGlobalUniqueName(($node)sc,ad->meta?Meta:None);
            Type* stt = mnamedT[symbolT];
            Value* gep = nullptr;
            int index = 0;
            if( ad->meta ) {
                auto symbolE = generateGlobalUniqueName(($node)sc,Entity);
                auto symbolT = generateGlobalUniqueName(($node)sc,Meta);
                gep = mcurmod->getOrInsertGlobal(symbolE,mnamedT[symbolT]);
                while( sc->metadefs[index] != ad ) index++;  //[ATTENTION] : 危险
            } else {
                auto mdef = requestPrototype(($implementation)impl);
                gep = requestThis(($implementation)impl);
                while( sc->instdefs[index] != ad ) index++;   //[ATTENTION] : 危险
                /** if( mdef and !mdef->meta and gep ) continue; */
            }
            gep = builder.CreateStructGEP( stt, gep, index );
            if( gep ) ret << imm::address(gep,ad->proto);
        } else if( auto mt = ($MethodDef)e; mt ) {
            auto fs = generateGlobalUniqueName(($node)mt);
            auto gv = mcurmod->getFunction(fs);
            if( !gv ) gv = Function::Create((FunctionType*)mnamedT[fs],GlobalValue::ExternalLinkage,fs,mcurmod.get());
            ret << imm::function( gv, mt );
        }
    }

    if( ret.size() == 0 ) return {}; //保证返回结果不是空集,因为驱动函数要直接使用结果
    return ret;
}

imms Sengine::processMemberExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos ) {
    if( impl->type != ExpressionImpl::MEMBER ) return {};

    imms ret;
    auto host = performImplementationSemanticValidation( impl->sub[0], builder, BeforeMember );
    if( !host ) return {};
    Value* vhost = host->asaddress(builder);
    if( !vhost ) {
        mlogrepo(impl->getDocPath())(Lengine::E1010,impl->name[-1].name);  
        return {};
    }
    definitions search;

    if( auto def = host->metacls(); def ) {
        search = def->metadefs;
    } else if( auto proto = host->eproto(); proto ) {
        auto eve = request(proto->dtype->name,NormalClass);
        if( eve.size() != 1 ) {
            mlogrepo(impl->getDocPath())(Lengine::E2004,impl->name[-1].name);  
            return {};
        }
        def = ($ClassDef)eve[0];
        if( def ) search = def->instdefs;
    }
    for( auto d : search )
        if( (string)d->name == (string)impl->mean ) {
            if( auto ad = ($AttrDef)d; ad ) {
                int index = 0; while( search[index] != d ) index ++;
                auto gep = builder.CreateStructGEP( mnamedT[generateGlobalUniqueName(($node)d,Meta)], vhost, index );
                ret << imm::address(gep,ad->proto,host);
            } else if( auto md = ($MethodDef)d; md ) {
                auto fs = generateGlobalUniqueName(($node)md);
                auto fp = mcurmod->getFunction(fs);
                if( !fp ) fp = Function::Create( (FunctionType*)mnamedT[fs], GlobalValue::ExternalLinkage, fs, mcurmod.get() );
                ret << imm::function(fp,md,host);
            }
        }

    if( ret.size() == 0 ) mlogrepo(impl->getDocPath())(Lengine::E2004,impl->mean);
    return ret;
}

$imm Sengine::processAssignExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos ) {
    if( impl->type != ExpressionImpl::ASSIGN ) return nullptr;
    Value* rv;
    $eproto proto;

    auto left = performImplementationSemanticValidation(impl->sub[0],builder, LeftOfAssign);
    auto right = performImplementationSemanticValidation(impl->sub[1],builder, AsOperand);
    if( !left or !right ) return nullptr;

    if( !left->asaddress(builder) ) {
        mlogrepo(impl->getDocPath())(Lengine::E2053,impl->sub[0]->phrase);
        return nullptr;
    }

    right = insureEquivalent(left->eproto(),right,builder,Assigning);
    if( !right ) {
        mlogrepo(impl->getDocPath())(Lengine::E2054,impl->sub[1]->phrase);
        return nullptr;
    }
    
    switch( impl->mean.id ) {
        default: break;
        case VT::ASSIGN: builder.CreateStore(right->asobject(builder),left->asaddress(builder)); break;
        case VT::ASSIGN_PLUS:rv = builder.CreateAdd(left->asobject(builder),right->asobject(builder));builder.CreateStore(rv, left->asaddress(builder)); break;
        case VT::ASSIGN_MINUS:rv = builder.CreateSub(left->asobject(builder),right->asobject(builder));builder.CreateStore(rv, left->asaddress(builder)); break;
        case VT::ASSIGN_MUL:rv = builder.CreateMul(left->asobject(builder),right->asobject(builder));builder.CreateStore(rv, left->asaddress(builder)); break;
        case VT::ASSIGN_DIV:rv = builder.CreateSDiv(left->asobject(builder),right->asobject(builder));builder.CreateStore(rv, left->asaddress(builder)); break;
        case VT::ASSIGN_MOL:rv = builder.CreateSRem(left->asobject(builder),right->asobject(builder));builder.CreateStore(rv, left->asaddress(builder)); break;
        case VT::ASSIGN_SHL:rv = builder.CreateShl(left->asobject(builder),right->asobject(builder));builder.CreateStore(rv, left->asaddress(builder)); break;
        case VT::ASSIGN_SHR:rv = builder.CreateAShr(left->asobject(builder),right->asobject(builder));builder.CreateStore(rv, left->asaddress(builder)); break;
        case VT::ASSIGN_bAND:rv = builder.CreateAnd(left->asobject(builder),right->asobject(builder));builder.CreateStore(rv, left->asaddress(builder)); break;
        case VT::ASSIGN_bOR:rv = builder.CreateOr(left->asobject(builder),right->asobject(builder));builder.CreateStore(rv, left->asaddress(builder)); break;
        case VT::ASSIGN_bXOR:rv = builder.CreateXor(left->asobject(builder),right->asobject(builder));builder.CreateStore(rv, left->asaddress(builder)); break;
    }

    return left;
}

$imm Sengine::processValueExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos ) {
    if( impl->type != ExpressionImpl::VALUE ) return nullptr;
    switch( impl->mean.id ) {
        case VT::iINTEGERn: { // 限制10进制数字只能32位
            auto i = std::stoll(impl->mean);
            if( i > INT32_MAX or i < INT32_MIN ) return nullptr;
            return imm::object( 
                builder.getInt32(i),
                eproto::MakeUp(
                    impl->getScope(),
                    OBJ,
                    typeuc::GetBasicDataType(VT::INT32)
                )
            );
        } break;
        case VT::iINTEGERb: {
            auto i = std::stoll(string(impl->mean.tx.begin()+2,impl->mean.tx.end()),nullptr,2);
            $typeuc type = nullptr;
            Value* value = nullptr;
            if( auto len = impl->mean.tx.length() - 2;
                len <= 8 ) {type = typeuc::GetBasicDataType(typeuc::Uint8);value = builder.getInt8(i);}
                else if( len <= 16 ) {type = typeuc::GetBasicDataType(typeuc::Uint16);value = builder.getInt16(i);}
                else if( len <= 32 ) {type = typeuc::GetBasicDataType(typeuc::Uint32);value = builder.getInt32(i);}
                else if( len <= 64 ) {type = typeuc::GetBasicDataType(typeuc::Uint64);value = builder.getInt64(i);}
                else return nullptr;
            auto proto = eproto::MakeUp(impl->getScope(),OBJ,type);
            return imm::object( value, proto );
        } break;
        case VT::iINTEGERh: {
            auto i = std::stoll(string(impl->mean.tx.begin()+2,impl->mean.tx.end()),nullptr,16);
            $typeuc type = nullptr;
            Value* value = nullptr;
            if( auto len = impl->mean.tx.length() - 2;
                len <= 2 ) {type = typeuc::GetBasicDataType(typeuc::Uint8);value = builder.getInt8(i);}
                else if( len <= 4 ) {type = typeuc::GetBasicDataType(typeuc::Uint16);value = builder.getInt16(i);}
                else if( len <= 8 ) {type = typeuc::GetBasicDataType(typeuc::Uint32);value = builder.getInt32(i);}
                else if( len <= 16 ) {type = typeuc::GetBasicDataType(typeuc::Uint64);value = builder.getInt64(i);}
                else return nullptr;
            auto proto = eproto::MakeUp(impl->getScope(),OBJ,type);
            return imm::object( value, proto );
        } break;
        case VT::iINTEGERo: {
            auto i = std::stoll(string(impl->mean.tx.begin()+2,impl->mean.tx.end()),nullptr,8);
            $typeuc type = nullptr;
            Value* value = nullptr;
            if( auto len = impl->mean.tx.length() - 2;
                len <= 3 ) {type = typeuc::GetBasicDataType(typeuc::Uint8);value = builder.getInt8(i);}
                else if( len <= 6 ) {type = typeuc::GetBasicDataType(typeuc::Uint16);value = builder.getInt16(i);}
                else if( len <= 12 ) {type = typeuc::GetBasicDataType(typeuc::Uint32);value = builder.getInt32(i);}
                else if( len <= 124 ) {type = typeuc::GetBasicDataType(typeuc::Uint64);value = builder.getInt64(i);}
                else return nullptr;
            auto proto = eproto::MakeUp(impl->getScope(),OBJ,type);
            return imm::object( value, proto );
        } break;
        case VT::iFLOAT: {
            auto i = std::stod(impl->mean.tx);
            auto value = ConstantFP::get(builder.getContext(), APFloat(i));
            $typeuc type = typeuc::GetBasicDataType(value->getType()->isDoubleTy()?typeuc::Float64:typeuc::Float32);
            auto proto = eproto::MakeUp(impl->getScope(),OBJ,type);
            return imm::object( value, proto );
        }
        case VT::iSTRING: {
            return imm::object(
                builder.CreateGlobalStringPtr( Xengine::extractText(impl->mean) ),
                eproto::MakeUp(
                    impl->getScope(),
                    PTR,
                    typeuc::GetPointerType(typeuc::GetBasicDataType(VT::INT8))
                )
            );
        }
        case VT::iNULL: {
            return imm::object(
                ConstantPointerNull::get(builder.getInt8PtrTy()),
                eproto::MakeUp(
                    impl->getScope(),
                    PTR,
                    typeuc::GetPointerType()
                )
            );
        }
        case VT::iTHIS: {
            return imm::object(
                requestThis(($implementation)impl),
                eproto::MakeUp(
                    impl->getScope(),
                    REF,
                    typeuc::GetCompositeType(requestThisClass(($implementation)impl))
                )
            );
        }
        case VT::iTRUE: {
            return imm::object(
                builder.getTrue(),
                eproto::MakeUp(
                    impl->getScope(),
                    OBJ,
                    typeuc::GetBasicDataType(VT::BOOL)
                )
            );
        }
        case VT::iFALSE: {
            return imm::object(
                builder.getFalse(),
                eproto::MakeUp(
                    impl->getScope(),
                    OBJ,
                    typeuc::GetBasicDataType(VT::BOOL)
                )
            );
        }
        default : return nullptr;
    }
}

$imm Sengine::processCallExpression( $ExpressionImpl impl, llvm::IRBuilder<>& builder, Position pos ) {
    if( impl->type != ExpressionImpl::CALL ) return nullptr;

    imms argis;
    auto err = false;
    auto ait = impl->sub.begin();
    auto fps = (*ait)->type == ExpressionImpl::MEMBER?
        processMemberExpression( *(ait++), builder, AsProc ):
        processNameusageExpression( *(ait++), builder, AsProc );

    while( ait != impl->sub.end() ) {
        auto p = performImplementationSemanticValidation( *(ait++), builder, AsParam );
        if( !p ) err = true;
        else if( !p->eproto() ) {mlogrepo(impl->getDocPath())(Lengine::E2049,(*(ait-1))->phrase );err = true;}
        //else args.push_back( p->asparameter(builder) );
        argis << p;
    }
    if( err ) return nullptr;

    $imm fp = nullptr;
    for( auto tfp : fps ) {
        if( !tfp->asfunction() ) continue;
        auto mproto = tfp->prototype();
        if( !mproto or mproto->size() != argis.size() ) continue;
        bool continu = false;
        auto it = argis.begin();
        for( auto par : *tfp->prototype() ) {
            if( !checkEquivalent( par->proto->dtype, (*it)->eproto()->dtype ) ) continu = true;
            it++;
        }
        if( continu ) continue;
        if( fp ) {
            mlogrepo(impl->getDocPath())(Lengine::E2050,impl->phrase)
                (fp->prototype()->getDocPath(),Lengine::E2010,fp->prototype()->name)
                (mproto->getDocPath(),Lengine::E2010,mproto->name);
            err = true;
        } else {
            fp = tfp;
        }
    }

    std::vector<Value*> args;
    if( !fp or err ) {
        if( !fp ) mlogrepo(impl->getDocPath())(Lengine::E2051,impl->sub[0]->phrase);
        return nullptr;
    }

    auto pi = fp->prototype()->begin();
    for( auto& ai : argis ) {
        ai = insureEquivalent( (*pi++)->proto, ai, builder, Passing );
        args.push_back( ai->asparameter(builder) );
    }
    
    if( impl->sub[0]->type == ExpressionImpl::NAMEUSAGE ) {
        args.insert(args.begin(),requestThis(($implementation)impl));
    } else if( impl->sub[0]->type == ExpressionImpl::MEMBER ) {
        args.insert(args.begin(),fp->h->asaddress(builder));
    }

    return imm::object(builder.CreateCall(fp->asfunction(),args),fp->prototype()->rproto);  //[FIXME]object存疑
}

$imm Sengine::processCalcExpression( $ExpressionImpl impl, llvm::IRBuilder<>& builder, Position pos ) {

    switch( impl->type ) {
        case ExpressionImpl::INFIX: {
            bool fine = true;
            auto left = performImplementationSemanticValidation(impl->sub[0],builder, AsOperand);
            auto right = performImplementationSemanticValidation(impl->sub[1],builder, AsOperand);
            if( !left or !right ) return nullptr;

            auto lp = determineElementPrototype(left->eproto());
            auto rp = determineElementPrototype(right->eproto());
            if( !lp ) {mlogrepo(impl->getDocPath())(Lengine::E2055,impl->sub[0]->phrase,impl->mean);fine = false;}
            if( !rp ) {mlogrepo(impl->getDocPath())(Lengine::E2055,impl->sub[1]->phrase,impl->mean);fine = false;}

            if( lp->dtype->is(typeuc::BasicType) and rp->dtype->is(typeuc::BasicType) ) {
                Value* rv;
                $eproto proto;
                auto accl = getAccuracy(lp->dtype);
                auto accr = getAccuracy(rp->dtype);
                if( accl > accr ) {
                    proto = lp;
                    right = insureEquivalent(lp, right, builder, Calculating );
                } else if( accl < accr ) {
                    proto = rp;
                    left = insureEquivalent(rp, left, builder, Calculating );
                } else if( !checkEquivalent(lp->dtype,rp->dtype) ) {
                    mlogrepo(impl->getDocPath())(Lengine::E2054,impl->phrase);
                    return nullptr;
                } else {
                    proto = lp;
                }
                switch( impl->mean.id ) {
                    default: {
                        if( proto->dtype->is(typeuc::FloatPointType) ) {
                            mlogrepo(impl->getDocPath())(Lengine::E2055,impl->sub[0]->phrase,impl->mean);
                            mlogrepo(impl->getDocPath())(Lengine::E2055,impl->sub[1]->phrase,impl->mean);
                            return nullptr;
                        }
                        switch( impl->mean.id ) {
                            case VT::MOL:   rv = builder.CreateSRem(left->asobject(builder),right->asobject(builder)); break;
                            case VT::BITAND:rv = builder.CreateAnd(left->asobject(builder),right->asobject(builder)); break;
                            case VT::BITOR: rv = builder.CreateOr(left->asobject(builder),right->asobject(builder)); break;
                            case VT::BITXOR:rv = builder.CreateXor(left->asobject(builder),right->asobject(builder)); break;
                            case VT::SHL: rv = builder.CreateShl(left->asobject(builder),right->asobject(builder)); break;
                            case VT::SHR: rv = builder.CreateAShr(left->asobject(builder),right->asobject(builder)); break;
                            default: return nullptr;
                        }
                    } break;
                    case VT::GT: rv = builder.CreateICmpSGE(left->asobject(builder),right->asobject(builder));proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;
                    case VT::LT: rv = builder.CreateICmpSLT(left->asobject(builder),right->asobject(builder));proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;
                    case VT::LE: rv = builder.CreateICmpSLT(left->asobject(builder),right->asobject(builder));proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;
                    case VT::GE: rv = builder.CreateICmpSGE(left->asobject(builder),right->asobject(builder));proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;
                    case VT::EQ: rv = builder.CreateICmpEQ(left->asobject(builder),right->asobject(builder)); proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;
                    case VT::NE: rv = builder.CreateICmpNE(left->asobject(builder),right->asobject(builder)); proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;

                    case VT::AND: rv = builder.CreateAnd(left->asobject(builder),right->asobject(builder)); break;
                    case VT::OR:  rv = builder.CreateOr(left->asobject(builder),right->asobject(builder)); break;

                    case VT::PLUS:  rv = proto->dtype->is(typeuc::FloatPointType)?
                        builder.CreateFAdd(left->asobject(builder),right->asobject(builder)):
                        builder.CreateAdd(left->asobject(builder),right->asobject(builder)); break;
                    case VT::MINUS: rv = proto->dtype->is(typeuc::FloatPointType)?
                        builder.CreateFSub(left->asobject(builder),right->asobject(builder)):
                        builder.CreateSub(left->asobject(builder),right->asobject(builder)); break;
                    case VT::MUL:   rv = proto->dtype->is(typeuc::FloatPointType)?
                        builder.CreateFMul(left->asobject(builder),right->asobject(builder)):
                        builder.CreateMul(left->asobject(builder),right->asobject(builder)); break;
                    case VT::DIV:   rv = proto->dtype->is(typeuc::FloatPointType)?
                        builder.CreateFDiv(left->asobject(builder),right->asobject(builder)):
                        builder.CreateSDiv(left->asobject(builder),right->asobject(builder)); break;
                }
                return imm::object( rv, proto );
            }
        } break;
        case ExpressionImpl::SUFFIX: {
            auto operand = performImplementationSemanticValidation(impl->sub[0],builder,AsOperand);
            if( !operand ) return nullptr;
            Value* rv = operand->asobject(builder);
            switch( impl->mean.id ) {
                case VT::INCRESS: builder.CreateStore(builder.CreateAdd(rv,builder.getInt32(1)),operand->asaddress(builder));break;
                case VT::DECRESS: builder.CreateStore(builder.CreateSub(rv,builder.getInt32(1)),operand->asaddress(builder));break;
                case VT::OPENL: {
                    auto ind = performImplementationSemanticValidation(impl->sub[1],builder,AsParam);
                    if( !ind ) return nullptr;
                    rv = builder.CreateGEP(rv,ind->asobject(builder));
                    rv = builder.CreateLoad(rv);
                    auto proto = operand->eproto();
                    proto->dtype = proto->dtype->sub;
                    if( !proto->dtype->is(typeuc::PointerType) ) proto->elmt = OBJ;
                    return imm::object(rv,proto);
                }
            }
            return imm::object(rv,operand->eproto());
        } break;
        case ExpressionImpl::PREFIX: {
            auto right = performImplementationSemanticValidation(impl->sub[0],builder,AsOperand);
            if( !right ) return nullptr;
            Value* rv = nullptr;
            switch( impl->mean.id ) {
                default: break;
                case VT::BITAND: {
                    auto proto = right->eproto();
                    proto->dtype = proto->dtype->getPointerTo();
                    proto->elmt = PTR;
                    return imm::object(right->asaddress(builder),proto);
                }
                case VT::MUL: {
                    auto proto = right->eproto();
                    proto->dtype = proto->dtype->sub;
                    if( !proto->dtype->is(typeuc::PointerType) ) proto->elmt = OBJ;
                    return imm::object(builder.CreateLoad(right->asobject(builder)),proto);
                }
                case VT::NOT: {
                    auto proto = right->eproto();
                    rv = builder.CreateNot(right->asobject(builder));
                    return imm::object(rv,proto);
                }
                case VT::INCRESS: {
                    builder.CreateStore(builder.CreateAdd(right->asobject(builder),builder.getInt32(1)), right->asaddress(builder) );
                    return right;
                }
                case VT::DECRESS: {
                    builder.CreateStore(builder.CreateSub(right->asobject(builder),builder.getInt32(1)), right->asaddress(builder) );
                    return right;
                }
            }
        }break;
        default : return nullptr;
    }
    return nullptr;
}

$imm Sengine::processConvertExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos ) {
    if( !impl ) return nullptr;
    auto value = performImplementationSemanticValidation(impl->sub[0],builder,Position::AsOperand);
    if( !value ) return nullptr;
    auto proto = value->eproto(); if( !determineElementPrototype(proto) ){mlogrepo(impl->getDocPath())(Lengine::E2052,impl->sub[0]->phrase);return nullptr;}
    auto droto = determineElementPrototype(impl->target); if( !droto ) return nullptr;
    auto dst = droto->dtype;
    auto src = proto->dtype;
    if( checkEquivalent(dst,src) ) return value;

    return doConvert( dst, value, builder );
}

bool Sengine::performImplementationSemanticValidation( $ConstructImpl impl, llvm::IRBuilder<>& builder ) {
    if( flag_terminate ){
        mlogrepo(impl->getDocPath())(Lengine::E2033,impl->phrase);
        return false;
    } 
    $imm inm = nullptr;
    Value* inv = nullptr;
    bool fine = true;
    if( impl->init ) {
        inm = performImplementationSemanticValidation( impl->init, builder, AsInit );
        if( !impl->proto->dtype->is(typeuc::UnknownType) ) {
            inm = insureEquivalent(impl->proto,inm,builder,Constructing);
        }
        if( inm ) inv = inm->asobject(builder);
        else {mlogrepo(impl->getDocPath())(Lengine::E2054,impl->init->phrase);fine = false;}
    }

    if( impl->proto->dtype->is(typeuc::UnknownType) ) {
        if( inm ) impl->proto->dtype = inm->eproto()->dtype;
        else return false;
    }
    auto tp = generateTypeUsageAsAttribute(impl->proto); if( !tp ) return false;
    Value* addr = builder.CreateAlloca(tp, nullptr, (string)impl->name);
    if( inv and fine ) builder.CreateStore(inv,addr);

    mlocalV[impl] = addr;
    return addr != nullptr and fine;
}

bool Sengine::performImplementationSemanticValidation( $BranchImpl impl, IRBuilder<>& builder ) {
    if( flag_terminate ){
        mlogrepo(impl->getDocPath())(Lengine::E2033,impl->phrase);
        return false;
    } 
    auto bb2 = BasicBlock::Create(mctx,"",builder.GetInsertBlock()->getParent());
    auto bb3 = BasicBlock::Create(mctx,"",builder.GetInsertBlock()->getParent());
    auto bb4 = BasicBlock::Create(mctx,"",builder.GetInsertBlock()->getParent());
    auto bd = IRBuilder<>(mctx);
    bool ret = false;
    auto cond = performImplementationSemanticValidation( impl->exp, builder, AsOperand );
    if( !cond ) return false;
    builder.CreateCondBr(cond->asobject(builder),bb2,bb3);
    builder.SetInsertPoint(bb4);

    bd.SetInsertPoint(bb2);
    ret = performImplementationSemanticValidation( impl->first ,bd);
    if( ret == false ) return ret;
    if( flag_terminate ) flag_terminate = false;
    else bd.CreateBr(bb4);

    bd.SetInsertPoint(bb3);
    ret = performImplementationSemanticValidation( impl->secnd ,bd);
    if( ret == false ) return ret;
    if( flag_terminate ) flag_terminate = false;
    else bd.CreateBr(bb4);

    return ret;
}

bool Sengine::performImplementationSemanticValidation( $LoopImpl impl ,IRBuilder<>& builder) {
    if( flag_terminate ){
        mlogrepo(impl->getDocPath())(Lengine::E2033,impl->phrase);
        return false;
    } 
    auto bb1 = BasicBlock::Create(mctx,"",builder.GetInsertBlock()->getParent());
    auto bb3 = BasicBlock::Create(mctx,"",builder.GetInsertBlock()->getParent());
    auto bd = IRBuilder<>(mctx);

    builder.CreateBr(bb1);
    builder.SetInsertPoint(bb3);

    bd.SetInsertPoint(bb1);
    if( impl->cond ){
        auto cond = performImplementationSemanticValidation( impl->cond, bd, AsOperand );
        auto bb2 = BasicBlock::Create(mctx,"",builder.GetInsertBlock()->getParent());
        bd.CreateCondBr(cond->asobject(builder),bb2,bb3);
        bd.SetInsertPoint(bb2);
    }

    bool fine = performImplementationSemanticValidation( impl->imp,bd );
    if( flag_terminate ) flag_terminate = false;
    else bd.CreateBr(bb1);
   
    return fine;
}

Type* Sengine::performDefinitionSemanticValidation( $AttrDef attr ) {
    /** 语法阶段好像对此有所检查,记不清了 if( attr->proto->elmt == REL ) { ... } */
    return generateTypeUsageAsAttribute(attr->proto);
}

Type* Sengine::generateTypeUsageAsParameter( $eproto proto ) {
    if( !determineElementPrototype(proto) ) return nullptr;

    Type* ty = generateTypeUsage(proto->dtype);
    if( !ty ) return nullptr;
    if( proto->elmt == REL or proto->elmt == REF or proto->elmt == OBJ and ty->isStructTy() ) {
        ty = ty->getPointerTo();
    }

    return ty;
}

Type* Sengine::generateTypeUsageAsAttribute( $eproto proto ) {
    if( !determineElementPrototype(proto) ) return nullptr;

    Type* ty = generateTypeUsage(proto->dtype);
    if( !ty ) return nullptr;
    if( proto->elmt == REL or proto->elmt == REF )
        ty = ty->getPointerTo();

    return ty;
}

Type* Sengine::generateTypeUsageAsReturnValue( $eproto proto, vector<Type*>& pts ) {
    if( !proto ) return Type::getVoidTy(mctx);
    if( !determineElementPrototype(proto) ) return nullptr;
    Type* ty = generateTypeUsage(proto->dtype);
    if( !ty ) return nullptr;
    if( ty->isStructTy() ) {
        pts.insert(pts.begin(),ty->getPointerTo());
        return Type::getInt64Ty(mctx);
    }
    else return ty;
}

Type* Sengine::generateTypeUsage( $typeuc type ) {
    if( !type ) return nullptr;
    
    if( type->is(typeuc::BasicType) ) {
        switch( type->id ) {
            default : return nullptr;
            case typeuc::BooleanType                :  return Type::getInt1Ty(mctx);    break;
            case typeuc::Int8:  case typeuc::Uint8  :  return Type::getInt8Ty(mctx);    break;
            case typeuc::Int16: case typeuc::Uint16 :  return Type::getInt16Ty(mctx);   break;
            case typeuc::Int32: case typeuc::Uint32 :  return Type::getInt32Ty(mctx);   break;
            case typeuc::Int64: case typeuc::Uint64 :  return Type::getInt64Ty(mctx);   break;
            case typeuc::Float32                    :  return Type::getFloatTy(mctx);   break;
            case typeuc::Float64                    :  return Type::getDoubleTy(mctx);  break;
        }
    } else if( type->is(typeuc::UndeterminedType) ) {
        return generateTypeUsage(determineDataType(type));
    } else if( type->is(typeuc::CompositeType) ) {
        auto symbol = generateGlobalUniqueName(($node)type->sub);
        if( !mnamedT.count(symbol) ) mnamedT[symbol] = StructType::create(mctx,symbol);
        return mnamedT[symbol];
    } else if( type->is(typeuc::PointerType) ) {
        auto sub = generateTypeUsage(($typeuc)type->sub);
        if( sub ) return sub->getPointerTo();
    }

    return nullptr;
}

std::string Sengine::generateGlobalUniqueName( $node n, Decorate dec ) {
    string prefix;
    string suffix;
    string domain;

    auto nameProc = []( token name ) -> string {
        switch( name.in ) {
            case VN::OPL_SCTOR: return "sctor";
            case VN::OPL_LCTOR: return "lctor";
            case VN::OPL_CCTOR: return "cctor";
            case VN::OPL_MCTOR: return "mctor";
            case VN::OPL_DTOR: return "dtor";
            case VN::OPL_AS: return "as";
            case VN::OPL_MEMBER: return "member";
            case VN::OPL_WHERE: return "where";
            case VN::OPL_MOVE: return "move";
            case VN::OPL_NEGATIVE: return "negative";
            case VN::OPL_BITREV: return "bitrev";
            case VN::OPL_INCREASE: return "increase";
            case VN::OPL_DECREASE: return "decrease";
            case VN::OPL_INDEX: return "index";
            case VN::OPL_ADD: return "add";
            case VN::OPL_SUB: return "sub";
            case VN::OPL_MUL: return "mul";
            case VN::OPL_DIV: return "div";
            case VN::OPL_MOL: return "mol";
            case VN::OPL_BITAND: return "bitand";
            case VN::OPL_BITOR: return "bitor";
            case VN::OPL_BITXOR: return "bitxor";
            case VN::OPL_SHL: return "shl";
            case VN::OPL_SHR: return "shr";
            case VN::OPL_LT: return "lt";
            case VN::OPL_GT: return "gt";
            case VN::OPL_LE: return "le";
            case VN::OPL_GE: return "ge";
            case VN::OPL_EQ: return "eq";
            case VN::OPL_NE: return "ne";
            case VN::OPL_AND: return "and";
            case VN::OPL_OR: return "or";
            case VN::OPL_XOR: return "xor";
            case VN::OPL_NOT: return "not";
            case VN::OPL_ASSIGN: return "assign";
            case VN::OPL_ASSIGN_ADD: return "assign.add";
            case VN::OPL_ASSIGN_SUB: return "assign.sub";
            case VN::OPL_ASSIGN_MUL: return "assign.mul";
            case VN::OPL_ASSIGN_DIV: return "assign.div";
            case VN::OPL_ASSIGN_MOL: return "assign.mol";
            case VN::OPL_ASSIGN_SHL: return "assign.shl";
            case VN::OPL_ASSIGN_SHR: return "assign.shr";
            case VN::OPL_ASSIGN_BITAND: return "assign.bitand";
            case VN::OPL_ASSIGN_BITOR: return "assign.bitor";
            case VN::OPL_ASSIGN_BITXOR: return "assin.bitxor";
            default: return (string)name;
        }
    };

    if( auto list = dynamic_cast<morpheme::plist*>((node*)n); list ) for( auto pd : *list ) {
        suffix += ".";
        if( pd->proto->cons ) suffix += "C";
        switch( pd->proto->elmt ) {
            case OBJ : suffix += "V";break;
            case PTR : suffix += "P";break;
            case REF : suffix += "R";break;
            case REL : suffix += "L";break;
            case UDF : suffix += "X";break;
        }

        auto dtype = pd->proto->dtype;
        
        if( dtype->is(typeuc::PointerType) ) {
            int mask = 0;
            int i = 0;
            while( dtype->is(typeuc::PointerType) ) {
                if( dtype->is(typeuc::ConstraintedPointerType) ) mask |= 1 << i++;
                dtype = dtype->sub;
            }
            suffix += to_string(mask);
        }

        switch( dtype->id ) {
            case typeuc::ThisClassType: suffix += "T";break;
            case typeuc::UnknownType:   suffix += "U";break;
            case typeuc::BooleanType:   suffix += "b";break;
            case typeuc::Int8:          suffix += "i8";break;
            case typeuc::Uint8:         suffix += "u8";break;
            case typeuc::Int16 :        suffix += "i16";break;
            case typeuc::Uint16:        suffix += "u16";break;
            case typeuc::Int32 :        suffix += "i32";break;
            case typeuc::Uint32:        suffix += "u32";break;
            case typeuc::Int64 :        suffix += "i64";break;
            case typeuc::Uint64:        suffix += "u64";break;
            case typeuc::Float32:       suffix += "f32";break;
            case typeuc::Float64:       suffix += "f64";break;
            case typeuc::NamedType: case typeuc::CompositeType: {
                for( auto i = 0; i < pd->proto->dtype->name.size(); i++ )
                    suffix += "." + (string)pd->proto->dtype->name[i].name;
            }
        }
    }

    if( auto impl = ($MethodImpl)n; impl ) {
        for( int i = impl->cname.size()-1; i >= 0; i-- ) domain = "." + (string)impl->cname[i].name + domain;
        domain += "." + nameProc(impl->name);
        if( impl->constraint ) suffix = "const." + suffix;
        if( auto pro = requestPrototype(($implementation)impl);pro and pro->raw ) {
            if( pro->raw.is(VT::iSTRING) ) return Xengine::extractText(pro->raw);
            else return impl->name;
        }
    } else if( auto impl = ($OperatorImpl)n; impl ) {
        for( int i = impl->cname.size()-1; i >= 0; i-- ) domain = "." + (string)impl->cname[i].name + domain;
        domain += "." + nameProc(impl->name);
        if( impl->constraint ) suffix = "const." + suffix;
        if( impl->modifier ) suffix = (string)impl->modifier + "." + suffix;
        if( impl->subtitle ) suffix = (string)impl->subtitle + "." + suffix;
    } else if( auto def = ($MethodDef)n; def ) {
        if( def->raw ) {
            if( def->raw.is(VT::iSTRING) ) return Xengine::extractText(def->raw);
            else return def->name;
        }
        if( def->constraint ) suffix = "const." + suffix;
    } else if( auto def = ($OperatorDef)n; def ) {
        if( def->constraint ) suffix = "const." + suffix;
        if( def->modifier ) suffix = (string)def->modifier + "." + suffix;
        if( def->subtitle ) suffix = (string)def->subtitle + "." + suffix;
    }

    for( auto def = ($definition)n; def; def = def->getScope() ) domain = "." + nameProc(def->name) + domain;
    
    if( n->is(METHODDEF) ) prefix += "method";
    else if( n->is(OPERATORDEF) ) prefix += "operator";
    else if( n->is(METHODIMPL) ) prefix += "method";
    else if( n->is(OPERATORIMPL) ) prefix += "operator";
    else if( n->is(DEFINITION) ) prefix += "class";

    switch( dec ) {
        case None:break;
        case Meta: suffix += ".meta"; break;
        case Entity: suffix += ".entity"; break;
    }

    return prefix + domain + suffix;
}

everything Sengine::request( const nameuc& name, Len len, $scope sc ) {

    if( !sc ) sc = name.getScope();
    if( !sc or name.size() == 0 ) return {};
    auto sname = (string)name[0].name;
    everything res;

    function<everything(const nameuc&,$scope)> lookupInternal = [&]( const nameuc& fn, $scope fsc ) -> everything {
        auto fsname = (string)fn[0].name;
        if( auto mdef = ($module)fsc; mdef ) {
            for( auto in : mdef->internal + mdef->external ) if( (string)in->name == fsname ) {
                if( fn.size() == 1 ) res << (anything)in;
                else return lookupInternal( fn%1, in );
            }
        } else if( auto cdef = ($ClassDef)fsc; cdef ) {
            for( auto in : cdef->internal ) if( (string)in->name == fsname ) {
                if( fn.size() == 1 ) res << (anything)in;
                else return lookupInternal( fn%1, in );
            }
        } else {
            return {};
        }
        return res;
    };
    
    if( auto bimpl = ($InsBlockImpl)sc; bimpl and name.size() == 1 ) {
    /** 在执行块中搜索已经被翻译的构建语句,若没有结果则上行传递搜索 */
        for( auto im : bimpl->impls ) if( auto cimpl = ($ConstructImpl)im; cimpl ) 
            if( (string)cimpl->name == sname and mlocalV.count(cimpl) ) res << (anything)cimpl;
        if( res.size() == 0 ) {
            if( auto scsc = sc->getScope(); !scsc ) return {};
            else return request( name, len, scsc );
        }
    } else if( auto mimpl = ($MethodImpl)sc; mimpl and name.size() == 1 ) {
    /** 在方法实现中搜索参数,若没有结果,加ThisClass滤镜上行传递搜索 */
        for( auto im : *mimpl ) if( (string)im->name == sname )
            res << (anything)im;
        if( res.size() == 0 ) {
            if( auto org = requestThisClass(($implementation)mimpl); !org ) return {};
            else return request( name, ThisClass, org );
        }
    } else if( auto mdef = ($module)sc; mdef ) {
    /**
     * 在模块层
     */
        if( sname == mdef->desc->name ) {
            if( name.size() == 1 ) res << (anything)mdef;
            else return lookupInternal( name%1, mdef );
        } else {
            if( name.size() == 1 ) {
                for( auto meta : mdef->metadefs + mdef->extmeta ) if( sname == (string)meta->name ) res << (anything)meta;
            }
            for( auto idef : mdef->internal + mdef->external ) if( (string)idef->name == sname ) {
                if( name.size() == 1 ) res << (anything)idef;
                else return lookupInternal( name%1, idef );
            }
            for( auto ddef : mdef->desc->deps ) if( (string)ddef->literal() == sname ) {
                if( name.size() == 1 ) res << (anything)mrepo[ddef->dest];
                else return lookupInternal( name%1, mrepo[ddef->dest] );
            }
        }
    } else if( auto cdef = ($ClassDef)sc; cdef ) {
    /**
     * ThisClass : 搜索全部能搜索的内容,实例定义,元定义,内部定义,基类, 若查无所获,上行到作用域继续搜索
     * SuperClass : 搜索实例定义,元定义,不论是否查无所获,都上行至基类继续搜索
     * NormalClass : 搜索内部定义,若查无所获,上行至作用域继续搜索
     */
        if( name.size() == 1 ) {
            if( len == ThisClass or len == SuperClass ) {
                for( auto idef : cdef->instdefs + cdef->metadefs ) if( (string)idef->name == sname ) res << (anything)idef;
                if( res.size() == 0 ) for( const auto& super : cdef->supers ) {
                    auto eve = request(super,NormalClass);
                    if( eve.size() != 1 ) continue;
                    if( auto sdef = ($ClassDef)eve[0]; sdef ) res += request(name,SuperClass,sdef);
                }
            }
            if( res.size() == 0 and len != SuperClass ) for( auto ndef : cdef->internal ) 
                if( (string)ndef->name == sname ) res << (anything)ndef;
        } else if( len != NormalClass and name.size() == 2 ) { /** 对于基类和当前类,两层名称可能指向基类的成员定义 */
            for( const auto& super : cdef->supers ) {
                auto eve = request(super,NormalClass);
                if( eve.size() != 1 ) continue;
                if( auto sdef = ($ClassDef)eve[0]; sdef ) {
                    if( (string)super[-1].name == sname ) {
                        auto sname2 = (string)name[1].name;
                        for( auto idef : sdef->instdefs + sdef->metadefs ) if( (string)idef->name == sname2 ) res << (anything)idef;
                    } else {
                        res += request( name, SuperClass, sdef );
                    }
                }
            }
        }
        if( len != SuperClass and res.size() == 0 ) { /** 对于普通类和当前类,若尚且查无所获,查询内部定义 */
                res = lookupInternal(name, sc);
        }
        if( res.size() == 0 and len != SuperClass ) {/** 若仍然查无所获,上行继续搜索 */
            if( auto scsc = sc->getScope(); !scsc ) return {};
            else return request( name, NormalClass, scsc );
        }
    } else {
    /**
     * 其他语法结构直接上行传递搜索
     */
        if( auto scsc = sc->getScope(); !scsc ) return {};
        else return request( name, len, scsc );
    }
    return res;
}

$ClassDef Sengine::requestThisClass( $implementation impl ) {
    while( impl and !impl->is(METHODIMPL) ) impl = impl->getScope();
    if( !impl ) return nullptr;
    auto method = ($MethodImpl)impl;
    if( mmethodP.count(method) ) return mmethodP[method]->getScope();

    auto eve = request(method->cname, NormalClass);
    if( eve.size() != 1 ) return nullptr;

    return ($ClassDef)eve[0];
}

$MethodDef Sengine::requestPrototype( $implementation impl ) {
    while( impl and !impl->is(METHODIMPL) ) impl = impl->getScope();
    if( !impl ) return nullptr;
    auto met = ($MethodImpl)impl;

    if( mmethodP.count(met) ) return mmethodP[met];
    
    auto scope = requestThisClass(($implementation)met);
    if( !scope ) return nullptr;
    auto sname = (string)met->name;

    for( auto def : scope->instdefs + scope->metadefs ) if( auto mdef = ($MethodDef)def; mdef and (string)mdef->name == sname ) {
        if( mdef->size() != met->size() ) continue;
        auto arg = met->begin();
        bool found = true;
        for( auto par : *mdef ) {
            found = checkEquivalent((*arg++)->proto, par->proto ) and found;
            if( !found ) break;
        }
        if( found ) return mmethodP[met] = mdef;
    }

    return nullptr;
}

Value* Sengine::requestThis( $implementation impl ) {
    while( impl and !impl->is(METHODIMPL) ) impl = impl->getScope();
    if( !impl ) return nullptr;

    auto fp = mcurmod->getFunction(generateGlobalUniqueName(($node)impl));
    if( !fp ) return nullptr;

    return fp->arg_begin();
}

bool Sengine::checkEquivalent( $eproto dst, $eproto src ) {
    if( !determineElementPrototype(dst) or !determineElementPrototype(src) ) return false;
    if( (bool)dst->cons xor (bool)src->cons ) return false;
    if( dst->elmt != src->elmt ) return false;
    return checkEquivalent( dst->dtype, src->dtype );
}

bool Sengine::checkEquivalent( $typeuc dst, $typeuc src ) {
    if( !dst or !src ) return false;
    if( !determineDataType(dst) ) return false;
    if( !determineDataType(src) ) return false;

    if( dst->id == typeuc::NullPointerType ) return src->is(typeuc::PointerType);
    if( src->id == typeuc::NullPointerType ) return dst->is(typeuc::PointerType);
    if( dst->id != src->id and dst->id ) return false;
    if( dst->is(typeuc::PointerType) ) return checkEquivalent( ($typeuc)dst->sub, ($typeuc)src->sub );
    if( dst->is(typeuc::CompositeType) ) return dst->sub == src->sub;
    return true;
}

$imm Sengine::insureEquivalent( $eproto dproto, $imm src, IRBuilder<>& builder, Situation s ) {
    if( !determineElementPrototype(dproto) or !src or !src->eproto() ) return nullptr;
    auto sproto = determineElementPrototype(src->eproto());
    if( !sproto ) return nullptr;

    switch( s ) {
        case Situation::Assigning:
            if( checkEquivalent( dproto, sproto ) ) return src;
            if( sproto->dtype->is(typeuc::BasicType) and dproto->dtype->is(typeuc::BasicType) ) {
                if( getAccuracy(dproto->dtype) > getAccuracy(sproto->dtype) ) 
                    return doConvert( dproto->dtype, src, builder );
            } else if( sproto->dtype->is(typeuc::NullPointerType) and dproto->dtype->is(typeuc::PointerType) ) {
                auto ty = generateTypeUsage(dproto->dtype);
                src = imm::object(builder.CreatePointerCast(src->asobject(builder),ty),eproto::MakeUp(dproto->dtype->name.getScope(),PTR,dproto->dtype));
            } break;
        case Situation::Calculating:
            if( checkEquivalent(dproto,sproto) ) return src;
            if( sproto->dtype->is(typeuc::BasicType) and dproto->dtype->is(typeuc::BasicType) ) {
                if( getAccuracy(dproto->dtype) > getAccuracy(sproto->dtype) ) 
                    return doConvert( dproto->dtype, src, builder );
            } else if( sproto->dtype->is(typeuc::NullPointerType) and dproto->dtype->is(typeuc::PointerType) ) {
                auto ty = generateTypeUsage(dproto->dtype);
                src = imm::object(builder.CreatePointerCast(src->asobject(builder),ty),eproto::MakeUp(dproto->dtype->name.getScope(),PTR,dproto->dtype));
            } break;
            break;
        case Situation::Constructing:
            if( checkEquivalent(dproto,sproto) ) return src;
            if( sproto->dtype->is(typeuc::BasicType) ) {
                if( !dproto->dtype->is(typeuc::BasicType) ) return nullptr;
                if( dproto->elmt == REF and !dproto->cons ) return nullptr;
                if( getAccuracy(dproto->dtype) > getAccuracy(sproto->dtype) ) return doConvert( dproto->dtype, src, builder );
            } else if( sproto->dtype->is(typeuc::NullPointerType) and dproto->dtype->is(typeuc::PointerType) ) {
                auto ty = generateTypeUsage(dproto->dtype);
                src = imm::object(builder.CreatePointerCast(src->asobject(builder),ty),eproto::MakeUp(dproto->dtype->name.getScope(),PTR,dproto->dtype));
            } break;
        case Situation::Passing:
            if( checkEquivalent(dproto,sproto) ) return src;
            if( sproto->dtype->is(typeuc::BasicType) ) {
                if( !dproto->dtype->is(typeuc::BasicType) ) return nullptr;
                if( dproto->elmt == REF and !dproto->cons ) return nullptr;
                if( getAccuracy(dproto->dtype) > getAccuracy(sproto->dtype) ) return doConvert( dproto->dtype, src, builder );
            } else if( sproto->dtype->is(typeuc::NullPointerType) and dproto->dtype->is(typeuc::PointerType) ) {
                auto ty = generateTypeUsage(dproto->dtype);
                src = imm::object(builder.CreatePointerCast(src->asobject(builder),ty),eproto::MakeUp(dproto->dtype->name.getScope(),PTR,dproto->dtype));
            } else {
                //[TODO]: 复合数据类型的转换
            }
            break;
        case Situation::Returning:
            if( checkEquivalent(dproto,sproto) ) return src;
            if( sproto->dtype->is(typeuc::BasicType) and dproto->dtype->is(typeuc::BasicType) ) {
                if( getAccuracy(dproto->dtype) > getAccuracy(sproto->dtype) ) 
                    return doConvert( dproto->dtype, src, builder );
            } else if( sproto->dtype->is(typeuc::NullPointerType) and dproto->dtype->is(typeuc::PointerType) ) {
                auto ty = generateTypeUsage(dproto->dtype);
                src = imm::object(builder.CreatePointerCast(src->asobject(builder),ty),eproto::MakeUp(dproto->dtype->name.getScope(),PTR,dproto->dtype));
            } break;
            break;
        default:break;
    }
    return nullptr;
}

int Sengine::getAccuracy( $typeuc basic ) {
    if( !basic or !basic->is(typeuc::BasicType) ) return 0;

    switch( basic->id ) {
        case typeuc::Float64: 64+100;
        case typeuc::Float32: 32+100;
        case typeuc::Int64: case typeuc::Uint64: return 64;
        case typeuc::Int32: case typeuc::Uint32: return 32;
        case typeuc::Int16: case typeuc::Uint16: return 16;
        case typeuc::Int8: case typeuc::Uint8: return 8;
        case typeuc::BooleanType: return 1;
        default: return 0;
    }
}

$typeuc Sengine::determineDataType( $typeuc type ) {
    if( type->is(typeuc::NamedType) ) {
        auto eve = request( type->name, NormalClass );

        if( eve.size() != 1 ) {
            mlogrepo(type->name.getScope()->getDocPath())(Lengine::E2040,type->phrase)
                (type->name.getScope()->getDocPath(),Lengine::E2004,type->name[-1].name);
            return nullptr;
        }
        if( auto cdef = ($ClassDef)eve[0]; cdef ) {
            type->id = typeuc::CompositeType;
            type->sub = cdef;
            return type;
        } else {
            type->id = typeuc::UnsolvableType;
            mlogrepo(type->name.getScope()->getDocPath())(Lengine::E2040,type->phrase)
                (type->name.getScope()->getDocPath(),Lengine::E2004,type->name[-1].name);
            return nullptr;
        }
    } else if( type->is(typeuc::ThisClassType) ) {
        if( auto scope = ($definition)type->sub; scope ) {
            while( scope and !scope->is(CLASSDEF) ) scope = scope->getScope();
            if( scope ) {
                type->sub = scope;
                type->name *= scope->name;
                type->id = typeuc::CompositeType;
                return type;
            } else {
                type->id = typeuc::UnsolvableType;
                mlogrepo(type->name.getScope()->getDocPath())(Lengine::E2040,type->phrase)
                    (type->name.getScope()->getDocPath(),Lengine::E2004,type->name[-1].name);
                return nullptr;
            }
        } else if( auto def = requestThisClass(($implementation)type->sub); def ) {
            type->sub = def;
            type->name *= scope->name;
            type->id = typeuc::CompositeType;
            return type;
        } else {
            type->id = typeuc::UnsolvableType;
            mlogrepo(type->name.getScope()->getDocPath())(Lengine::E2040,type->phrase)
                (type->name.getScope()->getDocPath(),Lengine::E2004,type->name[-1].name);
            return nullptr;
        }
    } else if( type->is(typeuc::UndeterminedType) ) {
        return nullptr;
    } else if( auto sub = ($typeuc)type->sub; sub ) {
        if( type->is(typeuc::NullPointerType) or determineDataType(sub) ) return type;
        return nullptr;
    } else {
        return type;
    }
}

$eproto Sengine::determineElementPrototype( $eproto proto ) {
    if( !proto ) return nullptr;
    if( !determineDataType(proto->dtype) ) return nullptr;
    if( proto->elmt == UDF ) {
        if( proto->dtype->is(typeuc::PointerType) ) proto->elmt = PTR;
        else proto->elmt = OBJ;
    }  else if( proto->elmt == PTR ) {
        if( !proto->dtype->is(typeuc::PointerType) ) {
            mlogrepo(proto->dtype->name.getScope()->getDocPath())(Lengine::E2039,proto->phrase);
            return nullptr;
        }
    } else if( proto->elmt == OBJ ) {
        if( proto->dtype->is(typeuc::PointerType) ) {
            mlogrepo(proto->dtype->name.getScope()->getDocPath())(Lengine::E2039,proto->phrase);
            return nullptr;
        }
    }

    return proto;
}

$imm Sengine::doConvert( $typeuc dst, $imm value, IRBuilder<>& builder ) {
    auto dstt = generateTypeUsage(dst);
    auto val = value->asobject(builder);
    auto src = value->eproto()->dtype;
    auto droto = eproto::MakeUp( dst->name.getScope(), dst->is(typeuc::PointerType)?PTR:src->is(typeuc::CompositeType)?REF:OBJ, dst );

    if( src->is(typeuc::SignedIntegerType) ) {
        if( dst->is(typeuc::SignedIntegerType) ) return imm::object(builder.CreateIntCast(val,dstt,true), droto->copy() );
        else if( dst->is(typeuc::UnsignedIntegerType) ) return imm::object(builder.CreateIntCast(val,dstt,true),droto->copy() );
        else if( dst->is(typeuc::FloatPointType) ) return imm::object(builder.CreateSIToFP(val,dstt), droto->copy() );
        else if( dst->is(typeuc::PointerType) ) return imm::object(builder.CreateIntToPtr(val,dstt), droto->copy() );
    } else if( src->is(typeuc::UnsignedIntegerType) ) {
        if( dst->is(typeuc::SignedIntegerType) ) return imm::object(builder.CreateIntCast(val,dstt,false), droto->copy() );
        else if( dst->is(typeuc::UnsignedIntegerType) ) return imm::object(builder.CreateIntCast(val,dstt,false),droto->copy() );
        else if( dst->is(typeuc::FloatPointType) ) return imm::object(builder.CreateUIToFP(val,dstt), droto->copy() );
        else if( dst->is(typeuc::PointerType) ) return imm::object(builder.CreateIntToPtr(val,dstt), droto->copy() );
    } else if( src->is(typeuc::PointerType) ) {
        if( dst->is(typeuc::IntegerType) ) return imm::object(builder.CreatePtrToInt(val,dstt), droto->copy() );
        else if( dst->is(typeuc::PointerType) ) return imm::object(builder.CreateBitCast(val,dstt), droto->copy() );
    } else if( src->is(typeuc::FloatPointType) ) {
        if( dst->is(typeuc::SignedIntegerType) ) return imm::object(builder.CreateFPToSI(val,dstt), droto->copy() );
        else if( dst->is(typeuc::UnsignedIntegerType) ) return imm::object(builder.CreateFPToUI(val,dstt), droto->copy() );
        else if( dst->is(typeuc::FloatPointType) ) return imm::object(builder.CreateFPCast(val,dstt), droto->copy() );
    } else if( src->is(typeuc::CompositeType) ) {
        //[TODO]: 完成复合数据类型的类型转换
    }

    return nullptr;
}

/*
$tcp Sengine::checkCompatibility( $typeuc dst, $typeuc src, Situation s ) {
    if( !dst or !src ) return nullptr;
    if( !determineDataType(dst) ) return nullptr;
    if( !determineDataType(src) ) return nullptr;

    if( dst->is(typeuc::CompositeType) ) {
        if( src->is(typeuc::CompositeType) ) {
            if( dst->sub == src->sub ) return new tcp(Noneed,dst,src);
            else {
                /**
                 * [TODO]: 先搜索类型转换运算符重载再搜索
                 *
                throw runtime_error("function not supported yet");
            }
        } else {

        }
    } else if( src->is(typeuc::CompositeType) ) {

    } else if( dst->is(typeuc::UnconstraintedPointerType) ) {

    } else if( dst->is(typeuc::ConstraintedPointerType) ) {

    } else if( dst->id == src->id ) {
        return new tcp(Noneed,dst,src);
    } else {

    }

    return nullptr;
}
*/

$typeuc Sengine::tcd_get_node( $typeuc t ) {
    if( !determineDataType(t) ) return nullptr;
    if( mtcd.cachen.count(t) ) return mtcd.cachen[t];
    for( auto n : mtcd.node ) if( checkEquivalent(n,t) ) {
        mtcd.cachen[t] = n;
        return n;
    }
    auto nt = t->dup();
    mtcd.node << nt;
    mtcd.cachen[t] = nt;
    mtcd.cachen[nt] = nt;
    return nt;
}

bool Sengine::tcd_add_edge( $typeuc dst, $typeuc src, ConvertAction ca ) {
    auto ndst = tcd_get_node(dst);
    auto nsrc = tcd_get_node(src);
    if( !ndst or !nsrc ) return false;
    if( ca == Nocando ) return false;
    if( mtcd.in.count(ndst) ) for( auto path : mtcd.in[ndst] ) if( path->src == nsrc ) return true;

    auto path = new tcp(ca,ndst,nsrc);
    mtcd.edge << path;
    mtcd.in[ndst] << path;
    mtcd.out[nsrc] << path;
    return true;
}

Sengine::Sengine() {
    using namespace sys;
    TargetOptions opt;
    std::string Error;
    auto CPU = "generic";
    auto Features = "";

    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    //LLVMInitializeX86AsmParser();
    LLVMInitializeX86AsmPrinter();

    mttraiple = getDefaultTargetTriple();
    auto target = TargetRegistry::lookupTarget(mttraiple, Error);
    auto RM = Optional<Reloc::Model>();
    mtmachine = target->createTargetMachine( mttraiple, CPU, Features, opt, RM);
}

int Sengine::loadModuleDefinition( $modesc mod ) {

    if( mrepo.count(mod) ) return 1;
    auto err = false;
    auto& root = mrepo[mod] = new module;
    root->desc = mod;
    root->name = mod->name;
    $ClassDef tpclass = nullptr;
    mod->deps.clear();

    for( auto syn : mod->syntrees ) {

        syn->setScope(root);
        mod->deps += syn->signature->deps;

        if( syn->signature->entry ) {
            if( root->es ) {
                mlogrepo(syn->getDocPath())(Lengine::E2031,syn->signature->entry,root->es->getDocPath(),root->es->entry);
                return 0;
            } else {
                root->es = syn->signature;
            }
        }

        for( auto d : syn->signature->deps ) {
            d->self = mod;
            mod->manager->closeUpDependency(d);
        }
        
        for( auto& d : syn->defs ) {
            if( auto trans = ($ClassDef)d; trans and (string)d->name == mod->name ) {
                if( tpclass ) {
                    mlogrepo(d->getDocPath())(Lengine::E2001,d->name,tpclass->getDocPath(),tpclass->name);
                    return 0;
                } else {
                    if( trans->abstract ) {
                        mlogrepo(trans->getDocPath())(Lengine::E2027,trans->phrase);
                        return 0;
                    } else if( trans->alias ) {
                        mlogrepo(trans->getDocPath())(Lengine::E2027,trans->phrase);
                        return 0;
                    } else if( trans->tmpls.size() ) {
                        mlogrepo(trans->getDocPath())(Lengine::E2018,trans->phrase);
                        return 0;
                    }
                    for( auto inst : trans->instdefs ) {
                        if( auto m = ($MethodDef)inst; m and !m->meta ) m->meta = token(VT::META);
                        else if( auto a = ($AttrDef)inst; a and !a->meta ) a->meta = token(VT::META);
                        else if( auto o = ($OperatorDef)inst; o ) {mlogrepo(trans->getDocPath())(Lengine::E2048,o->phrase);err = true;}
                    }
                    root->internal += trans->internal;
                    trans->metadefs = root->metadefs += trans->metadefs + trans->instdefs;
                    trans->instdefs.clear();
                    tpclass = trans;
                } 
            } else {
                root->internal << d;
            }
        }

        root->impls += syn->impls;
    }

    if( err ) return 0;
    if( root->es ) return 2;
    else return 1;
}

bool Sengine::performDefinitionSemanticValidation() {

    bool fine = true;

    for( auto& [desc,mod] : mrepo ) {
        mcurmod = mtrepo[desc] = std::make_shared<Module>(desc->name,mctx);
        fine = performDefinitionSemanticValidation(desc) and fine;
    }

    return fine;
}

Sengine::ModuleTrnsUnit Sengine::performImplementationSemanticValidation( $modesc desc, Dengine& dengine ) {
    using namespace llvm;
    if( mrepo.count(desc) != 1 ) return nullptr;

    auto mod = mrepo[desc];
    mcurmod = mtrepo[desc];
    bool fine = true;

    string src;
    for( auto g : desc->syntrees ) {
        src += dengine.getPath(g->document) + "; ";
    }
    mcurmod->setSourceFileName(src);

    fine = performImplementationSemanticValidation( ($ClassDef)mod );
    for( auto def : mod->internal ) if( auto cdef = ($ClassDef)def; cdef ) {
        fine = performImplementationSemanticValidation(cdef) and fine;
    }
    for( auto im : mod->impls ) if( auto mim = ($MethodImpl)im; mim ) {
        fine = performImplementationSemanticValidation(mim) and fine;
    }

    if( mod->es ) {
        auto res = request(mod->es->entry,NormalClass,mod);
        auto int32 = eproto::MakeUp(mod,OBJ,typeuc::GetBasicDataType(VT::INT32),{});
        auto int8p = eproto::MakeUp(mod,PTR,typeuc::GetBasicDataType(VT::INT8)->getPointerTo()->getPointerTo());
        bool found = false;

        for( auto def : res ) if( auto met = ($MethodDef)def; met ) { 
            if( met->size() != 2 ) continue;
            if( !checkEquivalent( (*met)[0]->proto,int32) ) continue;
            if( !checkEquivalent( (*met)[1]->proto,int8p) ) continue;
            auto fs = generateGlobalUniqueName(($node)met);
            auto ft = (FunctionType*)mnamedT[fs];
            auto fp = mcurmod->getFunction(fs);
            if( !fp ) {fine = false;break;}
            auto start = Function::Create(
                FunctionType::get(Type::getInt32Ty(mctx),{Type::getInt32Ty(mctx),Type::getInt8PtrTy(mctx)->getPointerTo()},false),
                GlobalValue::ExternalLinkage,
                "start",
                mcurmod.get()
            );
            auto ebb = BasicBlock::Create(mctx,"",start);
            auto builder = IRBuilder<>(ebb);
            auto et = mnamedT[generateGlobalUniqueName(($node)mod,Meta)];
            auto gv = mcurmod->getOrInsertGlobal(generateGlobalUniqueName(($node)mod,Entity),et);
            vector<Value*> args;
            args.push_back((Value*)gv);
            args.push_back(start->arg_begin());
            args.push_back(start->arg_begin()+1);
            auto ret = builder.CreateCall(fp,args);
            builder.CreateRet( ret );
            found = true;
            break;
        }

        if( !found ) {
            fine = false;
            mlogrepo(mod->es->getDocPath())(Lengine::E2032,mod->es->entry);
        }
    }

    if( !fine ) mcurmod = nullptr;
    return mcurmod;
}

bool Sengine::triggerBackendTranslation( ModuleTrnsUnit unit, Dengine::vfdm fd, Dengine& dengine ) {
    auto lfd = fd, wfd = fd, efd = fd;
    wfd.name += ".w";
    lfd.name += ".log";
    efd.name += ".ll";
    raw_fd_ostream ldest = raw_fd_ostream(dengine.getOfd(lfd),true);
    raw_fd_ostream dest = raw_fd_ostream(dengine.getOfd(fd),true);
    raw_fd_ostream wdest = raw_fd_ostream(dengine.getOfd(wfd),true);
    legacy::PassManager pass;

    unit->setTargetTriple(mttraiple);
    unit->setDataLayout(mtmachine->createDataLayout());
    auto efdi = dengine.getOfd(efd);
    auto se = dup(2);
    dup2(efdi,2);
    unit->dump();
    dup2(se,2);
    close(efdi);
    mtmachine->addPassesToEmitFile(pass,dest,&wdest,TargetMachine::CGFT_ObjectFile);
    for( auto& fun : unit->getFunctionList() ) {
        string title = "\n--------------------- in function : ";
        title += fun.getName();
        title += "\n";
        ldest.write(title.data(),title.size());
        if( verifyFunction(fun,&ldest) ) return false;
    }
    if( verifyModule(*unit,&ldest) ) return false;
    pass.run(*unit);
    dest.flush();
    return true;

}

Lengine::logr Sengine::getLog() {
    return mlogrepo;
}

}

#endif