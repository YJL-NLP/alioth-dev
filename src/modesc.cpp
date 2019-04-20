#ifndef __modesc_cpp__
#define __modesc_cpp__

#include "modesc.hpp"
#include "manager.hpp"
#include "xengine.hpp"
#include "yengine.hpp"

namespace alioth {

modesc::modesc( Manager& mana, const string& pname, const string& papp ):
  program(papp.empty()?mana.getAppName():papp),
  name(pname),
  manager(&mana){
}

modesc::~modesc() {

}

bool modesc::appendDocument( Dengine::vfd desc ) {
    desc.space &= 0x00FF;
    desc.app = program;
    for( auto& doc : mdocs ) 
        if( desc == doc ) return false;
    mdocs << desc;
    return true;
}

Dengine::vfds modesc::getDocuments(bool tr) const {
    Dengine::vfds vfds = mdocs;
    if( tr ) {
        auto s = program=="alioth"?Root:program==manager->getAppName()?Work:Apps;
        for( auto& vfd : vfds ) vfd.space |= s;
    }
    return move(vfds);
}

modescs::modescs():mtim(0){}
modescs::modescs( const modescs& an):chainz(an),aname(an.aname),mtim(an.mtim) {}
modescs::modescs( modescs&& an ):chainz(an),aname(an.aname),mtim(an.mtim) {}

modescs& modescs::operator=( const modescs& an ) {
    chainz::operator=(an);
    aname = an.aname;
    mtim = an.mtim;
    return *this;
}
modescs& modescs::operator=( modescs&& an ) {
    chainz::operator=(an);
    aname = std::move(an.aname);
    mtim = std::move(an.mtim);
    return *this;
}

bool modesc::constructAbstractSyntaxTree( Lengine::logr& log ) {
    if( mdocs.size() == 0 ) return false;
    bool error = false;
    Xengine xeng;
    Yengine yeng;
    auto& dengine = manager->getDocumentEngine();
    
    for( auto& doc : getDocuments() ) {
        auto is = dengine.getIs(doc);
        auto path = dengine.getPath(doc);
        auto& lo = log.construct(-1,path);
        if( !is ) {
            lo(Lengine::E107,path,name,program);
            error = true;
            continue;
        }
        
        auto ts = xeng.parseSourceCode(*is);
        auto ref = yeng.constructSyntaxTree(ts,lo);
        if( ref == nullptr ) {error = true;continue;}

        ref->desc = this;
        ref->document = doc;
        syntrees << ref;
    }

    return !error;
}

}
#endif