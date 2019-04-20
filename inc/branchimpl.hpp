#ifndef __branch__
#define __branch__

#include "implementation.hpp"
#include "methoddef.hpp"
#include "expressionimpl.hpp"

namespace alioth{

class BranchImpl;
using   $BranchImpl = agent<BranchImpl>;

class BranchImpl : public implementation {
    public:
        $ExpressionImpl     exp;
        $implementation      first;
        $implementation      secnd;

    public:
        BranchImpl() = default;
        ~BranchImpl() = default;

        BranchImpl( const BranchImpl& ) = delete;
        BranchImpl( BranchImpl&& ) = delete;

        BranchImpl& operator =( const BranchImpl& ) = delete;
        BranchImpl& operator =( BranchImpl&& ) = delete;
        
        bool    is( cnode ) const override;
};

}



#endif