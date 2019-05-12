#ifndef __yengine_cpp__
#define __yengine_cpp__

#include "yengine.hpp"

namespace alioth {

Yengine::state::state(int _s,int _c):s(_s),c(_c) {

}
Yengine::state::operator int&() {
    return s;
}

Yengine::smachine::smachine(tokens::iterator& i):it(i) {

}
void Yengine::smachine::movi(int s,int c) {
    states << state(s,c);
    it += c;
}
void Yengine::smachine::movo(int c ) {
    while( c-- > 0 ) {
        it -= states[-1].c;
        states.pop();
    }

}
void Yengine::smachine::stay(int c ) {
    states[-1].c += c;
    it += c;
}
void Yengine::smachine::redu(int c, VN n ) {
    token node = token(n);
    if( c >= 0 ) {
        //node.insert( std::move(*it), 0 );
        node.bl = it->bl;
        node.bc = it->bc;
        node.el = it->el;
        node.ec = it->ec;
        it.r.remove(it.pos);
    }
    else {
        c = -c;
        if( it.pos > 0 ) {
            node.bl = (it-1)->el;       //由于还不能确定状态中是否包含这个单词,所以不能直接囊括其坐标
            node.bc = (it-1)->ec;
            node.el = (it-1)->el;
            node.ec = (it-1)->ec;
        }
    }

    while( c-- > 0 ) {
        while( states[-1].c-- > 0 ) {
            //node.insert( std::move(*(--it)), 0 );
            node.bl = (it-1)->bl;
            node.bc = (it-1)->bc;
            (--it).r.remove(it.pos);
        }
        states.remove(-1);
    }
    
    it.r.insert(std::move(node),it.pos);
}
int Yengine::smachine::size()const {
    return states.size();
}

Yengine::smachine::operator state&() {
    return states[-1];
}

nameuc::atom Yengine::constructAtomicNameUseCase( tokens::iterator& it, Lengine::logs& log, $scope sc, bool absorb ) {
    smachine stack = smachine(it);
    stack.movi(1,0);

    nameuc::atom ret;

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1: 
            if( it->is(VT::LABEL) ) {
                ret.name = *it;
                stack.movi(2);
            } else {
                log(Lengine::E1,*it);
                return {};
            } break;
        case 2:
            if( it->is(VT::LT) and absorb ) {
                stack.movi(3);
                if( it->is(VT::GT) ) {
                    log(Lengine::E401,*(it-1),*it);
                    return {};
                }
            } else {
                stack.redu(-2,VN::NAMEUC_ATOM);
            } break;
        case 3:
            if( it->is(VT::SHR) ) {
                it.r.insert(*it,it.pos);
                it->ec -= 1;
                it->id = (it+1)->id = VT::GT;
                (it+1)->bc += 1;
            } else if( it->is(VT::GT) ) {
                stack.redu(3,VN::NAMEUC_ATOM);
            } else if( it->is(VN::PROTO) ) {
                stack.stay(1);
            } else if( it->is(VT::COMMA) ) {
                stack.stay();
                if( !it->is(VT::LABEL,CT::ELETYPE,CT::BASIC_TYPE,VT::BITXOR,VT::MUL,VT::CONST) ) {
                    log(Lengine::E3,*it);
                    return {};
                }
            } else if( auto p = constructElementPrototype(it,log,sc,true); p ) {
                ret.tmpl << move(p);
            } else {
                return {};
            } break;
    }

    return ret;
}

nameuc Yengine::constructNameUseCase( tokens::iterator& it, Lengine::logs& log, $scope sc, bool absorb ) {
    smachine stack = smachine(it);

    auto a = constructAtomicNameUseCase(it,log,sc,absorb);
    if( !a ) return {};
    nameuc ret = a;
    ret.setScope(sc);
    stack.movi(1);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1:
            if( it->is(VT::SCOPE) ) {
                stack.movi(2);
            } else if( it->is(VN::NAMEUC) ) {
                stack.stay();
            } else {
                stack.redu(-1,VN::NAMEUC);
            } break;
        case 2:
            if( it->is(VT::LABEL) ) {
                auto a = constructAtomicNameUseCase(it,log,sc,absorb);
                if( !a ) return {};
                ret.msequence << move(a);
            } else if( it->is(VN::NAMEUC_ATOM) ) {
                stack.redu(1,VN::NAMEUC);
            } else {
                log(Lengine::E1,*it);
                return {};
            } break;
    }

    ret.phrase = *it;
    return ret;
}

token Yengine::constructOperatorLabel( tokens::iterator& it, Lengine::logs& log, token& sub ) {
    smachine stack = smachine(it);
    stack.movi(1,0);
    
    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1: 
            if( it->is(VT::OPENS) ) {
                stack.movi(110);
            } else if( it->is(VT::OPENL) ) {
                stack.movi(120);
            } else if( it->is(CT::LB_SCTOR) ) {
                stack.redu(1,VN::OPL_SCTOR);
            } else if( it->is(CT::LB_LCTOR) ) {
                stack.redu(1,VN::OPL_LCTOR);
            } else if( it->is(CT::LB_CCTOR) ) {
                stack.redu(1,VN::OPL_CCTOR);
            } else if( it->is(CT::LB_MCTOR) ) {
                stack.redu(1,VN::OPL_MCTOR);
            } else if( it->is(CT::LB_DTOR) ) {
                stack.redu(1,VN::OPL_DTOR);
            } else if( it->is(VT::AS) ) {
                stack.redu(1,VN::OPL_AS);
            } else if( it->is(CT::LB_MOVE) ) {
                stack.redu(1,VN::OPL_MOVE);
            } else if( it->is(CT::LB_NEGATIVE) ) {
                stack.redu(1,VN::OPL_NEGATIVE);
            } else if( it->is(CT::LB_BITREV) ) {
                stack.redu(1,VN::OPL_BITREV);
            } else if( it->is(CT::LB_INCREASE) ) {
                stack.redu(1,VN::OPL_INCREASE);
            } else if( it->is(CT::LB_DECREASE) ) {
                stack.redu(1,VN::OPL_DECREASE);
            } else if( it->is(VT::NOT) ) {
                stack.redu(1,VN::OPL_NOT);
            } else if( it->is(CT::LB_ADD) ) {
                stack.redu(1,VN::OPL_ADD);
            } else if( it->is(CT::LB_SUB) ) {
                stack.redu(1,VN::OPL_SUB);
            } else if( it->is(CT::LB_DIV) ) {
                stack.redu(1,VN::OPL_DIV);
            } else if( it->is(CT::LB_MOL) ) {
                stack.redu(1,VN::OPL_MOL);
            } else if( it->is(CT::LB_BITAND) ) {
                stack.redu(1,VN::OPL_BITAND);
            } else if( it->is(CT::LB_BITOR) ) {
                stack.redu(1,VN::OPL_BITOR);
            } else if( it->is(CT::LB_BITXOR) ) {
                stack.redu(1,VN::OPL_BITXOR);
            } else if( it->is(CT::LB_SHL) ) {
                stack.redu(1,VN::OPL_SHL);
            } else if( it->is(CT::LB_SHR) ) {
                stack.redu(1,VN::OPL_SHR);
            } else if( it->is(CT::LB_LT) ) {
                stack.redu(1,VN::OPL_LT);
            } else if( it->is(CT::LB_GT) ) {
                stack.redu(1,VN::OPL_GT);
            } else if( it->is(CT::LB_LE) ) {
                stack.redu(1,VN::OPL_LE);
            } else if( it->is(CT::LB_GE) ) {
                stack.redu(1,VN::OPL_GE);
            } else if( it->is(CT::LB_EQ) ) {
                stack.redu(1,VN::OPL_EQ);
            } else if( it->is(CT::LB_NE) ) {
                stack.redu(1,VN::OPL_NE);
            } else if( it->is(VT::AND) ) {
                stack.redu(1,VN::OPL_AND);
            } else if( it->is(VT::OR) ) {
                stack.redu(1,VN::OPL_OR);
            } else if( it->is(VT::XOR) ) {
                stack.redu(1,VN::OPL_XOR);
            } else if( it->is(VT::NOT) ) {
                stack.redu(1,VN::OPL_NOT);
            } else if( it->is(CT::LB_MEMBER) ) {
                stack.movi(130);
            } else if( it->is(CT::LB_WHERE) ) {
                stack.movi(131);
            } else if( it->is(CT::LB_ASSIGN) ) {
                stack.movi(132);
            } else {
                log(Lengine::E2036,*it);
                return VT::R_ERR;
            } break;
        case 110:
            if( it->is(VT::ETC) ) {
                stack.movi(111);
            } else if( it->is(VT::ASSIGN) ) {
                stack.movi(112);
            } else if( it->is(VT::LT) ) {
                stack.movi(113);
            } else if( it->is(VT::BITREV) ) {
                stack.movi(114);
            } else {
                log(Lengine::E2036,*it);
                return VT::R_ERR;
            } break;
        case 111:
            if( it->is(VT::CLOSES) ) {
                stack.redu(3,VN::OPL_SCTOR);
            } else {
                log(Lengine::E2036,*it);
                return VT::R_ERR;
            } break;
        case 112:
            if( it->is(VT::CLOSES) ) {
                stack.redu(3,VN::OPL_CCTOR);
            } else {
                log(Lengine::E2036,*it);
                return VT::R_ERR;
            } break;
        case 113:
            if( it->is(VT::CLOSES) ) {
                stack.redu(3,VN::OPL_MCTOR);
            } else {
                log(Lengine::E2036,*it);
                return VT::R_ERR;
            } break;
        case 114:
            if( it->is(VT::CLOSES) ) {
                stack.redu(3,VN::OPL_DTOR);
            } else {
                log(Lengine::E2036,*it);
                return VT::R_ERR;
            } break;
        case 120:
            if( it->is(VT::ETC) ) {
                stack.movi(121);
            } else if( it->is(VT::CLOSEL) ) {
                stack.redu(2,VN::OPL_INDEX);
            } else {
                log(Lengine::E2036,*it);
                return VT::R_ERR;
            } break;
        case 121:
            if( it->is(VT::CLOSEL) ) {
                stack.redu(3,VN::OPL_LCTOR);
            } else {
                log(Lengine::E2036,*it);
                return VT::R_ERR;
            } break;
        case 130:
            if( it->is(VT::LABEL) ) {
                sub = *it;
                stack.redu(2,VN::OPL_MEMBER);
            } else {
                log(Lengine::E2036,*it);
                return VT::R_ERR;
            } break;
        case 131:
            if( it->is(VT::iINTEGERn) ) {
                sub = *it;
                stack.redu(2,VN::OPL_WHERE);
            } else {
                log(Lengine::E2036,*it);
                return VT::R_ERR;
            } break;
        case 132:
            if( it->is(CT::LB_ADD) ) {
                stack.redu(2,VN::OPL_ASSIGN_ADD);
            } else if( it->is(CT::LB_SUB) ) {
                stack.redu(2,VN::OPL_ASSIGN_SUB);
            } else if( it->is(CT::LB_MUL) ) {
                stack.redu(2,VN::OPL_ASSIGN_MUL);
            } else if( it->is(CT::LB_DIV) ) {
                stack.redu(2,VN::OPL_ASSIGN_DIV);
            } else if( it->is(CT::LB_MOL) ) {
                stack.redu(2,VN::OPL_ASSIGN_MOL);
            } else if( it->is(CT::LB_SHL) ) {
                stack.redu(2,VN::OPL_ASSIGN_SHL);
            } else if( it->is(CT::LB_SHR) ) {
                stack.redu(2,VN::OPL_ASSIGN_SHR);
            } else if( it->is(CT::LB_BITAND) ) {
                stack.redu(2,VN::OPL_ASSIGN_BITAND);
            } else if( it->is(CT::LB_BITOR) ) {
                stack.redu(2,VN::OPL_ASSIGN_BITOR);
            } else if( it->is(CT::LB_BITXOR) ) {
                stack.redu(2,VN::OPL_ASSIGN_BITXOR);
            } else {
                stack.redu(-2,VN::OPL_ASSIGN);
            } break;
    }

    return *it;
}

