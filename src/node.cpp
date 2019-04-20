#ifndef __node_cpp__
#define __node_cpp__

#include "node.hpp"
#include "modulegranule.hpp"

namespace alioth {

node::node( $scope sc ):mscope(sc) {}

bool node::setScope( $scope sc ) {
    if( mscope ) return false;
    mscope = sc;
    return true;
}

$scope node::getScope()const {
    return mscope;
}

std::string node::getDocPath()const {return mscope?mscope->getDocPath():"";}
anything node::getGranule() {return mscope?mscope->getGranule():nullptr;}
anything node::getModule() {return mscope?mscope->getModule():nullptr;}
Manager* node::getManager() {return mscope?mscope->getManager():nullptr;}

}

#endif