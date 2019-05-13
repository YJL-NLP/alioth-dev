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
struct OperatorImpl : public implementation, public morpheme::plist, public morpheme::opsig {

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