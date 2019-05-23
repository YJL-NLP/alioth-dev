#ifndef __constructorimpl__
#define __constructorimpl__

#include "implementation.hpp"
#include "expressionimpl.hpp"

namespace alioth {

/**
 * @struct ConstructorImpl : 构造器
 * @desc :
 *  构造器区别于构造语句
 *  构造器是构造运算符的实现体，包括结构化构造运算符，序列化构造运算符，拷贝构造运算符，移动构造运算符在内的构造运算符都使用构造器作为实现体。
 *  构造器由两部分构成
 *      构造列表
 *      初始化块
 *  构造列表对所有的成员的构造方法进行调用
 *  初始化块在构造结束之后被执行，给定this是当前正在构造的对象
 *  构造运算符不负责开辟空间
 */
struct ConstructorImpl : public implementation {

    public:
        struct ConstructInfo {
            nameuc name;
            $ExpressionImpl ctor;
        };

    public:
        /**
         * @member construct : 构造列表
         * @desc :
         *  构造列表对所有成员调用构造运算符，此时nameuc是成员名
         *  构造列表还调用其他构造运算符，进行代理构造，此时nameuc是自身类名
         *  构造列表还能调用基类构造运算符，进行预构造，此时nameuc是基类类名
         *  构造列表中，成员不能重复构造，语义分析时检查
         *  基类或自身最多只能调用一次代理构造或预构造，语义分析时检查
         */
        chainz<ConstructInfo> construct;

        /**
         * @member initiate : 初始化块
         * @desc :
         *  初始化块的内容在当前对象完全被构造之后被执行
         */
        implementations initiate;
    public:

        bool is( cnode ) const override;
};

using $ConstructorImpl = agent<ConstructorImpl>;

}
#endif