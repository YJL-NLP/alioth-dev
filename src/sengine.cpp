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
#include <llvm/IR/GlobalVariable.h>
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
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <cctype>
#include <set>

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
    std::map<string,$definition> nameT;

    auto perform = [&]( auto all, auto symbol, Type*& slot, vector<Type*> members ) {
        if( !slot ) slot = StructType::create(mctx,symbol);
        for( auto def : all ) {
            if( nameT.count((string)def->name) ) {
                auto prev = nameT[(string)def->name];
                mlogrepo(def->getDocPath())(Lengine::E2001,def->name,prev->getDocPath(),prev->name);
                fine = false;
            } else if( auto adef = ($AttrDef)def; adef ) {
                auto mty = performDefinitionSemanticValidation(adef);
                nameT[(string)def->name] = def;
                adef->offset = members.size();
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
                fine = performDefinitionSemanticValidation(odef) and fine;
                auto symbol = generateGlobalUniqueName(($node)odef);
                if( nameT.count(symbol) ) {
                    auto prev = nameT[symbol];
                    mlogrepo(odef->getDocPath())(Lengine::E2001,def->name,prev->getDocPath(),prev->name);
                    fine = false;
                }
            }
        }
        ((StructType*)slot)->setBody(members);
    };

    vector<Type*> basesi,basesm;
    for( const auto& super : clas->supers ) {
        auto sdef = requestClass( super, Len::NormalClass );
        if( !sdef ) {mlogrepo(clas->getDocPath())(Lengine::E2004,super.phrase);continue;}
        auto type = typeuc::GetCompositeType(sdef);
        auto tpi = generateTypeUsage(type,false);
        auto tpm = generateTypeUsage(type,true);
        if( !tpi or !tpm ) continue;
        basesi.push_back(tpi);
        basesm.push_back(tpm);
    }

    auto instS = generateGlobalUniqueName(($node)clas);
    auto metaS = generateGlobalUniqueName(($node)clas,Meta);

    perform( clas->metadefs, metaS, mnamedT[metaS], basesm );
    perform( clas->instdefs, instS, mnamedT[instS], basesi );

    for( auto def : clas->internal ) {
        if( auto cdef = ($ClassDef)def; cdef ) fine = performDefinitionSemanticValidation(cdef) and fine;
        /*else if( auto edef = ($EnumDef)def; edef ) fine = performDefinitionSemanticValidation(edef) and fine;*/
    }
    return fine;
}

