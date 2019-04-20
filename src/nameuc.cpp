#ifndef __nameuc_cpp__
#define __nameuc_cpp__

#include "nameuc.hpp"
#include "eproto.hpp"

namespace alioth {

nameuc::atom::operator bool()const {
    if( !name.is(VT::LABEL) ) return false;
    for( auto& t : tmpl ) if( !t ) return false;
    return true;
}

nameuc& nameuc::operator=( const atom& a ) {
    auto sa = a;
    msequence.clear();
    msequence.construct(-1, move(sa));
    return *this;
}

nameuc& nameuc::operator=( atom&& a ) {
    auto sa = a;
    msequence.clear();
    msequence.construct(-1, move(sa));
    return *this;
}

nameuc& nameuc::operator*=( const nameuc& a ) {
    msequence += a.msequence;
    return *this;
}

nameuc nameuc::operator*( const nameuc& a )const {
    nameuc r;
    r.msequence = msequence + a.msequence;
    return move(r);
}

nameuc::operator bool() const {
    if( msequence.size() == 0 ) return false;
    for( auto& i : msequence ) if( !i ) return false;
    return true;
}

nameuc::atom& nameuc::operator [] ( int index ) {
    return msequence[index];
}

nameuc::atom nameuc::operator[] ( int index )const {
    return msequence[index];
}

int nameuc::size()const {
    return msequence.size();
}

nameuc::operator int()const {
    return msequence.size();
}

nameuc nameuc::operator%( int n )const {
    nameuc ret = *this;
    int limit = (n < 0)?(-n):(msequence.size()-n); 
    while( ret.msequence.size() >0 and ret.msequence.size() > limit ) ret.msequence.remove(0);
    return move(ret);
}

nameuc& nameuc::operator%=( int n ) {
    int limit = (n < 0)?(-n):(msequence.size()-n); 
    while( msequence.size() >0 and msequence.size() > limit ) msequence.remove(0);
    return *this;
}

nameuc nameuc::operator/( int n )const {
    nameuc ret = *this;
    int limit = (n < 0)?(-n):(msequence.size()-n); 
    while( ret.msequence.size() >0 and ret.msequence.size() > limit ) ret.msequence.remove(-1);
    return move(ret);
}

nameuc& nameuc::operator/=( int n ) {
    int limit = (n < 0)?(-n):(msequence.size()-n); 
    while( msequence.size() >0 and msequence.size() > limit ) msequence.remove(-1);
    return *this;
}

void nameuc::setScope( $scope sc ) {
    mscope = sc;
    for( auto& a : msequence )
        for( auto t : a.tmpl )
            t->setScope(sc);
}

$scope nameuc::getScope() const {
    return mscope;
}

}

#endif