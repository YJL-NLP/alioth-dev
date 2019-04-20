#ifndef __eproto_cpp__
#define __eproto_cpp__

#include "eproto.hpp"

namespace alioth {

$dtype dtype::load( unsigned level )const {
    if( cons.size() < level ) return nullptr;
    auto t = new dtype(*this);
    t->cons.erase(t->cons.begin(), t->cons.begin()+level-1);
    return move(t);
}

$dtype dtype::store( bool b )const {
    auto t = new dtype(*this);
    t->cons.insert(t->cons.begin(), b);
    return move(t);
}

$dtype dtype::store( const constraints& cons )const {
    auto t = new dtype(*this);
    t->cons.insert(t->cons.begin(),cons.begin(),cons.end());
    return move(t);
}

$dtype dtype::MakeUp( $scope sc, const nameuc& name, const constraints& cons ) {
    $dtype dt = new dtype;
    dt->cate = NAMED;
    dt->name = name;
    dt->cons = cons;
    dt->phrase = name.phrase;
    dt->setScope(sc);
    return dt;
}

$dtype dtype::MakeUp( $scope sc, const token& base, const constraints& cons ) {
    $dtype dt = new dtype;
    dt->cate = BASIC;
    dt->basc = base;
    dt->cons = cons;
    dt->phrase = base;
    dt->setScope(sc);
    return dt;
}

void dtype::setScope( $scope sc ) {
    name.setScope(sc);
}

$eproto eproto::MakeUp( $scope scope, etype tele, $dtype tdat, const token& fconst ) {
    if( !scope or !tdat ) return nullptr;
    $eproto ret = new eproto;
    ret->phrase = tdat->phrase;
    ret->cons = fconst;
    ret->dtyp = tdat;
    ret->elmt = tele==UDF?tdat->cons.size()>0?PTR:VAR:tele;
    ret->setScope(scope);
    return ret;
}

void eproto::setScope( $scope sc ) {
    if( dtyp ) dtyp->setScope(sc);
}

}

#endif