bool Sengine::performImplementationSemanticValidation( $ClassDef clas ) {

    function<bool($ClassDef,set<$ClassDef>,anything)> validateUnfold = [&]( $ClassDef def, set<$ClassDef> padding, anything come ) -> bool {

        if( padding.count(def) ) {
            if( auto attr = ($AttrDef)come; attr ) 
                mlogrepo(attr->getDocPath())(Lengine::E2021,attr->name);
            else if( auto super = (agent<nameuc>)come; super )
                mlogrepo(super->getScope()->getDocPath())(Lengine::E2005,super->phrase);
            return false;
        }

        padding.insert(def);

        for( auto& super : def->supers ) {
            auto sdef = requestClass(super,NormalClass,def);
            if( !validateUnfold( sdef, padding, come?come:&super ) ) return false;
        }

        for( auto d : def->instdefs ) if( auto adef = ($AttrDef)d; adef
            and !(bool)adef->meta
            and adef->proto->dtype->is(typeuc::CompositeType)
            and adef->proto->elmt == OBJ ) {
                auto cls = ($ClassDef)adef->proto->dtype->sub;
                if( !validateUnfold( cls, padding, come?come:adef ) ) return false; }
        
        return true;
    };


    function<bool($ClassDef,chainz<map<string,$definition>>)> validateLayout = [&]( $ClassDef def, chainz<map<string,$definition>> layout ) -> bool {
        
        layout.construct(-1);
        
        for( const auto& d : def->instdefs + def->metadefs ) {
            token name;
            if( auto odef = ($OperatorDef)d; odef ) {
                if( odef->name.is(VN::OPL_MEMBER) )
                    name = odef->subtitle;
                else
                    continue;
            } else {
                name = d->name;
            }
            
            for( auto& segment : layout ) {
                if( segment.count( name ) ) if( d->is(METHODDEF) and segment[name]->is(METHODDEF) ) continue;
                mlogrepo(d->getDocPath())(Lengine::E2022,name)
                    (segment[name]->getDocPath(),Lengine::H2002,segment[name]->name);
                return false;
            }

            layout[-1][name] = d;
        }

        for( auto super : def->supers ) {
            auto sdef = requestClass(super,SuperClass,def);
            if( !validateLayout( sdef, layout ) ) return false;
        }

        return true;
    };

    if( !validateUnfold(clas,{},nullptr) ) return false;
    if( !validateLayout(clas,{}) ) return false;
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

    if( !method->meta ) {
        auto tss = generateGlobalUniqueName(method->getScope());
        if( mnamedT.count(tss) == 0 ) mnamedT[tss] = StructType::create(mctx,tss);
        pts.push_back(mnamedT[tss]->getPointerTo());
    }
    
    for( auto par : *method ) {
        auto t = generateTypeUsageAsParameter(par->proto);
        if( !t ) fine = false;
        pts.push_back( t );
    }
    if( !fine or !rtp ) return false;
    
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
            #warning [TODO]: 产生默认运算符体
            return true;
        } else {
            return false;
        }
    }

    #warning [TODO]: 检查其他细节。
    if( opdef->rproto and !determineElementPrototype(opdef->rproto) ) fine = false;
    for( auto par : *opdef ) if( !determineElementPrototype(par->proto) ) fine = false;
     
    if( !opdef->rproto ) opdef->rproto = eproto::MakeUp(opdef,OBJ,typeuc::GetVoidType());
    auto rtp = generateTypeUsageAsReturnValue(opdef->rproto,pts);
    if( !rtp ) fine = false;

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
            if( opdef->size() > 1 )
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
    
    auto def = ($MethodDef)requestPrototype(($implementation)method);
    if( !def ) {
        mlogrepo(method->getDocPath())(Lengine::E2029,method->name);
        for( auto par : *method ) {
            if( par->proto->dtype->is(typeuc::UnsolvableType) )
                mlogrepo(method->getDocPath())(Lengine::E2040,par->proto->dtype->phrase);
        }
        return false;
    }
    enterScope(($implementation)method);
    auto fp = executableEntity( ($node)method );
    auto ebb = BasicBlock::Create(mctx,"",fp);
    auto builder = IRBuilder<>(ebb);
    auto arg = fp->arg_begin();
    if( !def->meta ) arg += 1;
    if( def->rproto->elmt == OBJ and def->rproto->dtype->is(typeuc::CompositeType) ) arg += 1;

    for( auto par : *method ) {
        arg->setName( (string)par->name );
        if( par->proto->elmt == OBJ and par->proto->dtype->is(typeuc::CompositeType) ) {
            registerElement( par, imm::element(arg,par->proto) );
        } else {
            auto addr = builder.CreateAlloca(arg->getType());
            builder.CreateStore(arg,addr);
            registerElement( par, imm::element(addr,par->proto) );
        }
        arg += 1;
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

bool Sengine::performImplementationSemanticValidation( $OperatorImpl oper ) {

    auto def = requestPrototype(($implementation)oper);
    if( !def ) {
        mlogrepo(oper->getDocPath())(Lengine::E2029,oper->name);
        for( auto par : *oper ) {
            if( par->proto->dtype->is(typeuc::UnsolvableType) )
                mlogrepo(oper->getDocPath())(Lengine::E2040,par->proto->dtype->phrase);
        }
        return false;
    }
    auto fp = executableEntity( ($node)oper );
    if( !oper->rproto ) oper->rproto = eproto::MakeUp( oper, OBJ, typeuc::GetVoidType() );

    enterScope( ($implementation)oper );
    auto ebb = BasicBlock::Create(mctx,"",fp);
    auto builder = IRBuilder<>(ebb);
    auto arg = fp->arg_begin();

    for( auto par : *oper ) {
        arg += 1;
        arg->setName( (string)par->name );
        if( par->proto->elmt == OBJ and par->proto->dtype->is(typeuc::CompositeType) ) {
            registerElement( par, imm::element(arg,par->proto) );
        } else {
            auto addr = builder.CreateAlloca(arg->getType());
            builder.CreateStore(arg,addr);
            registerElement( par, imm::element(addr,par->proto) );
        }
    }

    flag_terminate = false;
    if( !performImplementationSemanticValidation( ($implementation)oper->body, builder ) ) return false;

    if( !flag_terminate ) {
        mlogrepo(oper->getDocPath())(Lengine::E2056,oper->body->phrase,oper->name);
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
    else if( auto ct = ($ConstructorImpl)impl; ct ) ret = performImplementationSemanticValidation( ct, builder );
    return ret;
}

bool Sengine::performImplementationSemanticValidation( $InsBlockImpl  impl ,IRBuilder<>& builder ) {
    if( flag_terminate ) {
        mlogrepo(impl->getDocPath())(Lengine::E2033,impl->phrase);
        return false;
    } 
    bool ret = true;

    for( auto imp : impl->impls )
        ret = performImplementationSemanticValidation( imp, builder ) and ret;

    return ret;
}

bool Sengine::performImplementationSemanticValidation( $ConstructorImpl impl, IRBuilder<>& builder ) {
    if( flag_terminate ) {
        mlogrepo(impl->getDocPath())(Lengine::E2033,impl->phrase);
        return false;
    }
    bool ret = true;

    #warning [TODO]: 按照构造顺序建立构造名录名录

    #warning [TODO]: 按照构造名单填充构造表达式，报重复错误

    #warning [TODO]: 执行构造

    for( auto imp : impl->initiate )
        ret = performImplementationSemanticValidation( imp, builder ) and ret;

    return ret;
}

bool Sengine::performImplementationSemanticValidation( $FlowCtrlImpl impl, llvm::IRBuilder<>& builder ) {
    if( flag_terminate ){
        mlogrepo(impl->getDocPath())(Lengine::E2033,impl->phrase);
        return false;
    } 
    auto proto = requestPrototype(($implementation)impl);
    $eproto rproto = nullptr;
    if( auto o = ($OperatorDef)proto; o ) rproto = o->rproto;
    else if( auto m = ($MethodDef)proto; m ) rproto = m->rproto;
    switch( impl->action ) {
        case RETURN: {
            if( impl->expr ) {
                auto v = performImplementationSemanticValidation( impl->expr, builder, AsRetVal ); if( !v ) return false;
                #warning [TODO] : generateBackendIR for <leave> method

                if( auto rv = insureEquivalent(rproto, v, builder, Returning ); rv ) {
                    builder.CreateRet(rv->asunit(builder,*this));
                    flag_terminate = true;
                } else {
                    mlogrepo(impl->getDocPath())(Lengine::E2054,impl->expr->phrase);
                    return false;
                }
            } else if( !rproto->dtype->is(typeuc::VoidType) ) {
                mlogrepo(impl->getDocPath())(Lengine::E2057,impl->phrase,rproto->dtype->phrase);
            } else {
                builder.CreateRetVoid();
                flag_terminate = true;
            }
            return true;
        } break;
        #warning [TODO] : case BREAK: generateBackendIR for <leave> block
        default:
            return true;
    }
}

$imm Sengine::performImplementationSemanticValidation( $ExpressionImpl impl, IRBuilder<>& builder, Position pos ) {
    if( !impl or flag_terminate ) return nullptr;
    switch( impl->type ) {
        default: return nullptr;
        case ExpressionImpl::NAMEUSAGE: return processNameusageExpression( impl, builder, pos );
        case ExpressionImpl::MEMBER: return processMemberExpression( impl, builder, pos );
        case ExpressionImpl::ASSIGN: return processAssignExpression( impl, builder, pos );
        case ExpressionImpl::VALUE: return processValueExpression( impl, builder, pos );
        case ExpressionImpl::SUFFIX: return processCalcExpression( impl, builder, pos );
        case ExpressionImpl::PREFIX: return processCalcExpression( impl, builder, pos );
        case ExpressionImpl::INFIX: return processCalcExpression( impl, builder, pos );
        case ExpressionImpl::CALL: return processCallExpression( impl, builder, pos );
        case ExpressionImpl::CONVERT: return processConvertExpression( impl, builder, pos );
    }
}

$imm Sengine::processNameusageExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos ) {
    if( impl->type != ExpressionImpl::NAMEUSAGE ) return {};

    imms ret;
    
    auto eve = request( impl->name, NormalClass );
    if( eve.size() == 0 ) {
        mlogrepo(impl->getDocPath())(Lengine::E2004,impl->name[-1].name);
        return {};
    }
    for( auto e : eve ) {
        if( auto ce = ($ClassDef)e; ce ) {
            if( pos == Position::AsProc ) continue;
            auto symbolE = generateGlobalUniqueName(($node)e,Entity);
            auto symbolT = generateGlobalUniqueName(($node)e,Meta);
            auto gt = (StructType*)mnamedT[symbolT];
            auto gv = mcurmod->getOrInsertGlobal(symbolE, gt);
            ret << imm::entity( gv, ce );
        } else if( auto cm = ($ConstructImpl)e; cm ) {
            if( pos == Position::AsProc ) continue;
            if( auto inst = lookupElement(cm->name); inst ) ret << inst;
        } else if( auto ad = ($AttrDef)e; ad ) {
            if( pos == Position::AsProc ) continue;
            auto sc = ($ClassDef)ad->getScope();
            auto symbolT = generateGlobalUniqueName(($node)sc,ad->meta?Meta:None);
            Type* stt = mnamedT[symbolT];
            Value* gep = nullptr;
            if( ad->meta ) {
                auto symbolE = generateGlobalUniqueName(($node)sc,Entity);
                auto symbolT = generateGlobalUniqueName(($node)sc,Meta);
                gep = mcurmod->getOrInsertGlobal(symbolE,mnamedT[symbolT]);
            } else {
                auto mdef = requestPrototype(($implementation)impl);
                gep = requestThis(($implementation)impl);
            }
            gep = builder.CreateStructGEP( stt, gep, ad->offset );
            if( gep ) ret << imm::element(gep,ad->proto);
        } else if( auto mt = ($MethodDef)e; mt ) {
            if( pos != Position::AsProc ) continue;
            auto fs = generateGlobalUniqueName(($node)mt);
            auto gv = mcurmod->getFunction(fs);
            if( !gv ) gv = Function::Create((FunctionType*)mnamedT[fs],GlobalValue::ExternalLinkage,fs,mcurmod.get());
            ret << imm::function( gv, mt );
        }
    }

    return selectResult( impl, ret, pos );
}

$imm Sengine::processMemberExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos ) {
    if( impl->type != ExpressionImpl::MEMBER ) return {};

    auto host = performImplementationSemanticValidation( impl->sub[0], builder, BeforeMember );
    if( !host ) return {};
    imms ret;

    function<void($imm,const token&)> select = [&]( $imm v, const token& name ) {

        auto type = host->eproto()->dtype;
        $ClassDef def = nullptr;
        if( auto st = ($typeuc)type->sub; st and (st->is(typeuc::CompositeType) or st->is(typeuc::EntityType)) ) {
            def = st->sub;
            type = type->sub;

        } else if( auto sd = ($ClassDef)type->sub; sd ) {
            def = sd;
        }
        if( !def ) {mlogrepo(impl->getDocPath())(Lengine::E2055,impl->sub[0]->phrase,impl->mean);return;}

        for( auto d : type->is(typeuc::EntityType)?def->metadefs:def->instdefs )
            if( (string)d->name == (string)name ) {
                if( auto ad = ($AttrDef)d; ad ) {
                    auto gep = builder.CreateStructGEP( mnamedT[generateGlobalUniqueName(($node)d,Meta)], v->asaddress(builder,*this), ad->offset );
                    ret << imm::element(gep,ad->proto,v);
                } else if( auto md = ($MethodDef)d; md and pos == AsProc ) {
                    auto fs = generateGlobalUniqueName(($node)md);
                    auto fp = mcurmod->getFunction(fs);
                    if( !fp ) fp = Function::Create( (FunctionType*)mnamedT[fs], GlobalValue::ExternalLinkage, fs, mcurmod.get() );
                    ret << imm::function(fp,md,v);
                }
            } else if( auto od = ($OperatorDef)d; od and od->name.is(VN::OPL_MEMBER) and (string)od->subtitle == (string)name ) {
                if( pos != LeftOfAssign and od->size() != 0 ) continue;
                if( pos == LeftOfAssign and od->size() == 0 ) continue;
                auto fp = executableEntity(($node)od);
                ret << imm::member(fp,od,v);
            }

        for( int i = 0; i != def->supers.size(); i++ ) {
            auto sd = requestClass(def->supers[i], NormalClass);
            if( sd ) {
                auto nv = builder.CreateStructGEP( v->asaddress(builder,*this), i );
                auto np = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetCompositeType(sd));
                auto sp = imm::element( nv, np, host->h );
                select( sp, name );
            }
        }
    };

    select(host, impl->mean);
    return selectResult( impl, ret, pos );
}

