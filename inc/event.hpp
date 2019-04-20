#ifndef __event__
#define __event__

#include "agent.hpp"

namespace alioth {

class stmt;
using scope_ref = stmt*;

enum Event {

    /**
     * @event NEW_STRUCTURE : 产生了新的语法结构
     * @desc : 管理器(Manager)应当对此事件感兴趣.
     *  当有新的语法结果在语义分析阶段产生时,它有可能被
     *  正在执行的语义分析流程绕过.所以管理器有责任对所有
     *  动态产生的语法结构进行特指的语义分析.
     * @where : 新创建的语法结果
     * @what : none
     */
    NEW_STRUCTURE,
};

struct event {

    Event       when;
    scope_ref   where;
    everything  what;
};

}

#endif