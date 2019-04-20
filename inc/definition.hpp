#ifndef __definition__
#define __definition__

#include "node.hpp"

namespace alioth {

/**
 * @struct definition : 定义
 * @desc :
 *  描述所有携带定义语义的语法结构
 */
struct definition : public node {

    public:
        /**
         * @member visibility : 可见性
         * @desc :
         *  可见性修饰词
         *  此值若有效,则可以取值PUBLIC或PRIVATE
         */
        token   visibility;

        /**
         * @member name : 名称
         * @desc :
         *  所有定义都拥有名称
         */
        token   name;

        /**
         * @member wrtno : 书写次序
         * @decs :
         *  书写次序用于隔离定义的存储顺序与定义的书写次序
         *  保留书写次序的目的是支持定义分支
         */
        int wrtno;

    protected:

        /**
         * @constructor : 构造函数
         * @desc :
         *  提供默认的拷贝构造函数和移动构造函数
         */
        definition() = default;
        definition( const definition& ) = default;
        definition( definition&& ) = default;

        /**
         * @operator= : 赋值运算符
         * @desc :
         *  提供默认的拷贝赋值运算和移动赋值运算
         */
        definition& operator=( const definition& ) = default;
        definition& operator=( definition&& ) = default;
};

using $definition = agent<definition>;
using definitions = chainz<$definition>;

}

#endif