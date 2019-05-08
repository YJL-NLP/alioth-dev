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
struct OperatorDef : public definition, public morpheme::plist {

    public:
        /**
         * @member subtitle : 副标题
         * @desc :
         *  有些运算符携带副标题
         */
        anything subtitle;

        /**
         * @member revers : 反向重载
         * @desc :
         *  此标志与same冲突，表示此重载是一个反向重载
         */
        token revers;

        /**
         * @member same : 正反同构
         * @desc :
         *  正反同构标志
         */
        token same;

    public:

        bool is( cnode ) const override;
};

using $OperatorDef = agent<OperatorDef>;

}

#endif