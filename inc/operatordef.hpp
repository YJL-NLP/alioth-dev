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
        token subtitle;

        /**
         * @member rproto : 返回值元素原型
         * @desc :
         *  描述运算符的返回值元素原型
         *  并不是所有的运算符都具有返回值，此时返回值原型是nullptr
         */
        $eproto rproto;

        /**
         * @member modifier : 修饰
         * @desc :
         *  运算符定义可以携带一个修饰符
         *  修饰符的取值范围: {rev,ism,prefix,suffix}
         */
        token modifier;

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