$imm Sengine::processAssignExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos ) {
    if( impl->type != ExpressionImpl::ASSIGN ) return nullptr;
    Value* rv;
    $eproto proto;

    //[QUESTION]: 连等于算法的语法分析是否将运算符正确归约了
    auto right = performImplementationSemanticValidation(impl->sub[1],builder, AsOperand);
    auto left = impl->mean.is(VT::ASSIGN)?
        performImplementationSemanticValidation(impl->sub[0],builder, LeftOfAssign ):
        performImplementationSemanticValidation(impl->sub[0],builder, AsOperand );
    if( !left or !right ) return nullptr;

    if( !left->asaddress(builder,*this) ) {
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
        case VT::ASSIGN: builder.CreateStore(right->asunit(builder,*this),left->asaddress(builder,*this)); break;
        case VT::ASSIGN_PLUS:rv = builder.CreateAdd(left->asunit(builder,*this),right->asunit(builder,*this));builder.CreateStore(rv, left->asaddress(builder,*this)); break;
        case VT::ASSIGN_MINUS:rv = builder.CreateSub(left->asunit(builder,*this),right->asunit(builder,*this));builder.CreateStore(rv, left->asaddress(builder,*this)); break;
        case VT::ASSIGN_MUL:rv = builder.CreateMul(left->asunit(builder,*this),right->asunit(builder,*this));builder.CreateStore(rv, left->asaddress(builder,*this)); break;
        case VT::ASSIGN_DIV:rv = builder.CreateSDiv(left->asunit(builder,*this),right->asunit(builder,*this));builder.CreateStore(rv, left->asaddress(builder,*this)); break;
        case VT::ASSIGN_MOL:rv = builder.CreateSRem(left->asunit(builder,*this),right->asunit(builder,*this));builder.CreateStore(rv, left->asaddress(builder,*this)); break;
        case VT::ASSIGN_SHL:rv = builder.CreateShl(left->asunit(builder,*this),right->asunit(builder,*this));builder.CreateStore(rv, left->asaddress(builder,*this)); break;
        case VT::ASSIGN_SHR:rv = builder.CreateAShr(left->asunit(builder,*this),right->asunit(builder,*this));builder.CreateStore(rv, left->asaddress(builder,*this)); break;
        case VT::ASSIGN_bAND:rv = builder.CreateAnd(left->asunit(builder,*this),right->asunit(builder,*this));builder.CreateStore(rv, left->asaddress(builder,*this)); break;
        case VT::ASSIGN_bOR:rv = builder.CreateOr(left->asunit(builder,*this),right->asunit(builder,*this));builder.CreateStore(rv, left->asaddress(builder,*this)); break;
        case VT::ASSIGN_bXOR:rv = builder.CreateXor(left->asunit(builder,*this),right->asunit(builder,*this));builder.CreateStore(rv, left->asaddress(builder,*this)); break;
    }

    return left;
}