bool Yengine::constructParameterList( tokens::iterator& it, Lengine::logs& log, $scope sc, morpheme::plist& ps ) {
    using namespace morpheme;
    smachine stack = it;
    stack.movi(1,0);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1 :
            if( it->is(VT::OPENA) ) {
                stack.movi(2);
            } else {
                log(Lengine::E305,*it);
                return false;
            } break;
        case 2 :
            if( it->is(VT::CLOSEA) ) {
                stack.redu(2,VN::PARAM_LIST);
            } else if( it->is(VT::ETC) ) {
                ps.vargs = *it;
                stack.movi(5);
            } else if( it->is(VN::FINAL_PARAM) ) {
                stack.stay();
                if( !it->is(VT::CLOSEA) ) {
                    log(Lengine::E202,")",*it);
                    return false;
                }
            } else if( it->is(VN::PARAM,VN::COMMA_PARAM) ) {
                stack.movi(3);
            } else if( auto par = constructParameterDefinition(it,log,sc); par ) {
                ps << par;
            } else {
                return false;
            } break;
        case 3 :
            if( it->is(VT::COMMA) ) {
                stack.movi(4);
            } else if( it->is(VT::CLOSEA) ) {
                stack.redu(3,VN::PARAM_LIST);
            } else {
                log(Lengine::E201,*it);
                return false;
            } break;
        case 4 :
            if( it->is(VN::PARAM) ) {
                stack.redu(2,VN::COMMA_PARAM);
            } else if( it->is(VN::FINAL_PARAM) ) {
                stack.redu(2,VN::FINAL_PARAM);
            } else if( it->is(VT::ETC) ) {
                ps.vargs = *it;
                stack.movi(5);
            } else if( auto par = constructParameterDefinition(it,log,sc); par ) {
                ps << par;
            } else {
                return false;
            } break;
        case 5 :
            if( it->is(VT::LABEL) ) {
                ps.vargs = *it;
                stack.redu(1,VN::FINAL_PARAM);
            } else {
                stack.redu(-1,VN::FINAL_PARAM);
            } break;
    }

    return true;
}

$ConstructImpl Yengine::constructParameterDefinition( tokens::iterator& it, Lengine::logs& log, $scope sc ) {
    smachine stack = smachine(it);
    auto et = it->is(CT::ELETYPE)?(etype)(int)it->id:UDF;
    token ct;
    stack.movi(1,et==UDF?0:1);
    $ConstructImpl def = new ConstructImpl;
    def->setScope(sc);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1:
            if( it->is(VT::CONST) ) {
                if( ct ) {
                    log(Lengine::E302,*it);
                    return nullptr;
                }
                ct = *it;
                stack.stay();
            } else if( it->is(VT::LABEL) ) {
                def->name = *it;
                stack.movi(2);
            } else {
                log(Lengine::E306,*it);
                return nullptr;
            } break;
        case 2:
            if( it->is(VN::TYPEUC) ) {
                stack.redu(2,VN::PARAM);
            } else if( auto dt = constructDataType(it,log,sc,true); dt ) {
                if( dt->is(typeuc::UnknownType) and et == UDF ) {
                    log(Lengine::E6,def->name);
                    return nullptr;
                }
                def->proto = eproto::MakeUp(sc,et,dt,ct);
            } else {
                log(Lengine::E6,*it);
                return nullptr;
            } break;
            //[TODO]: 默认参数解析
    }

    def->phrase = *it;
    return def;
}

$ConstructImpl Yengine::constructParameterImplementation( tokens::iterator& it, Lengine::logs& log, $scope sc ) {
    smachine stack = smachine(it);
    auto et = it->is(CT::ELETYPE)?(etype)(int)it->id:UDF;
    token ct;
    stack.movi(1,et==UDF?0:1);
    $ConstructImpl ele = new ConstructImpl;
    ele->setScope(sc);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1:
            if( it->is(VT::CONST) ) {
                if( ct ) {
                    log(Lengine::E302,*it);
                    return nullptr;
                }
                ct = *it;
                stack.stay();
            } else if( it->is(VT::LABEL) ) {
                ele->name = *it;
                stack.movi(2);
            } else {
                log(Lengine::E306,*it);
                return nullptr;
            } break;
        case 2:
            if( it->is(VN::TYPEUC) ) {
                stack.redu(2,VN::PARAM);
            } else if( auto dt = constructDataType(it,log,sc,true); dt ) {
                if( dt->is(typeuc::UnknownType) and et == UDF ) {
                    log(Lengine::E6,ele->name);
                    return nullptr;
                }
                ele->proto = eproto::MakeUp(sc,et,dt,ct);
            } else {
                log(Lengine::E6,*it);
                return nullptr;
            } break;
    }

    ele->phrase = *it;
    return ele;
}

