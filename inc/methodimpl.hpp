#ifndef __methodimpl__
#define __methodimpl__

#include "implementation.hpp"
#include "insblockimpl.hpp"
#include "morpheme.hpp"

namespace alioth {

/**
 * @class MethodImpl : method implementation 方法实现体
 * @desc :
 *  方法实现体通过方法签名与方法定义锚定
 *  将方法的算法实现内容与方法定义所表示的入口地址绑定
 */
struct MethodImpl : public implementation, public morpheme::plist {

    public:

        /**
         * @member funame : 方法全名
         * @desc :
         *  方法全名用于将方法实体与方法定义关联在一起
         */
        nameuc funame;

        /**
         * @member rproto : 返回值元素原型
         * @desc :
         *  描述方法返回值元素原型
         */
        $eproto rproto;

        /**
         * @member constraint : 约束
         * @desc :
         *  若此成员有效则方法为实例方法,且this被约束为ref const
         */
        token constraint;

        /**
         * @member body : 方法体
         * @desc :
         *  方法体是一个大指令块
         */
        $InsBlockImpl body;
    
    public:

        bool is( cnode ) const override;
        
};

using $MethodImpl = agent<MethodImpl>;

}

#endif