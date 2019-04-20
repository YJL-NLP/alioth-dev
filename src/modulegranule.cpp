#ifndef __modulegranule_cpp__
#define __modulegranule_cpp__

#include "modulegranule.hpp"
#include "modesc.hpp"
#include "manager.hpp"

namespace alioth {

bool ModuleGranule::is( cnode c ) const {
    return c == GRANULE;
}

std::string ModuleGranule::getDocPath() const {
    if( !desc or !desc->manager ) return "";
    return desc->manager->getDocumentEngine().getPath(document);
}

anything ModuleGranule::getGranule() {return this;}
Manager* ModuleGranule::getManager() {return desc?desc->manager:nullptr;}

}

#endif