$typeuc Yengine::constructDataType( tokens::iterator& it, Lengine::logs& log, $scope sc, bool absorb ) {
    smachine stack = smachine(it);
    $typeuc ret = new typeuc;
    stack.movi(1,0);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1:
            if( it->is(VT::BITXOR) ) {
                ret->id = typeuc::ConstraintedPointerType;
                stack.movi(2);
            } else if( it->is(VT::MUL) ) {
                ret->id = typeuc::UnconstraintedPointerType;
                stack.movi(2);
            } else if( it->is(CT::BASIC_TYPE) ) {
                ret = typeuc::GetBasicDataType(it->id);
                stack.redu(1,VN::TYPEUC);
            } else if( it->is(VT::LABEL) ) {
                ret->name = constructNameUseCase(it,log,sc,absorb);
                if( !ret->name ) return nullptr;
                ret->id = typeuc::NamedType;
                stack.redu(1,VN::TYPEUC);
            } else if( it->is(VT::iTHIS) ) {
                stack.movi(3);
            } else {
                ret->id = typeuc::UnknownType;
                stack.redu(-1,VN::TYPEUC);
            } break;
        case 2:
            if( auto dt = constructDataType(it, log, sc, absorb); !dt ) {
                return nullptr;
            } else if( dt->is(typeuc::UnknownType) ) {
                log(Lengine::E501,*it);
                return nullptr;
            } else {
                ret->sub = dt;
                stack.redu(2,VN::TYPEUC);
            } break;
        case 3:
            if( it->is(VT::CLASS) ) {
                ret->id = typeuc::NamedType;
                ret->sub = sc;
                stack.redu(2,VN::TYPEUC);
            } else {
                log(Lengine::E202,VT::CLASS,*it);
                return nullptr;
            } break;
    }

    ret->phrase = *it;
    ret->setScope(sc);
    return move(ret);
}

$eproto Yengine::constructElementPrototype( tokens::iterator& it, Lengine::logs& log, $scope sc, bool absorb ) {
    smachine stack = smachine(it);
    $eproto ret = new eproto;
    
    if( it->is(VT::OBJ) ) {
        ret->elmt = OBJ;
    } else if( it->is(VT::PTR) ) {
        ret->elmt = PTR;
    } else if( it->is(VT::REF) ) {
        ret->elmt = REF;
    } else if( it->is(VT::REL) ) {
        ret->elmt = REL;
    } else {
        it.r.insert(*it,it.pos);
        it->id = VT::SPACE;
        it->el = it->bl;
        it->ec = it->bc;
        ret->elmt = UDF;
    }

    stack.movi(1);
    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1: 
            if( it->is(VT::CONST) ) {
                ret->cons = *it;
                stack.stay(1);
                if( it->is(VT::CONST) ) {
                    log(Lengine::E502,*it);
                    return nullptr;
                }
            } else if( it->is(VN::TYPEUC) ) {
                stack.redu(1,VN::PROTO);
            } else {
                ret->dtype = constructDataType(it,log,sc,absorb);
                if( !ret->dtype ) return nullptr;   //由于分支的覆盖能力,此处不能检查数据类型与元素类型是否相容
                if( ret->elmt == UDF and ret->dtype->is(typeuc::UnknownType) ) {
                    log(Lengine::E6,*it);
                    return nullptr;
                }
            } break;
    }

    ret->phrase = *it;
    ret->setScope(sc);
    return move(ret);
}

$ModuleGranule Yengine::constructSyntaxTree( tokens& is, Lengine::logs& log ) {
    if( is.size() < 2 or!is[0].is(VT::R_BEG) or !is[-1].is(VT::R_END) ) {
        log( Lengine::E0 );
        return nullptr;
    }

    /**
     * 剔除注释和空白
     * [TODO]: 将注释留给需要调试信息的地方,将注释和源码加入调试信息
     */
    for( auto i = is.begin(); i != is.end(); ) 
        if( i->is(VT::COMMENT,VT::SPACE) ) is.remover(*i);
        else i += 1;

    auto it = is.begin();
    auto stack = smachine(it);
    $ModuleGranule ref = new ModuleGranule;

    stack.movi(1);
    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1: 
            if( it->is(VT::MODULE) ) {
                ref->signature = constructModuleSignature(it,log,ref);
                if( !ref->signature ) return nullptr;
            } else if( it->is(VN::MODULE) ) {
                stack.movi(2);
            } else {
                return log(Lengine::E202,VT::MODULE,*it),nullptr;
            } break;
        case 2:
            if( it->is(VT::R_END) ) {
                stack.redu(2,VN::MODULE);
            } else if( it->is(VT::METHOD) ) {
                auto met = constructMethodImplementation(it,log,ref);
                if( !met ) return nullptr;
                ref->impls << ($implementation)met;
            } else if( it->is(VT::CLASS) ) {
                auto cls = constructClassDefinition(it,log,ref,0);
                if( !cls ) return nullptr;
                ref->defs << ($definition)cls;
            } else if( it->is(VT::ENUM) ) {
                auto enm = constructEnumDefinition(it,log,ref,0);
                if( !enm ) return nullptr;
                ref->defs << ($definition)enm;
            } else if( it->is(VN::METHOD,VN::CLASS,VN::ENUM) ) {
                stack.stay();
            } else {
                log(Lengine::E201,*it);
                return nullptr;
            } break;
    }

    ref->phrase = *it;
    return ref;
}

$ModuleSignature Yengine::detectModuleSignature( tokens& is, Lengine::logs& log ) {
    if( is.size() < 2 or!is[0].is(VT::R_BEG) or !is[-1].is(VT::R_END) ) {
        log( Lengine::E0 );
        return nullptr;
    }

    /**
     * 剔除注释和空白
     * [TODO]: 将注释留给需要调试信息的地方,将注释和源码加入调试信息
     */
    for( auto i = is.begin(); i != is.end(); ) 
        if( i->is(VT::COMMENT,VT::SPACE) ) is.remover(*i);
        else i += 1;

    auto it = is.begin()+1;
    return constructModuleSignature( it, log, nullptr );
}

$ModuleSignature Yengine::constructModuleSignature( tokens::iterator& it, Lengine::logs& log, $scope scope ) {
    $ModuleSignature sig = nullptr;
    auto stack = smachine(it);

    stack.movi(1,0);
    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1: 
            if( it->is(VT::MODULE) ) stack.movi(2);
            else return log(Lengine::E202,VT::MODULE,*it),nullptr;
            break;
        case 2:
            if( it->is(VT::LABEL) ) {
                sig = new ModuleSignature((string)*it);
                stack.movi(3);
            } else {
                return log(Lengine::E203,*it),nullptr;
            } break;
        case 3:
            if( it->is(VT::COLON) ) stack.movi(4);
            else if( it->is(VT::ENTRY) ) stack.movi(5);
            else if( it->is(VN::LIST) ) stack.redu(3,VN::MODULE);
            else stack.redu(-3,VN::MODULE);
            break;
        case 4:
            if( it->is(VT::LABEL) ) {
                auto im = constructDependencyDescriptor(it,log,sig);
                if( !im ) return nullptr;
                sig->deps << im;
            } else if( it->is(VN::DEPENDENCY) ) {
                stack.stay(1);
            } else if( it->is(VT::SEMI) ) {
                stack.redu(1,VN::LIST);
            } else {
                stack.redu(-1,VN::LIST);
            } break;
        case 5:
            if( it->is(VT::LABEL) ) {
                sig->entry = *it;
                stack.movi(6);
            } else {
                log(Lengine::E2030,*it);
                return nullptr;
            } break;
        case 6:
            if( it->is(VT::COLON) ) stack.movi(4);
            else if( it->is(VN::LIST) ) stack.redu(5,VN::MODULE);
            else stack.redu(-5,VN::MODULE);
            break;
    }

    sig->phrase = *it;
    sig->setScope(scope);
    return sig;
}

