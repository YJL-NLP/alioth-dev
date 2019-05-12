#ifndef __methoddef__
#define __methoddef__

#include "definition.hpp"
#include "morpheme.hpp"

namespace alioth {

/**
 * @class MethodDef : method definition 方法定义
 * @desc :
 *  记录方法定义的语法结构
 */
struct MethodDef : public definition, public morpheme::plist {

    public:

        /**
         * @member rproto : 返回值元素原型
         * @desc :
         *  描述方法的返回值元素原型
         */
        $eproto rproto;

        /**
         * @member constraint : 约束
         * @desc :
         *  若此成员有效,则方法的this元素的原型是ref const
         */
        token constraint;

        /**
         * @member meta : 元方法标记
         * @desc :
         *  标记方法为元方法
         *  元方法是属于类而非对象的方法
         */
        token meta;

        /**
         * @member atomic : 同步标记
         * @desc :
         *  若此成员有效,则方法成为原子操作,或称同步方法.
         */
        token atomic;
    
    public:

        bool is( cnode ) const override;
        
};

using $MethodDef = agent<MethodDef>;

}

#endif