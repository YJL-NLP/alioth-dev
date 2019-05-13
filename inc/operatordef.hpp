#ifndef __operatordef__
#define __operatordef__

#include "definition.hpp"
#include "morpheme.hpp"

namespace alioth {

/**
 * @struct OperatorDef : 运算符定义
 * @desc :
 *  用于表述运算符定义
 */
struct OperatorDef : public definition, public morpheme::plist, public morpheme::opsig {

    public:
        /**
         * @member action : 动作标记
         * @desc :
         *  可以通过动作标记指定编译器的动作，delete或default
         */
        token action;

    public:

        bool is( cnode ) const override;
};

using $OperatorDef = agent<OperatorDef>;

}

#endif