$depdesc Yengine::constructDependencyDescriptor( tokens::iterator& it, Lengine::logs& log, $scope scope ) {
    smachine stack = smachine(it);

    $depdesc ref = new depdesc;
    
    stack.movi(1,0);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1:
            if( it->is(VT::LABEL) ) {ref->name = *it;stack.movi(2);}
            else return log(Lengine::E51,*it),nullptr;
            break;
        case 2:
            if( it->is(VT::AT) ) stack.movi(3);
            else if( it->is(VT::AS) ) stack.movi(5);
            else if( it->is(VN::LIST) ) stack.stay();
            else stack.redu(-2,VN::DEPENDENCY);
            break;
        case 3:
            if( it->is(VT::LABEL,VT::MEMBER,VT::iSTRING,VT::iCHAR) ) {ref->mfrom = *it;stack.movi(4);}
            else return log(Lengine::E52,*it),nullptr;
            break;
        case 4:
            if( it->is(VT::AS) ) {stack.redu(-2,VN::LIST);}
            else stack.redu(-4,VN::DEPENDENCY);
            break;
        case 5:
            if( it->is(VT::iTHIS) ) {ref->alias = *it;stack.movi(6);}
            else if( it->is(VT::LABEL) ) {ref->alias = *it;stack.redu(3,VN::DEPENDENCY);}
            else return log(Lengine::E53,*it),nullptr;
            break;
        case 6:
            if( it->is(VT::MODULE) ) {stack.redu(4,VN::DEPENDENCY);}
            else return log(Lengine::E202,VT::MODULE,*it),nullptr;
    }

    ref->phrase = *it;
    ref->setScope(scope);
    return ref;
}

$EnumDef Yengine::constructEnumDefinition( tokens::iterator& it, Lengine::logs& log, $scope scope, int wn ) {
    smachine stack = it;
    $EnumDef ret = new EnumDef;
    ret->wrtno = wn;
    ret->setScope(scope);
    stack.movi(1,0);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1 :
            if( it->is(VT::ENUM) ) {
                stack.movi(2);
            } else {
                log(Lengine::E202,VT::ENUM,*it);
                return nullptr;
            } break;
        case 2 :
            if(it->is(VT::PUBLIC,VT::PRIVATE)) {
                if( ret->visibility ) {
                    if(ret->visibility.id == it->id) {
                        log(Lengine::E302, *it);
                        log(Lengine::E302, ret->visibility);
                    } else {
                        log(Lengine::E301, *it);
                        log(Lengine::E301, ret->visibility);
                    } 
                    return nullptr;
                }
                ret->visibility = *it;
                stack.stay();
            } else if( it->is(VT::LABEL) ) {
                ret->name = *it;
                stack.movi(3);
            } else {
                log(Lengine::E701,*it);
                return nullptr;
            } break;
        case 3 :
            if( it->is(VT::OPENS) ) {
                stack.movi(4);
            } else {
                log(Lengine::E702,*it);
                return nullptr;
            } break;
        case 4 :
            if( it->is(VT::CLOSES) ) {
                stack.redu(4,VN::ENUM);
                ret->phrase = *it;
            } else if( it->is(VT::LABEL) ) {
                if( (string)*it == (string)ret->name ) {
                    log(Lengine::E2001,*it,ret->getDocPath(),ret->name);
                    return nullptr;
                }
                ret->items << *it;
                stack.stay();
            } else {
                log(Lengine::E703,*it);
                return nullptr;
            } break;
    }

    ret->phrase = *it;
    return ret;
}

$ClassDef Yengine::constructClassDefinition( tokens::iterator& it, Lengine::logs& log, $scope scope, int wn ) {
    smachine stack = it;
    $ClassDef ret = new ClassDef;
    ret->wrtno = wn;
    ret->setScope(scope);
    int flag = 0;
    int wrtno = 0;
    stack.movi(1, 0);

    while (stack.size() > 0)
        switch ((state)stack) {
        case 1:
            if(it->is(VT::CLASS)) {
                stack.movi(2);
            } else {
                log(Lengine::E202, VT::CLASS, *it);
                return nullptr;
            } break;
        case 2:
            if(it->is(VT::PUBLIC,VT::PRIVATE)) {
                if( ret->visibility ) {
                    if(ret->visibility.id == it->id) {
                        log(Lengine::E302, *it);
                        log(Lengine::E302, ret->visibility);
                    } else {
                        log(Lengine::E301, *it);
                        log(Lengine::E301, ret->visibility);
                    } 
                    return nullptr;
                }
                ret->visibility = *it;
                stack.stay();
            } else if( it->is(CT::MF_ABSTRACT) and isalpha((it+1)->tx[0]) ) {
                if(ret->abstract) {
                    log(Lengine::E801, *it);
                    log(Lengine::E801, ret->abstract);
                    return nullptr;
                }
                ret->abstract = *it;
                stack.stay();
                if(!it->is(VT::LABEL)) {
                    log(Lengine::E802, *it);
                    return nullptr;
                }
            } else if(it->is(VT::LABEL)) {
                ret->name = *it;
                stack.movi(3);
            } else {
                log(Lengine::E802, *it);
                return nullptr;
            } break;
        case 3:
            if(it->is(VT::ASSIGN)) {
                stack.movi(7);
            } else if(it->is(VT::LT)) {
                stack.movi(4);
            } else if(it->is(VN::CLASS_TEMPLATE_PARAMLIST)) {
                stack.stay();
                if(!it->is(VT::COLON, VT::OPENS,VT::OPENL)) {
                    log(Lengine::E804, *it);
                    return nullptr;
                }
            } else if(it->is(VT::COLON)) {
                stack.movi(5);
            } else if(it->is(VN::CLASS_BASE_CLASSES)) {
                stack.stay();
                if(!it->is(VT::OPENS,VT::OPENL)) {
                    log(Lengine::E804, *it);
                    return nullptr;
                }
            } else if(it->is(VT::OPENS)) {
                stack.movi(6);
            } else if(it->is(VT::OPENL)) {
                ret->predicates.construct(-1);
                stack.movi(8);
            } else if( it->is(VN::CLASS_PREDICATE) ) {
                stack.stay();
            } else {
                log(Lengine::E804, *it);
                return nullptr;
            } break;
        case 4:
            if(it->is(VT::LABEL)) {
                bool e = (string)ret->name == (string)*it;
                for (auto i = ret->tmpls.begin(); !e and i != ret->tmpls.end(); i++)
                    e = e or (string)*i == (string)*it;
                if(e) {
                    log(Lengine::E803, *it);
                    return nullptr;
                }
                ret->tmpls.construct(-1);
                ret->tmpls[-1] = *it;
                stack.stay();
            } else if(it->is(VT::COMMA)) {
                stack.stay();
                if(!it->is(VT::LABEL)) {
                    log(Lengine::E3, *it);
                    return nullptr;
                }
            } else if(it->is(VT::GT)) {
                if(ret->tmpls.size() == 0) {
                    log(Lengine::E401, *(it - 1), *it);
                    return nullptr;
                }
                stack.redu(1, VN::CLASS_TEMPLATE_PARAMLIST);
            } else {
                log(Lengine::E3, *it);
                return nullptr;
            } break;
        case 5:
            if(it->is(VT::OPENS , VT::OPENL)) {
                if(ret->supers.size() == 0) {
                    log(Lengine::E805, *it);
                    return nullptr;
                }
                stack.redu(-1, VN::CLASS_BASE_CLASSES);
            } else if(it->is(VN::NAMEUC)) {
                stack.stay();
            } else if(auto base = constructNameUseCase(it,log,ret,true); base) { //此处作用域选择上层作用域,目的是阻止基类从类内部搜索名称.
                ret->supers << base;
            } else {
                return nullptr;
            } break;
        case 6:
            if(it->is(VT::iINTEGERn)) {
                if(flag) {
                    log(Lengine::E201,*it);
                    return nullptr;
                }

                auto& tem = ret->branchs.construct(-1);
                tem.offset = wrtno;
                tem.count = 0;
                tem.index = *it;
                stack.stay();

                if(it->is(VT::COLON)) {
                    stack.stay();
                } else {
                    log(Lengine::E1007,*it);
                    return nullptr;
                }

                if( it->is(VT::OPENS ) ) {
                    flag = 2;
                    stack.movi(6);
                } else {
                    flag = 1;
                }
           } else if(it->is(VT::CLASS)) {
                auto cls = constructClassDefinition(it,log,ret,wrtno++);
                if(!cls) return nullptr;
                if((string)cls->name == (string)ret->name) {
                    log(Lengine::E806, cls->name);
                    return nullptr;
                }
                cls->wrtno = wrtno++;
                ret->internal << ($definition)cls;
            } else if(it->is(VT::ENUM)) {
                auto enm = constructEnumDefinition(it,log,ret,wrtno++);
                if(!enm) return nullptr;
                ret->internal << ($definition)enm;
                enm->setScope(ret);
            } else if(it->is(VT::METHOD)) {
                auto met = constructMethodDefinition(it,log,ret,wrtno++);
                if(!met) return nullptr;
                else if( met->meta ) ret->metadefs << ($definition)met;
                else ret->instdefs << ($definition)met;
                met->setScope(ret);
            } else if(it->is(VT::OPERATOR,VT::DELETE) ) {
                auto opd = constructOperatorDefinition(it,log,ret,wrtno++);
                if(!opd) return nullptr;
                else ret->instdefs << ($definition)opd;
                opd->setScope(ret);
            } else if(it->is(CT::ELETYPE)) {
                auto attr = constructAttributeDefinition(it,log,ret,wrtno++);
                if( !attr ) return nullptr;
                else if( attr->meta ) ret->metadefs << ($definition)attr;
                else ret->instdefs << ($definition)attr;
            } else if(it->is(VN::ATTRIBUTE,VN::METHOD, VN::CLASS, VN::ENUM,VN::OPERATOR)) {
                stack.stay();
                if( flag == 1 ) {
                    ret->branchs[-1].count = 1;
                    flag = 0;
                } else if( flag == 2 ) {
                    ret->branchs[-1].count += 1;
                }
            } else if(it->is(VT::CLOSES)) {
                if(flag == 2) stack.redu(1, VN::BRANCH);
                else stack.redu(4, VN::CLASS);
            } else if(it->is(VN::BRANCH)) {
                stack.stay();
                flag = 0;
            } else {
                log(Lengine::E201, *it);
                return nullptr;
            } break;
        case 7:
            if(it->is(VN::TYPEUC)) {
                stack.redu(4, VN::CLASS);
            } else if(auto alia = constructNameUseCase(it,log,scope,true); alia) {
                ret->alias = move(alia);
            } else {
                return nullptr;
            } break;
        case 8:
            if(it->is(VT::LABEL)) {
                auto index = -1;
                for( auto& tmpli : ret->tmpls ) if( (string)tmpli == (string)(*it) ) index = ret->tmpls.index(tmpli);
                if( index < 0 ) {
                    log(Lengine::E2023,*it);
                    return nullptr;
                }
                ret->predicates[-1].construct(-1);
                ret->predicates[-1][-1].index = index;
                stack.movi(9);
            } else if(it->is(VT::CLOSEL)) {
                if( ret->predicates[-1].size() == 0 ) {
                    log(Lengine::E1002,*it);
                    return nullptr;
                }
                stack.redu(1,VN::CLASS_PREDICATE);
            } else if(it->is(VN::EXPRESSION)) {
                stack.stay();
            } else {
                log(Lengine::E201,*it);
                return nullptr;
            } break;
        case 9:
            if(it->is(VT::NE)) {
                stack.movi(10);
            } else if(it->is(VT::EQ)) {
                stack.movi(11);
            } else if(it->is(VT::SHR)) {
                stack.movi(12);
            } else if(it->is(VT::LT)) {
                stack.movi(13);
            } else {
                log(Lengine::E1001,*it);
                return nullptr;
            } break;
        case 10:case 11:
            if( it->is(CT::ELETYPE) ) {
                ret->predicates[-1][-1].rule = (int)it->id - (int)VT::OBJ;
                ret->predicates[-1][-1].rule += ((int)(state)stack - 10)?5:1;
                stack.redu(2,VN::EXPRESSION);
            } else {
                log(Lengine::E1004,*it);
                return nullptr;
            } break;
        case 12:
            if( it->is(VT::OBJ,VT::PTR) ) {
                ret->predicates[-1][-1].rule = 9 + (int)it->id - (int)VT::OBJ;
                stack.redu(2,VN::EXPRESSION);
            } else {
                log(Lengine::E1006,*it);
                return nullptr;
            } break;
        case 13:
            if( it->is(VT::GT) ) {
                stack.stay();
                $typeuc& dt = ret->predicates[-1][-1].arg = constructDataType(it,log,ret,true);
                if( !dt or dt->is(typeuc::UnknownType) ) {
                    log(Lengine::E1005,*(it+1));
                    return nullptr;
                }
            } else if( it->is(VN::TYPEUC) ) {
                stack.redu(2,VN::EXPRESSION);
            } else {
                log(Lengine::E1005,*it);
                return nullptr;
            } break;
        }

    ret->phrase = *it;
    return ret;
}

