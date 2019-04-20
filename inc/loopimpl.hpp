#ifndef __loop__
#define __loop__

#include "implementation.hpp"
#include "expressionimpl.hpp"

namespace alioth{
    class LoopImpl : public implementation{
        public:
            $implementation imp;
            //TODO 现只考虑loop后为{}
            $ExpressionImpl cond;

        public:
            LoopImpl() = default;
            ~LoopImpl() = default;

            LoopImpl( const LoopImpl& ) = delete;
            LoopImpl( LoopImpl&& ) = delete;

            LoopImpl& operator=( const LoopImpl& ) = delete;
            LoopImpl& operator=( LoopImpl&& ) = delete;

            bool is ( cnode ) const override;
    };

    using $LoopImpl = agent <LoopImpl>;
}

#endif