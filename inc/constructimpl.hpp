#ifndef __constructimpl__
#define __constructimpl__

#include "implementation.hpp"
#include "expressionimpl.hpp"
#include "eproto.hpp"

namespace alioth {

/**
 * @struct ConstructImpl : element implementation --- 局部元素
 * @desc :
 *  用于表述局部元素的结构体
 *  局部元素是修改符号表的可执行语句
 */
struct ConstructImpl : public implementation {

    public:

        /**
         * @member name : 元素名
         * @desc :
         *  元素名是其他源代码部分索引局部元素的唯一方法
         */
        token name;

        /**
         * @member proto : 元素原型
         * @desc :
         *  元素原型中的数据类型可以为空,此时初始化表达式类型必须可导
         */
        $eproto proto;

        /**
         * @member init : 初始化表达式
         * @desc :
         *  可选的初始化表达式
         *  当元素原型中未指定数据类型时,初始化表达式必须存在
         */
        $ExpressionImpl init;

    public:
        bool is( cnode ) const override;

};

using $ConstructImpl = agent<ConstructImpl>;
using ConstructImpls = chainz<$ConstructImpl>;

}

#endif