$MethodDef Yengine::constructMethodDefinition( tokens::iterator& it, Lengine::logs& log, $scope scope, int wn ) {
    smachine stack = smachine(it);
    $MethodDef ret = new MethodDef;
    ret->wrtno = wn;
    ret->setScope(scope);

    if( !it->is(VT::METHOD) ) return nullptr;
    stack.movi(1);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1 : 
            if( it->is(VT::CONST) ) {
                if( ret->constraint ) {
                    log(Lengine::E302,*it);
                    return nullptr;
                } else if( ret->meta ) {
                    log(Lengine::E301,*it);
                    return nullptr;
                }
                ret->constraint = *it;
                stack.stay();
            } else if(it->is(VT::PUBLIC,VT::PRIVATE)) {
                if( ret->visibility ) {
                    if(ret->visibility.id == it->id) {
                        log(Lengine::E302, *it);
                        log(Lengine::E302, ret->visibility);
                    } else {
                        log(Lengine::E301, *it);
                        log(Lengine::E301, ret->visibility);
                    } 
                    return nullptr;
                }
                ret->visibility = *it;
                stack.stay();
            } else if(it->is(CT::MF_ATOMIC) and isalpha((it+1)->tx[0]) ){
                if( ret->atomic ) {
                    log(Lengine::E311,*it);
                    log(Lengine::E311,ret->atomic);
                    return nullptr;
                }
                ret->atomic = *it;
                stack.stay();
            } else if( it->is(VT::META) ) {
                if( ret->constraint ) {
                    log(Lengine::E301,*it);
                    log(Lengine::E301,ret->constraint);
                    return nullptr;
                } else if( ret->meta ) {
                    log(Lengine::E302,*it);
                    log(Lengine::E302,ret->meta);
                    return nullptr;
                }
                ret->meta = *it;
                stack.stay();
            } else if( it->is(VT::LABEL) ) {
                ret->name = *it;
                stack.movi(2);
            } else {
                log(Lengine::E303,*it);
                return nullptr;
            } break;
        case 2:
            if( it->is(VT::OPENA) ) {
                auto pars = constructParameterList(it,log,ret,*ret);
                if( !pars ) return nullptr;
            } else if( it->is(VN::PARAM_LIST) ) {
                stack.movi(3);
            } else {
                log(Lengine::E305,*it);
                return nullptr;
            } break;
        case 3:
            if( it->is(VN::PROTO) ) {
                stack.redu(3,VN::METHOD);
            } else if( it->is(VT::OBJ) ) {
                log(Lengine::E308,*it);
                return  nullptr;
            } else if( it->is(VT::VOID) ) {
                stack.redu(0,VN::PROTO);
            } else if( auto proto = constructElementPrototype(it,log,scope,true); proto ) {
                ret->rproto = move(proto);
            } else {
                return nullptr;
            } break;
    }

    ret->phrase = *it;
    return ret;
}

