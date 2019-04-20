#ifndef __flowctrlimpl__
#define __flowctrlimpl__

#include "implementation.hpp"
#include "expressionimpl.hpp"

namespace alioth {

/**
 * @enum flwact : flow action --- 流动作
 */
enum flwact {
    BREAK,
    CONTINUE,
    RETURN
};

/**
 * @struct FlowCtrlImpl : flow control implementation --- 流控制
 * @desc :
 *  用于表述一种流控制语句
 */
struct FlowCtrlImpl : public implementation {

    public:
        /**
         * @member action : 流动作
         * @desc :
         *  此成员描述此流控制的动作是什么
         */
        flwact action;

        /**
         * @member label : 标记
         * @desc :
         *  若此成员有效,则流控制锚定标记所在的流层次
         */
        token   label;

        /**
         * @member expr : 表达式
         * @desc :
         *  当流控制为一个返回动作时,此成员可以用于容纳返回值表达式
         */
        $ExpressionImpl expr;

    public:
        bool is( cnode ) const override;
        
};

using $FlowCtrlImpl = agent<FlowCtrlImpl>;

}

#endif