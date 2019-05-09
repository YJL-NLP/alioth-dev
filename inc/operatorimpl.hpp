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

        /**
         * @member body : 运算符体
         * @desc :
         *  运算符体可能是ctor或一个InsBlockImpl
         */
        anything body;

    public:

        bool is( cnode ) const override;
};

using $OperatorImpl = agent<OperatorImpl>;

}

#endif