$AttrDef Yengine::constructAttributeDefinition( tokens::iterator& it, Lengine::logs& log, $ClassDef sc, int wn ) {
    smachine stack = it;
    $AttrDef ret = new AttrDef;
    ret->wrtno = wn;
    ret->setScope(sc);
    etype   ety;
    $typeuc dty;
    token   con;
    
    if(it->is(VT::OBJ)) {
        ety = OBJ;
    } else if(it->is(VT::PTR)) {
        ety = PTR;
    } else if(it->is(VT::REF)) {
        ety = REF;
    } else if(it->is(VT::REL)) {
        ety = REL;
    } else{
        log(Lengine::E4,*it);
        return nullptr;
    }
    stack.movi(1);
    while (stack.size() > 0)
        switch ((state)stack) {
            case 1:
                if(it->is(VT::CONST)) {
                    if(con.is(VT::CONST)) {
                        log(Lengine::E1008,*it);
                        return nullptr;
                    }
                    con = *it;
                    stack.stay();
                } else if(it->is(VT::META)) {
                    if(ret->meta.is(VT::META)) {
                        log(Lengine::E1009,*it);
                        return nullptr;
                    }
                    ret->meta = *it;
                    stack.stay();
                } else if(it->is(VT::LABEL)) {
                    ret->name = *it;
                    stack.movi(2);
                } else if(it->is(VT::PUBLIC,VT::PRIVATE)) {
                    if( ret->visibility ) {
                        if(ret->visibility.id == it->id) {
                            log(Lengine::E302, *it);
                            log(Lengine::E302, ret->visibility);
                        } else {
                            log(Lengine::E301, *it);
                            log(Lengine::E301, ret->visibility);
                        } 
                        return nullptr;
                    }
                    ret->visibility = *it;
                    stack.stay();
                } else {
                    log(Lengine::E5, *it );
                    return nullptr;
                } break;
            case 2:
                if( dty = constructDataType(it,log,ret,true); dty ) {
                    if(dty->is(typeuc::UnknownType)) {
                        log(Lengine::E1005,*(it+1));
                        return nullptr;
                    }
                    stack.redu(2,VN::ATTRIBUTE);
                } else {
                    return nullptr;
                } break;
        }
    ret->proto = eproto::MakeUp(ret,ety,dty,con);
    if(!ret->proto)
        return nullptr;
    
    ret->phrase = *it;
    return ret;
}

$OperatorDef Yengine::constructOperatorDefinition( tokens::iterator& it, Lengine::logs& log, $scope scope, int wn ) {
    smachine stack = it;
    $OperatorDef ret = new OperatorDef;
    ret->wrtno = wn;
    ret->setScope(scope);
    stack.movi(1,0);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1:
            if( it->is(VT::OPERATOR) ) {
                stack.movi(2);
            } else if( it->is(VT::DELETE) ) {
                ret->action = *it;
                stack.movi(5);
            } else {
                log(Lengine::E2035,*it);
                return nullptr;
            } break;
        case 2:
            if( it->is(VT::DEFAULT) ) {
                if( ret->action ) {
                    if( ret->action.id == it->id) {
                        log(Lengine::E302,ret->action);
                        log(Lengine::E302,*it);
                    } else {
                        log(Lengine::E301,ret->action);
                        log(Lengine::E301,*it);
                    }
                }
                ret->action = *it;
                stack.stay();
            } else if( it->is(CT::MF_REV,CT::MF_ISM,CT::PREFIX,CT::SUFFIX) ) {
                if( ret->modifier ) {
                    if( ret->modifier.id == it->id) {
                        log(Lengine::E302,ret->modifier);
                        log(Lengine::E302,*it);
                    } else {
                        log(Lengine::E301,ret->modifier);
                        log(Lengine::E301,*it);
                    }
                }
                ret->modifier = *it;
                stack.stay();
            } else if( it->is(VT::PUBLIC,VT::PRIVATE) ) {
                log(Lengine::E2034,*it);
                return nullptr;
            } else if( it->is(CT::OPL) ) {
                stack.movi(3);
            } else {
                auto name = constructOperatorLabel(it,log,ret->subtitle);
                if( !name ) return nullptr;
                ret->name = name;
            } break;
        case 3:
            if( constructParameterList(it,log,scope,*ret) ) {
                stack.movi(4);
            } else {
                return nullptr;
            } break;
        case 4:
            if( ret->name.is(VN::OPL_SCTOR,VN::OPL_LCTOR,VN::OPL_CCTOR,VN::OPL_MCTOR,VN::OPL_DTOR,VN::OPL_MOVE) ) {
                stack.redu(-4,VN::OPERATOR);
            } else if( it->is(VN::PROTO) ) {
                stack.redu(4,VN::OPERATOR);
            } else if( auto proto = constructElementPrototype(it,log,scope,true); proto ) {
                ret->rproto = proto;
            } else {
                return nullptr;
            } break;
        case 5:
            if( it->is(VT::DEFAULT) ) {
                stack.movi(6);
            } else {
                log(Lengine::E202,VT::DEFAULT,*it);
                return nullptr;
            } break;
        case 6:
            if( it->is(VT::OPERATOR) ) {
                stack.movi(7);
            } else {
                log(Lengine::E202,VT::OPERATOR,*it);
                return nullptr;
            } break;
        case 7:
            if( auto name = constructOperatorLabel(it,log,ret->subtitle); name ) {
                ret->name = name;
                stack.redu(4,VN::OPERATOR);
            } else {
                return nullptr;
            } break;
    }

    ret->phrase = *it;
    return ret;
}

$InsBlockImpl Yengine::constructInstructionBlockImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope ) {
    smachine stack = it;
    stack.movi(1,0);
    
    $InsBlockImpl ref = new InsBlockImpl;
    ref->setScope(scope);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1:
            if( it->is(VT::OPENS) ) {
                stack.movi(2);
            } else {
                log(Lengine::E201,*it);
                return nullptr;
            } break;
        case 2:
            if( it->is(VT::SEMI) ) {
                stack.stay();
            } else if( it->is(VT::CLOSES) ) {
                stack.redu(2,VN::BLOCK);
            } else if( it->is(CT::ELETYPE) ) {
                auto el = constructConstructImplementation(it,log,ref);
                if( el ) {
                    ref->impls << ($implementation)el;
                }
            } else if( it->is(VT::LOOP) ) {
                if( auto loop = constructLoopimplementation(it,log,ref) ; loop )
                    ref->impls << ($implementation) loop;
                else return nullptr;
            } else if( it->is(VT::SWITCH) ) {
                stack.stay();   //[TODO]
            } else if( it->is(VT::IF) ) {
                if( auto br = constructBranchImplementation(it,log,ref) ; br )
                    ref->impls << ($implementation) br;
                else return nullptr;
            } else if( it->is(VT::RETURN,VT::CONTINUE,VT::BREAK) ) {
                if( auto ctl = constructFlowControlImplementation(it,log,ref); ctl )
                    ref->impls << ($implementation)ctl;
                else
                    return nullptr;
            } else if( it->is(VN::CONTROL,VN::ELEMENT,VN::EXPRESSION,VN::BRANCH,VN::LOOP) ) {
                stack.stay();
            } else if( auto ex = constructExpressionImplementation(it,log,ref); ex ) {
                ref->impls << ($implementation)ex;
            } else {
                return nullptr;
            } break;
    }

    ref->phrase = *it;
    return ref;
}