$imm Sengine::processValueExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos ) {
    if( impl->type != ExpressionImpl::VALUE ) return nullptr;
    switch( impl->mean.id ) {
        case VT::iINTEGERn: { // 限制10进制数字只能32位
            auto i = std::stoll(impl->mean);
            if( i > INT32_MAX or i < INT32_MIN ) return nullptr;
            return imm::instance( 
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
            return imm::instance( value, proto );
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
            return imm::instance( value, proto );
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
            return imm::instance( value, proto );
        } break;
        case VT::iFLOAT: {
            auto i = std::stod(impl->mean.tx);
            auto value = ConstantFP::get(builder.getContext(), APFloat(i));
            $typeuc type = typeuc::GetBasicDataType(value->getType()->isDoubleTy()?typeuc::Float64:typeuc::Float32);
            auto proto = eproto::MakeUp(impl->getScope(),OBJ,type);
            return imm::instance( value, proto );
        }
        case VT::iSTRING: {
            return imm::instance(
                builder.CreateGlobalStringPtr( Xengine::extractText(impl->mean) ),
                eproto::MakeUp(
                    impl->getScope(),
                    PTR,
                    typeuc::GetPointerType(typeuc::GetBasicDataType(VT::INT8))
                )
            );
        }
        case VT::iNULL: {
            return imm::instance(
                ConstantPointerNull::get(builder.getInt8PtrTy()),
                eproto::MakeUp(
                    impl->getScope(),
                    PTR,
                    typeuc::GetPointerType()
                )
            );
        }
        case VT::iTHIS: {
            return imm::instance(
                requestThis(($implementation)impl),
                eproto::MakeUp(
                    impl->getScope(),
                    REF,
                    typeuc::GetCompositeType(requestThisClass(($implementation)impl))
                )
            );
        }
        case VT::iTRUE: {
            return imm::instance(
                builder.getTrue(),
                eproto::MakeUp(
                    impl->getScope(),
                    OBJ,
                    typeuc::GetBasicDataType(VT::BOOL)
                )
            );
        }
        case VT::iFALSE: {
            return imm::instance(
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

    env_expr.clear();
    auto err = false;
    auto ait = impl->sub.begin();

    while( ++ait != impl->sub.end() ) {
        auto p = performImplementationSemanticValidation( *ait, builder, AsParam );
        if( !p ) err = true;
        else if( !p->eproto() ) {mlogrepo(impl->getDocPath())(Lengine::E2049,(*ait)->phrase );err = true;}
        //else args.push_back( p->asparameter(builder) );
        env_expr << p;
    }
    auto fp = performImplementationSemanticValidation(impl->sub[0], builder, AsProc );
    if( err or !fp ) return nullptr;

    std::vector<Value*> args;
    auto pi = fp->prototype()->begin();
    for( auto& ai : env_expr ) {
        ai = insureEquivalent( (*pi)->proto, ai, builder, Passing );
        args.push_back( ai->asparameter(builder,*this,(*pi++)->proto->elmt) );
    }
    
    if( !fp->prototype()->meta ) {
        if( impl->sub[0]->type == ExpressionImpl::NAMEUSAGE ) {
            args.insert(args.begin(),requestThis(($implementation)impl));
        } else if( impl->sub[0]->type == ExpressionImpl::MEMBER ) {
            args.insert(args.begin(),fp->h->asaddress(builder,*this));
        }
    }

    return generateCall( builder, fp->asfunction(), args, fp->prototype()->rproto );
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
                            case VT::MOL:   rv = builder.CreateSRem(left->asunit(builder,*this),right->asunit(builder,*this)); break;
                            case VT::BITAND:rv = builder.CreateAnd(left->asunit(builder,*this),right->asunit(builder,*this)); break;
                            case VT::BITOR: rv = builder.CreateOr(left->asunit(builder,*this),right->asunit(builder,*this)); break;
                            case VT::BITXOR:rv = builder.CreateXor(left->asunit(builder,*this),right->asunit(builder,*this)); break;
                            case VT::SHL: rv = builder.CreateShl(left->asunit(builder,*this),right->asunit(builder,*this)); break;
                            case VT::SHR: rv = builder.CreateAShr(left->asunit(builder,*this),right->asunit(builder,*this)); break;
                            default: return nullptr;
                        }
                    } break;
                    case VT::GT: rv = builder.CreateICmpSGE(left->asunit(builder,*this),right->asunit(builder,*this));proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;
                    case VT::LT: rv = builder.CreateICmpSLT(left->asunit(builder,*this),right->asunit(builder,*this));proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;
                    case VT::LE: rv = builder.CreateICmpSLT(left->asunit(builder,*this),right->asunit(builder,*this));proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;
                    case VT::GE: rv = builder.CreateICmpSGE(left->asunit(builder,*this),right->asunit(builder,*this));proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;
                    case VT::EQ: rv = builder.CreateICmpEQ(left->asunit(builder,*this),right->asunit(builder,*this)); proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;
                    case VT::NE: rv = builder.CreateICmpNE(left->asunit(builder,*this),right->asunit(builder,*this)); proto = eproto::MakeUp(impl->getScope(),OBJ,typeuc::GetBasicDataType(VT::BOOL));break;

                    case VT::AND: rv = builder.CreateAnd(left->asunit(builder,*this),right->asunit(builder,*this)); break;
                    case VT::OR:  rv = builder.CreateOr(left->asunit(builder,*this),right->asunit(builder,*this)); break;

                    case VT::PLUS:  rv = proto->dtype->is(typeuc::FloatPointType)?
                        builder.CreateFAdd(left->asunit(builder,*this),right->asunit(builder,*this)):
                        builder.CreateAdd(left->asunit(builder,*this),right->asunit(builder,*this)); break;
                    case VT::MINUS: rv = proto->dtype->is(typeuc::FloatPointType)?
                        builder.CreateFSub(left->asunit(builder,*this),right->asunit(builder,*this)):
                        builder.CreateSub(left->asunit(builder,*this),right->asunit(builder,*this)); break;
                    case VT::MUL:   rv = proto->dtype->is(typeuc::FloatPointType)?
                        builder.CreateFMul(left->asunit(builder,*this),right->asunit(builder,*this)):
                        builder.CreateMul(left->asunit(builder,*this),right->asunit(builder,*this)); break;
                    case VT::DIV:   rv = proto->dtype->is(typeuc::FloatPointType)?
                        builder.CreateFDiv(left->asunit(builder,*this),right->asunit(builder,*this)):
                        builder.CreateSDiv(left->asunit(builder,*this),right->asunit(builder,*this)); break;
                }
                return imm::instance( rv, proto );
            }
        } break;
        case ExpressionImpl::SUFFIX: {
            auto operand = performImplementationSemanticValidation(impl->sub[0],builder,AsOperand);
            if( !operand ) return nullptr;
            Value* rv = operand->asunit(builder,*this);
            switch( impl->mean.id ) {
                case VT::INCRESS: builder.CreateStore(builder.CreateAdd(rv,builder.getInt32(1)),operand->asaddress(builder,*this));break;
                case VT::DECRESS: builder.CreateStore(builder.CreateSub(rv,builder.getInt32(1)),operand->asaddress(builder,*this));break;
                case VT::OPENL: {
                    auto ind = performImplementationSemanticValidation(impl->sub[1],builder,AsParam);
                    if( !ind ) return nullptr;
                    rv = builder.CreateGEP(rv,ind->asunit(builder,*this));
                    rv = builder.CreateLoad(rv);
                    auto proto = operand->eproto();
                    proto->dtype = proto->dtype->sub;
                    if( !proto->dtype->is(typeuc::PointerType) ) proto->elmt = OBJ;
                    return imm::instance(rv,proto);
                }
            }
            return imm::instance(rv,operand->eproto());
        } break;
        case ExpressionImpl::PREFIX: {
            auto right = performImplementationSemanticValidation(impl->sub[0],builder,AsOperand);
            if( !right ) return nullptr;
            Value* rv = nullptr;
            switch( impl->mean.id ) {
                default: break;
                case VT::BITAND: {
                    auto proto = right->eproto()->copy();
                    proto->dtype = proto->dtype->getPointerTo();
                    proto->elmt = PTR;
                    return imm::instance(right->asaddress(builder,*this),proto);
                }
                case VT::MUL: {
                    auto proto = right->eproto()->copy();
                    if( !proto->dtype->is(typeuc::PointerType) ) return nullptr; 
                    #warning [TODO]: 报错
                    proto->dtype = proto->dtype->sub;
                    if( !proto->dtype->is(typeuc::PointerType) ) proto->elmt = OBJ;
                    if( proto->dtype->is(typeuc::CompositeType) )
                        return imm::element(right->asunit(builder,*this),proto);
                    return imm::instance(builder.CreateLoad(right->asunit(builder,*this)),proto);
                }
                case VT::NOT: {
                    auto proto = right->eproto();
                    rv = builder.CreateNot(right->asunit(builder,*this));
                    return imm::instance(rv,proto);
                }
                case VT::INCRESS: {
                    builder.CreateStore(builder.CreateAdd(right->asunit(builder,*this),builder.getInt32(1)), right->asaddress(builder,*this) );
                    return right;
                }
                case VT::DECRESS: {
                    builder.CreateStore(builder.CreateSub(right->asunit(builder,*this),builder.getInt32(1)), right->asaddress(builder,*this) );
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

$imm Sengine::selectResult( $ExpressionImpl impl, imms results, Position pos ) {
    if( results.size() == 0 ) mlogrepo(impl->getDocPath())(Lengine::E2004,impl->mean);
    if( pos == Position::AsProc ) {
        for( auto fp = results.begin(); fp != results.end(); fp++ ) {
            auto mproto = (*fp)->prototype();
            if( !mproto ) {results.remove(fp--.pos);continue;}
            if( mproto->size() != env_expr.size() ) 
                results.remove(fp--.pos); 
                #warning [TODO]: 支持默认参数
        }

        for( int i = 0; i < env_expr.size(); i++ ) {
            auto& ai = env_expr[i];
            for( auto fpi = results.begin(); fpi != results.end(); fpi++ ) {
                auto mproto = (*fpi)->prototype();
                if( !insureEquivalent( (*mproto)[i]->proto, ai, Passing ) )
                    results.remove(fpi--.pos);
            }
        }

        $imm alleqfp = nullptr;
        Lengine::logi* log = nullptr;
        bool err = false;
        for( auto tfp : results ) {
            auto mproto = tfp->prototype();
            bool continu = false;

            auto it = env_expr.begin(); for( auto par : *mproto ) {
                if( !checkEquivalent( par->proto->dtype, (*it)->eproto()->dtype ) ) continu = true;
                it++;
            }
            if( continu ) continue;
            if( alleqfp ) {
                if( !log ) log = &mlogrepo(impl->getDocPath())(Lengine::E2050,impl->phrase)
                    (alleqfp->prototype()->getDocPath(),Lengine::E2010,alleqfp->prototype()->name);
                (*log)(mproto->getDocPath(),Lengine::E2010,mproto->name);
                err = true; //important
            } else {
                alleqfp = tfp;
            }
        }
        if( results.size() == 0 or err or !alleqfp and results.size() > 1 ) {
            if( !err ) mlogrepo(impl->getDocPath())(Lengine::E2051,impl->sub[0]->phrase);
            return nullptr;
        }

        auto fp = alleqfp?alleqfp:results[0];
    } else if( pos == Position::LeftOfAssign ) {
        for( auto ri = results.begin(); ri != results.end(); ri++ ) {
            auto& result = *ri;
            if( !result->is(imm::mem) ) {results.remove(ri--.pos);continue;}
            auto def = result->member();
            if( def->size() != 1 ) {results.remove(ri--.pos);continue;}
            if( !insureEquivalent((*def)[0]->proto, env_expr[0], Passing ) ) {results.remove(ri--.pos);continue;}
        }
        $imm alleqop = nullptr;
        for( auto op : results )
            if( checkEquivalent( (*op->member())[0]->proto, env_expr[0]->eproto() ) )
                if( alleqop ) return mlogrepo(impl->getDocPath())(Lengine::E2061,impl->phrase), nullptr;
                else alleqop = op;
        if( alleqop ) return alleqop;
        if( results.size() != 1 ) return mlogrepo(impl->getDocPath())(Lengine::E2061,impl->phrase), nullptr;
        return results[0];
    } else {
        if( results.size() != 1 ) return mlogrepo(impl->getDocPath())(Lengine::E2061,impl->phrase),nullptr;
        return results[0];
    }
}

$imm Sengine::generateCall( IRBuilder<>& builder, Value* fp, vector<Value*> args, $eproto rp ) {
    if( rp->elmt == OBJ and rp->dtype->is(typeuc::CompositeType) ) {
        auto rv = imm::element(builder.CreateAlloca(generateTypeUsage(rp->dtype)),rp);
        registerInstance( rv );
        args.insert(args.begin(),rv->asaddress(builder,*this));
        builder.CreateCall(fp,args);
        return rv;
    } else {
        return imm::instance(builder.CreateCall(fp,args),rp);
    }
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
        if( inm ) inv = inm->asunit(builder,*this);
        else {mlogrepo(impl->getDocPath())(Lengine::E2054,impl->init->phrase);fine = false;}
    }

    if( impl->proto->dtype->is(typeuc::UnknownType) ) {
        if( inm ) impl->proto->dtype = inm->eproto()->dtype;
        else return false;
    }
    auto tp = generateTypeUsageAsAttribute(impl->proto); if( !tp ) return false;
    Value* addr = builder.CreateAlloca(tp, nullptr, (string)impl->name);
    if( inv and fine ) builder.CreateStore(inv,addr);

    if( addr ) registerElement( impl, imm::element(addr,impl->proto) );
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
    builder.CreateCondBr(cond->asunit(builder,*this),bb2,bb3);
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
        bd.CreateCondBr(cond->asunit(builder,*this),bb2,bb3);
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

Type* Sengine::generateTypeUsage( $typeuc type, bool meta ) {
    if( !type ) return nullptr;
    
    if( type->is(typeuc::BasicType) ) {
        switch( type->id ) {
            default : return nullptr;
            case typeuc::BooleanType                :   return Type::getInt1Ty(mctx);    break;
            case typeuc::Int8:  case typeuc::Uint8  :   return Type::getInt8Ty(mctx);    break;
            case typeuc::Int16: case typeuc::Uint16 :   return Type::getInt16Ty(mctx);   break;
            case typeuc::Int32: case typeuc::Uint32 :   return Type::getInt32Ty(mctx);   break;
            case typeuc::Int64: case typeuc::Uint64 :   return Type::getInt64Ty(mctx);   break;
            case typeuc::Float32                    :   return Type::getFloatTy(mctx);   break;
            case typeuc::Float64                    :   return Type::getDoubleTy(mctx);  break;
            case typeuc::VoidType                   :   return Type::getVoidTy(mctx);    break;
        }
    } else if( type->is(typeuc::UndeterminedType) ) {
        return generateTypeUsage(determineDataType(type), meta);
    } else if( type->is(typeuc::CompositeType) ) {
        auto symbol = generateGlobalUniqueName(($node)type->sub,meta?Meta:None);
        if( !mnamedT.count(symbol) ) mnamedT[symbol] = StructType::create(mctx,symbol);
        return mnamedT[symbol];
    } else if( type->is(typeuc::PointerType) ) {
        auto sub = generateTypeUsage(($typeuc)type->sub,meta);
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

    if( auto olist = dynamic_cast<morpheme::plist*>((node*)n); olist ) {
        morpheme::plist list;
        if( n->is(OPERATORDEF) or n->is(OPERATORIMPL) ) {
            for( auto p : *olist ) {
                bool in = false;
                for( auto i = list.begin(); i != list.end(); i++ ) 
                    if( (string)p->name < (string)(*i)->name ) {
                        list.insert(p,i.pos);
                        in = true;
                        break; 
                    }
                list << p;
            }
        }
        for( auto pd : list ) {
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
    }


    if( auto impl = ($MethodImpl)n; impl ) {
        for( int i = impl->cname.size()-1; i >= 0; i-- ) domain = "." + (string)impl->cname[i].name + domain;
        domain += "." + nameProc(impl->name);
        if( impl->constraint ) suffix = "const." + suffix;
        if( auto pro = ($MethodDef)requestPrototype(($implementation)impl);pro and pro->raw ) {
            if( pro->raw.is(VT::iSTRING) ) return Xengine::extractText(pro->raw);
            else return impl->name;
        }
    } else if( auto impl = ($OperatorImpl)n; impl ) {
        for( int i = impl->cname.size()-1; i >= 0; i-- ) domain = "." + (string)impl->cname[i].name + domain;
        domain += "." + nameProc(impl->name);
        if( impl->constraint ) suffix += "." + (string)impl->constraint;
        if( impl->modifier ) suffix += "." + (string)impl->modifier;
        if( impl->subtitle ) suffix += "." + (string)impl->subtitle;
    } else if( auto def = ($MethodDef)n; def ) {
        if( def->raw ) {
            if( def->raw.is(VT::iSTRING) ) return Xengine::extractText(def->raw);
            else return def->name;
        }
        if( def->constraint ) suffix = "const." + suffix;
    } else if( auto def = ($OperatorDef)n; def ) {
        if( def->constraint ) suffix += "." + (string)def->constraint;
        if( def->modifier ) suffix += "." + (string)def->modifier;
        if( def->subtitle ) suffix += "." + (string)def->subtitle;
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

Function* Sengine::executableEntity( $node impl ) {
    if( !impl ) return nullptr;
    auto fs = generateGlobalUniqueName(impl);
    if( mnamedT.count(fs) != 1 ) return nullptr;
    auto ft = (FunctionType*)mnamedT[fs];
    auto fp = mcurmod->getFunction(fs);
    if( !fp ) fp = Function::Create(ft,GlobalValue::ExternalLinkage,fs,mcurmod.get());
    return fp;
}

tuple<$imm,$OperatorDef,$imm> Sengine::selectOperator( $imm left, token op, $imm right ) {
    if( !left or !right ) return {nullptr,nullptr,nullptr};
    if( !op.is(VN::OPL_ADD,VN::OPL_SUB,VN::OPL_MUL,VN::OPL_DIV,VN::OPL_MOL,
        VN::OPL_BITAND,VN::OPL_BITOR,VN::OPL_BITXOR,VN::OPL_SHL,VN::OPL_SHR,
        VN::OPL_EQ,VN::OPL_NE,VN::OPL_GT,VN::OPL_LT,VN::OPL_GE,VN::OPL_LE,
        VN::OPL_AND,VN::OPL_OR,VN::OPL_XOR,
        VN::OPL_ASSIGN,
        VN::OPL_ASSIGN_ADD,VN::OPL_ASSIGN_SUB,VN::OPL_ASSIGN_MUL,VN::OPL_ASSIGN_DIV,VN::OPL_ASSIGN_MOL,
        VN::OPL_ASSIGN_BITAND,VN::OPL_ASSIGN_BITOR,VN::OPL_ASSIGN_BITXOR,VN::OPL_ASSIGN_SHL,VN::OPL_ASSIGN_SHR))
            return {nullptr,nullptr,nullptr};

    auto lp = left->eproto(), rp = right->eproto();
    if( !lp or !rp ) return {nullptr,nullptr,nullptr};
    if( !lp->dtype->is(typeuc::CompositeType) and !rp->dtype->is(typeuc::CompositeType) ) return {nullptr,nullptr,nullptr};

    if( auto lc = ($ClassDef)lp->dtype->sub; lc ) {
        for( auto d : lc->instdefs ) if( auto od = ($OperatorDef)d; od and od->name.in == op.in ) {
            #warning [TODO]: 考虑const
            if( !insureEquivalent((*od->begin())->proto, right, Situation::Passing ) ) continue;
            return {left,od,right};
        }
    }

    if( auto rc = ($ClassDef)rp->dtype->sub; rc ) {
        for( auto d : rc->instdefs ) if( auto od = ($OperatorDef)d; od and od->name.in == op.in and od->modifier.is(CT::MF_REV) ) {
            #warning [TODO]: 考虑const
            if( !insureEquivalent((*od->begin())->proto, left, Situation::Passing ) ) continue;
            return {right,od,left};
        }
    }

    return {nullptr,nullptr,nullptr};
}

$OperatorDef Sengine::selectOperator( token op, $imm right ) {
    if( !right ) return nullptr;
    if( !op.is(VN::OPL_INCREASE,VN::OPL_DECREASE,VN::OPL_SUB,VN::OPL_BITREV) ) return nullptr;
    auto proto = right->eproto();
    if( !proto or !proto->dtype->is(typeuc::CompositeType) ) return nullptr;

    if( auto cd = ($ClassDef)proto->dtype->sub; cd ) 
        for( auto d : cd->instdefs ) 
            if( auto od = ($OperatorDef)d; od and od->modifier.is(CT::MF_PREFIX) and od->name.in == op.in ) {
                return od;
            }
    
    return nullptr;
}

$OperatorDef Sengine::selectOperator( $imm left, token op ) {
    if( !left ) return nullptr;
    if( !op.is(VN::OPL_INCREASE,VN::OPL_DECREASE,VN::OPL_INDEX) ) return nullptr;
    auto proto = left->eproto();
    if( !proto or !proto->dtype->is(typeuc::CompositeType) ) return nullptr;

    if( auto cd = ($ClassDef)proto->dtype->sub; cd ) 
        for( auto d : cd->instdefs ) 
            if( auto od = ($OperatorDef)d; od and ( op.is(VN::OPL_INDEX) or od->modifier.is(CT::MF_SUFFIX) ) and od->name.in == op.in ) {
                return od;
            }
    
    return nullptr;
}

$OperatorDef Sengine::selectOperator( $imm master, token op, token sub, $imm slave ) {
    if( !master ) return nullptr;
    auto proto = master->eproto();
    if( !proto or !proto->dtype->is(typeuc::CompositeType) ) return nullptr;


    if( auto cd = ($ClassDef)proto->dtype->sub; cd )
        for( auto d : cd->instdefs ) {
            if( auto od = ($OperatorDef)d; od and op.in == od->name.in and sub.in == od->subtitle.in ) {
                if( slave and od->size() != 1 ) continue;
                if( !slave and od->size() != 0 ) continue;
                if( slave and !insureEquivalent( (*od->begin())->proto,  slave, Passing ) ) continue;
                return od;
            }
        }
    
    return nullptr;
}

$OperatorDef Sengine::selectOperator( $typeuc type, bundles od ) {
    if( !type or !type->is(typeuc::CompositeType) ) return nullptr;
    
    auto def = ($ClassDef)type->sub; if( !def ) return nullptr;
    chainz<$OperatorDef> sctor;

    for( auto d : def->instdefs ) 
        if( auto od = ($OperatorDef)d; od and od->name.is(VN::OPL_SCTOR) )
            sctor << od;
    if( sctor.size() == 0 ) return nullptr;

    #warning [TODO]: 考虑模板类
    if( od.size() == 0 ) {
        for( auto od : sctor ) if( od->size() == 0 ) return od;
        return nullptr;
    } else for( auto ctor : sctor ) {
        #warning [TODO]
    }

    return nullptr;
}

$OperatorDef Sengine::selectOperator( $imm host ) {

}

$OperatorDef Sengine::selectOperator( $imm master, $typeuc type ) {

}

$OperatorDef Sengine::generateDefaultSctor( $ClassDef cls ) {

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

    if( auto impl = ($implementation)sc; impl and name.size() == 1 ) {
    //// 在实现中
        auto inst = lookupElement( impl, sname );
        if( inst ) res << (anything)inst;
        if( res.size() == 0 ) {
            if( auto org = requestThisClass(impl); !org ) return {};
            else return request( name, ThisClass, org );
        }
    } else if( auto mdef = ($module)sc; mdef ) {
    //// 在模块层
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
                for( auto idef : cdef->instdefs ) 
                    if( auto odef = ($OperatorDef)idef; odef
                        and odef->name.is(VN::OPL_MEMBER)
                        and (string)odef->subtitle == sname ) res << (anything)idef;
                for( const auto& super : cdef->supers ) if( auto sdef = requestClass(super,NormalClass); sdef ) 
                    res += request(name,SuperClass,sdef);
            }
            if( res.size() == 0 and len != SuperClass ) for( auto ndef : cdef->internal ) 
                if( (string)ndef->name == sname ) res << (anything)ndef;
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

$ClassDef Sengine::requestClass( const nameuc& name, Len len, $scope sc ) {
    auto eve = request( name, len, sc );
    if( eve.size() == 1 ) return ($ClassDef)eve[0];
    return nullptr;
}

$ClassDef Sengine::requestThisClass( $implementation impl ) {
    while( impl and !impl->is(METHODIMPL) and !impl->is(OPERATORIMPL) ) impl = impl->getScope();
    if( !impl ) return nullptr;

    if( auto method = ($MethodImpl)impl; method ) {
        if( mmethodP.count(method) ) return mmethodP[method]->getScope();

        return requestClass(method->cname, NormalClass);
    } else if( auto oper = ($OperatorImpl)impl; oper ) {
        if( moperatorP.count(oper) ) return moperatorP[oper]->getScope();

        return requestClass(oper->cname, NormalClass);
    }

    return nullptr;
}

$definition Sengine::requestPrototype( $implementation impl ) {
    while( impl and !impl->is(METHODIMPL) and !impl->is(OPERATORIMPL) ) impl = impl->getScope();
    if( !impl ) return nullptr;

    if( auto met = ($MethodImpl)impl; met ) {
        if( mmethodP.count(met) ) return ($definition)mmethodP[met];
        
        auto scope = requestThisClass(($implementation)met);
        if( !scope ) return nullptr;
        auto sname = (string)met->name;

        for( auto def : scope->instdefs + scope->metadefs ) if( auto mdef = ($MethodDef)def; mdef and (string)mdef->name == sname ) {
            if( mdef->size() != met->size() ) continue;
            if( (bool)mdef->constraint xor (bool)met->constraint ) continue;
            auto arg = met->begin();
            bool found = true;
            for( auto par : *mdef ) {
                found = checkEquivalent((*arg++)->proto, par->proto ) and found;
                if( !found ) break;
            }
            if( found ) return ($definition)(mmethodP[met] = mdef);
        }
    } else if( auto op = ($OperatorImpl)impl; op ) {
        if( moperatorP.count(op) ) return ($definition)moperatorP[op];

        auto scope = requestThisClass(($implementation)op);
        if( !scope ) return nullptr;
        
        for( auto def : scope->instdefs ) if( auto odef = ($OperatorDef)def; odef and odef->name.in == op->name.in and !odef->action ) {
            if( odef->modifier and odef->modifier.tx != op->modifier.tx ) continue;
            if( (bool)odef->constraint xor (bool)op->constraint ) continue;
            if( odef->size() != op->size() ) continue;
            auto arg = op->begin();
            bool found = true;
            for( auto par : *odef ) {
                found = checkEquivalent((*arg++)->proto, par->proto ) and found;
                if( !found ) break;
            }
            if( found ) return ($definition)(moperatorP[op] = odef);
        }
    }

    return nullptr;
}

Value* Sengine::requestThis( $implementation impl ) {
    while( impl and !impl->is(METHODIMPL) and !impl->is(OPERATORIMPL) ) impl = impl->getScope();
    if( !impl ) return nullptr;

    auto pro = requestPrototype(impl);
    int offset = 0;
    if( auto mpro = ($MethodDef)pro; mpro ) {
        if( mpro->meta ) {
            $node sc = mpro->getScope();
            return mcurmod->getOrInsertGlobal(
                generateGlobalUniqueName(sc,Entity),
                mnamedT[generateGlobalUniqueName(sc,Meta)] ); }
        
        if( mpro->rproto->elmt == OBJ and mpro->rproto->dtype->is(typeuc::CompositeType) )
            offset = 1;
    } else if( auto opro = ($OperatorDef)pro; opro ) {
        if( opro->rproto->elmt == OBJ and opro->rproto->dtype->is(typeuc::CompositeType) )
            offset = 1;
    }

    auto fp = mcurmod->getFunction(generateGlobalUniqueName(($node)impl));
    if( !fp ) return nullptr;

    return fp->arg_begin() + offset;
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
    if( insureEquivalent( dproto, src, s ) ) return doConvert( dproto->dtype, src, builder );
    else return nullptr;
}

bool Sengine::insureEquivalent( $eproto dproto, $imm src, Situation s ) {
    if( !determineElementPrototype(dproto) or !src or !src->eproto() ) return false;
    auto sproto = determineElementPrototype(src->eproto());
    if( !sproto ) return false;

    switch( s ) {
        case Situation::Assigning:
            if( checkEquivalent( dproto, sproto ) ) return src;
            if( sproto->dtype->is(typeuc::BasicType) and dproto->dtype->is(typeuc::BasicType) ) {
                if( getAccuracy(dproto->dtype) > getAccuracy(sproto->dtype) ) 
                    return true;
            } else if( sproto->dtype->is(typeuc::NullPointerType) and dproto->dtype->is(typeuc::PointerType) ) {
                return true;
            } break;
        case Situation::Calculating:
            if( checkEquivalent(dproto,sproto) ) return src;
            if( sproto->dtype->is(typeuc::BasicType) and dproto->dtype->is(typeuc::BasicType) ) {
                if( getAccuracy(dproto->dtype) > getAccuracy(sproto->dtype) ) 
                    return true;
            } else if( sproto->dtype->is(typeuc::NullPointerType) and dproto->dtype->is(typeuc::PointerType) ) {
                return true;
            } break;
            break;
        case Situation::Constructing:
            if( checkEquivalent(dproto,sproto) ) return src;
            if( sproto->dtype->is(typeuc::BasicType) ) {
                if( !dproto->dtype->is(typeuc::BasicType) ) return false;
                if( dproto->elmt == REF and !dproto->cons ) return false;
                if( getAccuracy(dproto->dtype) > getAccuracy(sproto->dtype) ) return true;
            } else if( sproto->dtype->is(typeuc::NullPointerType) and dproto->dtype->is(typeuc::PointerType) ) {
                return true;
            } break;
        case Situation::Passing:
            if( checkEquivalent(dproto->dtype,sproto->dtype) ) {
                if( dproto->elmt == REF or dproto->elmt == REL )
                    return src->hasaddress()?true:false;
                else
                    return true;
            } else if( sproto->dtype->is(typeuc::BasicType) ) {
                if( !dproto->dtype->is(typeuc::BasicType) ) return false;
                if( dproto->elmt == REF and !dproto->cons ) return false;
                if( getAccuracy(dproto->dtype) > getAccuracy(sproto->dtype) ) return true;
            } else if( sproto->dtype->is(typeuc::NullPointerType) and dproto->dtype->is(typeuc::PointerType) ) {
                return true;
            } else {
                #warning [TODO]: 复合数据类型的转换
            }
            break;
        case Situation::Returning:
            if( checkEquivalent(dproto,sproto) ) return src;
            if( sproto->dtype->is(typeuc::BasicType) and dproto->dtype->is(typeuc::BasicType) ) {
                if( getAccuracy(dproto->dtype) > getAccuracy(sproto->dtype) ) 
                    return true;
            } else if( sproto->dtype->is(typeuc::NullPointerType) and dproto->dtype->is(typeuc::PointerType) ) {
                return true;
            } break;
            break;
        default:break;
    }
    return false;
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
    auto val = value->asunit(builder,*this);
    auto src = value->eproto()->dtype;

    if( checkEquivalent(dst,src) ) return value;
    auto droto = eproto::MakeUp( dst->name.getScope(), dst->is(typeuc::PointerType)?PTR:src->is(typeuc::CompositeType)?REF:OBJ, dst );

    if( src->is(typeuc::SignedIntegerType) ) {
        if( dst->is(typeuc::SignedIntegerType) ) return imm::instance(builder.CreateIntCast(val,dstt,true), droto->copy() );
        else if( dst->is(typeuc::UnsignedIntegerType) ) return imm::instance(builder.CreateIntCast(val,dstt,true),droto->copy() );
        else if( dst->is(typeuc::FloatPointType) ) return imm::instance(builder.CreateSIToFP(val,dstt), droto->copy() );
        else if( dst->is(typeuc::PointerType) ) return imm::instance(builder.CreateIntToPtr(val,dstt), droto->copy() );
    } else if( src->is(typeuc::UnsignedIntegerType) ) {
        if( dst->is(typeuc::SignedIntegerType) ) return imm::instance(builder.CreateIntCast(val,dstt,false), droto->copy() );
        else if( dst->is(typeuc::UnsignedIntegerType) ) return imm::instance(builder.CreateIntCast(val,dstt,false),droto->copy() );
        else if( dst->is(typeuc::FloatPointType) ) return imm::instance(builder.CreateUIToFP(val,dstt), droto->copy() );
        else if( dst->is(typeuc::PointerType) ) return imm::instance(builder.CreateIntToPtr(val,dstt), droto->copy() );
    } else if( src->is(typeuc::PointerType) ) {
        if( dst->is(typeuc::IntegerType) ) return imm::instance(builder.CreatePtrToInt(val,dstt), droto->copy() );
        else if( dst->is(typeuc::PointerType) ) return imm::instance(builder.CreateBitCast(val,dstt), droto->copy() );
    } else if( src->is(typeuc::FloatPointType) ) {
        if( dst->is(typeuc::SignedIntegerType) ) return imm::instance(builder.CreateFPToSI(val,dstt), droto->copy() );
        else if( dst->is(typeuc::UnsignedIntegerType) ) return imm::instance(builder.CreateFPToUI(val,dstt), droto->copy() );
        else if( dst->is(typeuc::FloatPointType) ) return imm::instance(builder.CreateFPCast(val,dstt), droto->copy() );
    } else if( src->is(typeuc::CompositeType) ) {
        #warning [TODO]: 完成复合数据类型的类型转换
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

bool Sengine::enterScope( $implementation impl ) {
    if( !impl ) return false;
    if( !impl->is(METHODIMPL) and !impl->is(OPERATORIMPL) 
        and !impl->is(BLOCKIMPL) and !impl->is(BRANCHIMPL) and !impl->is(LOOPIMPL) ) {
            return false; }
    
    if( impl->is(METHODIMPL) or impl->is(OPERATORIMPL) ) mstackS.clear();
    else if( mstackS.size() == 0 ) return false;    //其他语法结构需要有方法或运算符作为根

    mstackS.insert((StackSection){title:impl},0);

    return true;
}

bool Sengine::leaveScope( IRBuilder<>& builder, $implementation impl ) {
    #warning [TODO]
    if( mstackS.size() == 0 ) return false;
    if( !impl ) impl = mstackS[0].title;

    bool found = false;
    for( auto& seg : mstackS ) if( seg.title == impl ) { found = true; break;}
    if( !found ) return false;

    for( auto& seg : mstackS ) {
        for( auto& inst : seg.instances ) {
            auto proto = inst->eproto();
            if( proto->elmt == OBJ and proto->dtype->is(typeuc::CompositeType) ) {
                auto op = selectOperator(inst);
                auto opfun = executableEntity(($node)op);
                if( opfun ) builder.CreateCall( opfun, {inst->asaddress(builder,*this)} );
                else found = false;
            }
        }
        if( seg.title == impl ) break;
    }
    
    return found;
}

bool Sengine::registerElement( $ConstructImpl ctis, $imm inst ) {
    if( !ctis or !inst ) return false;
    if( mstackS.size() < 1 ) return false;
    auto& scope = mstackS[0];

    for( auto& [is,in] : scope.elements ) if( (string)ctis->name == (string)is->name ) {
        auto path = scope.title->getDocPath();
        mlogrepo(path)(Lengine::E2001,ctis->name,path,is->name);
        return false;
    }

    scope.elements[ctis] = inst;
    registerInstance(inst);
    return true;
}

bool Sengine::registerInstance( $imm inst ) {
    if( !inst ) return false;
    if( mstackS.size() < 1 ) return false;
    mstackS[0].instances << inst;
    return true;
}

$imm Sengine::lookupElement( const token& name, $implementation sc ) {
    if( mstackS.size() < 1 ) return nullptr;
    if( !sc ) sc = mstackS[0].title;
    while( sc and !sc->is(METHODIMPL) and !sc->is(OPERATORIMPL) 
        and !sc->is(BLOCKIMPL) and !sc->is(BRANCHIMPL) and !sc->is(LOOPIMPL) ) {
            sc = sc->getScope(); }
    if( !sc ) return nullptr;
    bool begin = false;

    for( auto& sec : mstackS ) {
        if( sec.title == sc ) {begin = true;}
        if( !begin ) continue;
        for( auto& [nm,inst] : sec.elements )
            if( (string)nm->name == (string)name ) return inst;
    }
    return nullptr;
}
$ConstructImpl Sengine::lookupElement( $implementation sc, const token& name ) {
    if( mstackS.size() < 1 ) return nullptr;
    if( !sc ) sc = mstackS[0].title;
    while( sc and !sc->is(METHODIMPL) and !sc->is(OPERATORIMPL) 
        and !sc->is(BLOCKIMPL) and !sc->is(BRANCHIMPL) and !sc->is(LOOPIMPL) ) {
            sc = sc->getScope(); }
    if( !sc ) return nullptr;
    bool begin = false;

    for( auto& sec : mstackS ) {
        if( sec.title == sc ) {begin = true;}
        if( !begin ) continue;
        for( auto& [nm,inst] : sec.elements )
            if( (string)nm->name == (string)name ) return nm;
    }
    return nullptr;
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
    } else if( auto oim = ($OperatorImpl)im; oim ) {
        fine = performImplementationSemanticValidation(oim) and fine;
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
            // args.push_back((Value*)gv);
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