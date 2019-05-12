#ifndef __operatorimpl__
#define __operatorimpl__

#include "implementation.hpp"
#include "insblockimpl.hpp"
#include "morpheme.hpp"

namespace alioth {

/**
 * @struct OperatorImpl : 运算符实现
 * @desc :
 *  运算符实现语法结构
 */
struct OperatorImpl : public implementation, public morpheme::plist {

    public:

        /**
         * @member cname : 宿主类名
         * @desc :
         *  运算符宿主的数据类型
         */
        nameuc cname;

        /**
         * @member name : 运算符标签
         * @desc :
         *  运算符标签确定运算符的结构
         */
        token name;

        /**
         * @member subtitle : 副标题
         * @desc :
         *  有些运算符携带副标题
         */
        token subtitle;

        /**
         * @member midifier : 修饰符
         * @desc:
         *  运算符可以携带的修饰符。
         */
        token modifier;

        /**
         * @member body : 运算符体
         * @desc :
         *  运算符体可能是ConstructorImpl或一个InsBlockImpl
         */
        anything body;

    public:

        bool is( cnode ) const override;
};

using $OperatorImpl = agent<OperatorImpl>;

}

#endif