$ExpressionImpl Yengine::constructExpressionImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope ) {
    smachine stack =it;
    $ExpressionImpl ret = new ExpressionImpl;
    stack.movi(1,0);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1 :
            if( it->is(VT::CLASS) ) {
                stack.stay();
                ret->name = constructNameUseCase(it,log,scope,true);
                if( !ret->name ) return nullptr;
            } else if( it->is(VT::LABEL) ) {
                ret->name = constructNameUseCase(it,log,scope,false);
                if( !ret->name ) return nullptr;
            } else if( it->is(VN::NAMEUC) ) {
                ret->type = ExpressionImpl::NAMEUSAGE;
                stack.redu(0,VN::EXPRESSION);
            } else if( it->is(CT::CONSTANT,VT::iTHIS)) {
                ret->type = ExpressionImpl::VALUE;
                ret->mean = *it;
                stack.redu(0,VN::EXPRESSION);
            } else if( it->is(CT::PREFIX) ) {
                ret->type = ExpressionImpl::PREFIX;
                ret->mean = *it;
                stack.movi(2);
            } else if( it->is(VT::OPENA) ) {
                stack.movi(3);
            } else if( it->is(VN::EXPRESSION) ) {
                stack.movi(5);
            } else {
                log(Lengine::E2025,*it);
                return nullptr;
            } break;
        case 2 :
            if( it->is(VN::EXPRESSION) ) {
                stack.redu(1,VN::EXPRESSION);
            } else if( auto sub = constructExpressionImplementation(it,log,scope); sub ) {
                ret->sub << sub;
            } else {
                return nullptr;
            } break;
        case 3 : 
            if( ret = constructExpressionImplementation(it,log,scope); ret ) {
                stack.movi(4);
            } else {
                return nullptr;
            } break;
        case 4 :
            if( it->is(VT::CLOSEA) ) {
                stack.redu(2,VN::EXPRESSION);
            } else {
                //[TODO]: 报错 ')' missing
                log(Lengine::E202,")",*it);
                return nullptr;
            } break;
        case 5 :
            if( it->is(CT::SUFFIX) ) {
                $ExpressionImpl nr = new ExpressionImpl;
                nr->type = ExpressionImpl::SUFFIX;
                nr->mean = *it;
                nr->sub << ret;
                ret->setScope(scope);
                ret = nr;
                stack.redu(1,VN::EXPRESSION);
            } else if( it->is(VT::AS,VT::TREAT) ) {
                return nullptr;
                //[TODO]: 分析类型转换
            } else if( it->is(VT::OPENL) ) {
                $ExpressionImpl nr = new ExpressionImpl;
                nr->type = ExpressionImpl::SUFFIX;
                nr->mean = *it;
                nr->sub << ret;
                ret->setScope(scope);
                ret = nr;
                stack.movi(10);
            } else if( it->is(CT::INFIX) ) {
                auto prev = prio(*(it-2));
                if( prev and prev < prio(*it)) {
                    stack.redu(-2,VN::EXPRESSION);
                } else if( it->is(VT::MEMBER) ) {
                    $ExpressionImpl nr = new ExpressionImpl;
                    nr->type = ExpressionImpl::MEMBER;
                    nr->sub << ret;
                    ret->setScope(scope);
                    ret = nr;
                    stack.movi(9);
                } else {
                    $ExpressionImpl nr = new ExpressionImpl;
                    nr->mean = *it;
                    nr->type = it->is(CT::ASSIGN)?ExpressionImpl::ASSIGN:ExpressionImpl::INFIX;
                    nr->sub << ret;
                    ret->setScope(scope);
                    ret = nr;
                    stack.movi(8);
                }
            } else if( it->is(VT::OPENA) ) {
                auto nr = new ExpressionImpl;
                nr->type = ExpressionImpl::CALL;
                nr->sub << ret;
                ret->setScope(scope);
                ret = nr;
                stack.movi(6);
            } else {
                stack.redu(-2,VN::EXPRESSION);
            } break;
        case 6 :
            if( it->is(VT::CLOSEA) ) {
                stack.redu(2,VN::EXPRESSION);
            } else if( it->is(VN::EXPRESSION) ) {
                stack.movi(7);
            } else if( auto sub = constructExpressionImplementation(it,log,scope); sub ) {
                ret->sub << sub;
            } else {
                return nullptr;
            } break;
        case 7 :
            if( it->is(VT::COMMA) ) {
                stack.stay();
                if( auto sub = constructExpressionImplementation(it,log,scope); sub ) ret->sub << sub;
                else return nullptr;
            } else if( it->is(VN::EXPRESSION) ) {
                stack.stay();
            } else if( it->is(VT::CLOSEA) ) {
                stack.redu(3,VN::EXPRESSION);
            } else {
                return nullptr;
            } break;
        case 8 : 
            if( it->is(VN::EXPRESSION) ) {
                stack.redu(2,VN::EXPRESSION);
            } else {
                ret->sub << constructExpressionImplementation( it, log, scope );
                if( !ret->sub[-1] ) return nullptr;
            } break;
        case 9 :
            if( it->is(VT::LABEL) ) {
                ret->mean = *it;
                stack.redu(2,VN::EXPRESSION);
            } else {
                log(Lengine::E2028,*it);
                return nullptr;
            } break;
        case 10:
            if( it->is(VN::EXPRESSION) ) {
                stack.movi(11);
            } else if( auto id = constructExpressionImplementation(it, log, scope ); id ) {
                ret->sub << id;
            } else {
                return nullptr;
            } break;
        case 11:
            if( it->is(VT::CLOSEL) ) {
                stack.redu(3,VN::EXPRESSION);
            } else {
                log(Lengine::E202,"]",*it);
                return nullptr;
            } break;
    }

    ret->setScope(scope);
    ret->phrase = *it;
    return ret;
}

$FlowCtrlImpl Yengine::constructFlowControlImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope ) {
    smachine stack = it;
    $FlowCtrlImpl ret = new FlowCtrlImpl;
    ret->setScope(scope);

    if( it->is(VT::BREAK) ) {
        ret->action = BREAK;
        stack.movi(1);
    } else if( it->is(VT::CONTINUE) ) {
        ret->action = CONTINUE;
        stack.movi(1);
    } else if( it->is(VT::RETURN) ) {
        ret->action = RETURN;
        stack.movi(2);
    } else {
        log(Lengine::E2023,*it);
        return nullptr;
    }

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1 :
            return nullptr; //][TODO]
        case 2 :
            if( it->is(VT::SEMI,VT::CLOSES) ) {
                stack.redu(-1,VN::CONTROL);
            } else if( auto ex = constructExpressionImplementation(it,log,scope); ex ) {
                ret->expr = ex;
                stack.redu(1,VN::CONTROL);
            } else {
                return nullptr;
            } break;
    }

    ret->phrase = *it;
    return ret;
}

$ConstructImpl Yengine::constructConstructImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope ) {
    smachine stack = it;
    etype et;
    token ct;
    $ConstructImpl ret = new ConstructImpl;
    ret->setScope(scope);

    stack.movi(1,0);
    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1:
            if( it->is(CT::ELETYPE) ) {
                et = (etype)(int)it->id;
                stack.movi(2);
            } else {
                log(Lengine::E4, *it );
                return nullptr;
            } break;
        case 2:
            if( it->is(VT::LABEL) ) {
                ret->name = *it;
                stack.movi(3);
            } else if( it->is(VT::CONST) ) {
                if( ct ) {
                    log(Lengine::E302, *it);
                    return nullptr;
                }
                ct = *it;
                stack.stay();
            } else {
                log(Lengine::E5, *it);
                return nullptr;
            } break;
        case 3:
            if( it->is(VN::TYPEUC) ) {
                stack.stay();
            } else if( it->is(VT::ASSIGN) ) {
                if( !ret->proto ) {
                    auto dt = constructDataType(it,log,scope,true);
                    if( !dt ) return nullptr;
                    ret->proto = eproto::MakeUp(scope,et,dt,ct);
                } else {
                    stack.movi(4);
                }
            } else if( it->is(VT::SEMI) ) {
                stack.redu(-3, VN::ELEMENT);
            } else if( ret->proto ) {
                log(Lengine::E202,";",*it);
                return nullptr;
            } else if( auto dt = constructDataType(it,log,scope,true); dt ) {
                ret->proto = eproto::MakeUp(scope,et,dt,ct);
            } else {
                return nullptr;
            } break;
        case 4:
            if( it->is(VN::EXPRESSION) ) {
                stack.redu(-4, VN::ELEMENT);
            } else if( auto ex = constructExpressionImplementation(it,log,scope); ex ) {
                ret->init = ex;
            } else {
                return nullptr;
            } break;
    }
    ret->phrase = *it;
    return ret;

}

$MethodImpl Yengine::constructMethodImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope ) {
    smachine stack = it;
    $MethodImpl ret = new MethodImpl;
    ret->setScope(scope);
    stack.movi(1,0);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1 :
            if( it->is(VT::METHOD) ) {
                stack.movi(2);
            } else {
                log(Lengine::E202,VT::METHOD,*it);
                return nullptr;
            } break;
        case 2 :
            if( it->is(VT::CONST) ) {
                if( ret->constraint ) {
                    log(Lengine::E302,*it);
                    log(Lengine::E302,ret->constraint);
                    return nullptr;
                }
                ret->constraint = *it;
                stack.stay(1);
            } else if( it->is(VT::LABEL) ) {
                ret->cname = constructNameUseCase( it, log, scope, true );
                if( !ret->cname ) return nullptr;
            } else if( it->is(VN::NAMEUC) ) {
                stack.movi(3);
            } else {
                log(Lengine::E309,*it);
                return nullptr;
            } break;
        case 3:
            if( it->is(VT::LABEL) ) {
                ret->name = *it;
                stack.movi(4);
            } else {
                log(Lengine::E310,*it);
                return nullptr;
            } break;
        case 4:
            if( it->is(VT::OPENA) ) {
                auto args = constructParameterList(it,log,ret,*ret);
                if( !args ) return nullptr;
            } else if( it->is(VN::PARAM_LIST) ) {
                stack.movi(5);
            } else {
                log(Lengine::E305,*it);
                return nullptr;
            } break;
        case 5:
            if( it->is(VT::OBJ) ) {
                log(Lengine::E308,*it);
                return nullptr;
            } else if( it->is(VT::VOID) ) {
                stack.redu(0,VN::PROTO);
            } else if( it->is(VN::PROTO) ) {
                stack.movi(6);
            } else if( auto proto = constructElementPrototype(it,log,ret,true); proto ) {
                ret->rproto = move(proto);
            } else {
                return nullptr;
            } break;
        case 6:
            if( it->is(VT::OPENS) ) {
                ret->body = constructInstructionBlockImplementation( it, log, ret );
                if( !ret->body ) return nullptr;
            } else if( it->is(VN::BLOCK) ) {
                stack.redu(6,VN::METHOD);
            } else {
                log(Lengine::E312,*it);
                return nullptr;
            } break;
    }

    ret->phrase = *it;
    return ret;
}

$BranchImpl Yengine::constructBranchImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope ) {
    smachine stack = it;
    $BranchImpl ret = new BranchImpl;
    ret->setScope(scope);

    stack.movi(1,0);
    while( stack.size() > 0 ) switch ( (state)stack ) {
        case 1:
            if( it->is(VT::IF) ) {
                stack.movi(2);
            } else {
                log(Lengine::E202, VT::IF,*it);
                return nullptr;
            } break;
        case 2:
            if( it->is(VT::OPENA) ) {
                stack.movi(3);
            } else {
                log(Lengine::E202, "(",*it);
                return nullptr;
            } break;
        case 3:
            if( it->is(VN::EXPRESSION) ) {
                stack.movi(4);
            } else if( auto e = constructExpressionImplementation(it, log, ret) ; e ) {
                ret->exp = e;
            } else {
                return nullptr;
            } break;
        case 4:
            if( it->is(VT::CLOSEA) ) {
                stack.movi(5);
            } else{
                log(Lengine::E202,")",*it);
                return nullptr;
            } break;
        case 5:
            if( it->is(CT::IMPLEMENTATION) ) {
                stack.movi(6);
            } else if( it->is(VT::IF) ) {
                auto br = constructBranchImplementation( it, log, ret );
                if( !br ) return nullptr;
                ret->first = br;
            } else if( it->is(VT::LOOP) ) {
                auto lp = constructLoopimplementation( it, log, ret );
                if( !lp ) return nullptr;
                ret->first = lp;
            } else if( it->is(VT::RETURN,VT::BREAK,VT::CONTINUE) ) {
                auto ct = constructFlowControlImplementation( it, log, ret );
                if( !ct ) return nullptr;
                ret->first = ct;
            } else if( it->is(VT::OPENS) ) {
                auto bk = constructInstructionBlockImplementation( it, log, ret );
                if( !bk ) return nullptr;
                ret->first = bk;
            } else if( auto ex = constructExpressionImplementation( it, log, ret ); ex ) {
                ret->first = ex;
            } else {
                return nullptr;
            } break;
        case 6:
            if( it->is(VT::ELSE) ) {
                stack.movi(7);
            } else {
                stack.redu(-6,VN::BRANCH);
            } break;
        case 7:
            if( it->is(CT::IMPLEMENTATION) ) {
                stack.redu(7,VN::BRANCH);
            } else if( it->is(VT::IF) ) {
                auto br = constructBranchImplementation( it, log, ret );
                if( !br ) return nullptr;
                ret->secnd = br;
            } else if( it->is(VT::LOOP) ) {
                auto lp = constructLoopimplementation( it, log, ret );
                if( !lp ) return nullptr;
                ret->secnd = lp;
            } else if( it->is(VT::RETURN,VT::BREAK,VT::CONTINUE) ) {
                auto ct = constructFlowControlImplementation( it, log, ret );
                if( !ct ) return nullptr;
                ret->secnd = ct;
            } else if( it->is(VT::OPENS) ) {
                auto bk = constructInstructionBlockImplementation( it, log, ret );
                if( !bk ) return nullptr;
                ret->secnd = bk;
            } else if( auto ex = constructExpressionImplementation( it, log, ret ); ex ) {
                ret->secnd = ex;
            } else {
                return nullptr;
            } break;
    }

    ret->phrase = *it;
    return ret;
}

$LoopImpl Yengine::constructLoopimplementation( tokens::iterator& it, Lengine::logs& log, $scope scope ) {
    smachine stack = it;
    $LoopImpl ret = new LoopImpl;
    ret->setScope(scope);
    stack.movi(1,0);

    while( stack.size() > 0 ) switch( (state)stack ) {
        case 1:
            if( it->is(VT::LOOP) ) {
                stack.movi(2);
            } else {
                log(Lengine::E202,VT::LOOP,*it);
                return nullptr;
            } break;
        case 2:
            if( it->is(VT::OPENA) ) {
                stack.movi(3);
            } else if( it->is( VN::EXPRESSION) ) {
                stack.movi(5);
            } else {
                stack.movi(0,0);
                stack.redu(-1,VN::EXPRESSION);
            } break;
        case 3:
            if( it->is(VN::EXPRESSION) ) {
                stack.movi(4);
            } else if( auto cond = constructExpressionImplementation(it,log,ret); cond ) {
                ret->cond = cond;
            } else {
                return nullptr;
            } break;
        case 4:
            if( it->is(VT::CLOSEA) ) {
                stack.redu( 2, VN::EXPRESSION );
            } else {
                log(Lengine::E202,")",*it);
                return nullptr;
            } break;
        case 5:
            if( it->is(CT::IMPLEMENTATION)) {
                stack.redu(3,VN::LOOP);
            } else if( it->is(VT::IF) ) {
                auto br = constructBranchImplementation(it,log,ret);
                if( !br ) return nullptr;
                ret->imp = br;
            } else if( it->is(VT::LOOP) ) {
                auto loop = constructLoopimplementation(it,log,ret);
                if( !loop ) return nullptr;
                ret->imp = loop;
            } else if( it->is(VT::RETURN,VT::CONTINUE,VT::BREAK) ) {
                auto flow = constructFlowControlImplementation(it,log,ret);
                if( !flow ) return nullptr;
                ret->imp = flow;
            } else if( it->is(VT::SEMI) ) {
                stack.redu(4,VN::LOOP);
            } else if( it->is(VT::OPENS) ) {
                auto ins = constructInstructionBlockImplementation(it,log,ret);
                if( !ins ) return nullptr;
                ret->imp = ins;
            } else if( auto exp = constructExpressionImplementation(it,log,ret) ; exp ) {
                ret->imp = exp;
            } else {
                return nullptr;
            } break;
        
        
    }
    ret->phrase = *it;
    return ret ;
}

int Yengine::prio(const token& it)const {
    int p = 0;  //[TODO]添加了好多运算符,这个方法待更新


    /**
     * 所有运算符
     * p越小,优先级越高
     */
    while( true ) {
        if( !it.is(CT::OPERATOR) ) break;
        p += 1;

        if( it.is(VT::MEMBER) )break;
        p += 1;
        //if( it.is(VN::INDEX) ) break;p += 1;                   //单目运算符    后
        if( it.is(VT::INCRESS,VT::DECRESS) ) break;
        p += 1;       //单目运算符    前后
        //if( it.is(VT::prePLUS, VT::preMINUS) ) break; 
        p += 1;    //单目运算符    前
        //if( it.is(VT::ADDRESS,VT::REFER) ) break;
        p += 1;         //单目运算符    前
        if( it.is(VT::NOT) ) break; 
        p += 1;                      //单目运算符    前
        if( it.is(VT::BITREV) ) break;
        p += 1;                      //单目运算符    前
        if( it.is(VT::SHL,VT::SHR) ) break;
        p += 1;               //双目运算符    左结合
        if( it.is(VT::BITAND) ) break;
        p += 1;                      //双目运算符    左结合
        if( it.is(VT::BITXOR) ) break;
        p += 1;
        if( it.is(VT::BITOR) ) break;
        p += 1;
        if( it.is(VT::MOL,VT::MUL,VT::DIV) ) break;
        p += 1;
        if( it.is(VT::PLUS,VT::MINUS) ) break;
        p += 1;
        //if( it.is(VT::AS) ) break;p += 1;
        if( it.is(VT::RANGE) ) break;
        p += 1;
        if( it.is(CT::RELATION) ) break;
        p += 1;
        if( it.is(VT::AND) ) break;
        p += 1;
        if( it.is(VT::OR) ) break;
        p += 1;
        if( it.is(CT::ASSIGN) ) break;
        p += 1;
        break;
    }
    return p;
}

}

#endif