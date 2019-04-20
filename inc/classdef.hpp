#ifndef __classdef__
#define __classdef__

#include "definition.hpp"
#include "nameuc.hpp"
#include "morpheme.hpp"

namespace alioth {

struct ClassDef;
using $ClassDef = agent<ClassDef>;
using ClassDefs = chainz<$ClassDef>;

/**
 * @class ClassDef : 概念
 * @desc :
 *  概念是特征的独特组合形成的知识单元.
 *  概念是逻辑思维的基本单位,个人认为更贴近问题描述的思维方式.
 */
struct ClassDef : public definition {

    public:
        /**
         * @member alias : 别名
         * @desc :
         *  ClassDef A = B; 的语法是别名定义
         */
        nameuc alias;

        /**
         * @member abstract : 抽象的
         * @desc :
         *  描述类是否为抽象类
         *  抽象类的所有方法都被实现之前不能被实例化
         *  抽象类的方法可以覆盖重载
         */
        token abstract;

        /**
         * @member tmpls : 模板参数列表
         * @desc :
         *  在模板用例中,模板参数列表应当被适当填充
         *  在其他类定义中,模板参数列表只是形参
         */
        chainz<token> tmpls;

        /**
         * @member targs : 模板参数实参
         * @desc :
         *  判断方法依据此成员是否为空来判断此语法结构是否为
         *  模板用例
         */
        chainz<$eproto> targs;

        /**
         * @member predicates : 谓词
         * @desc :
         *  谓词用于修饰模板类的模板参数列表
         *  描述对模板参数的限制性要求
         *  至少有一个谓词表达式成立时,模板类用例才能被创建
         */
        morpheme::preds predicates;

        /**
         * @member branchs : 定义分支
         * @desc :
         *  定义分支是标记,不是容器,以避免混乱
         */
        morpheme::dbras branchs;

        /**
         * @member supers : 超类列表
         * @desc :
         *  超类列表描述了类继承自哪些类
         *  类会按照顺序继承超类的布局,方法和运算符
         */
        chainz<nameuc> supers;

        /**
         * @member instdefs : 实例定义
         * @desc :
         *  实例定义包含了类的实例的属性和方法
         */
        definitions instdefs;

        /**
         * @member metadefs : 元定义
         * @desc :
         *  元定义包含了类的元属性和元方法
         */
        definitions metadefs;

        /**
         * @member internl : 内部定义
         * @desc :
         *  类中定义的类和枚举属于内部定义
         *  当前类对其所有的内部定义的成员拥有绝对访问权限 [2019/03/30 *new]
         */
        definitions internal;

        /**
         * @member usages : 用例容器
         * @desc :
         *  模板类的用例和模板类同名,如果直接放在同级作用域,会产生命名冲突
         *  所以模板类的用例被存储在模板类内,但逻辑上,模板类与其用例都存在
         *  于相同的作用域
         */
        ClassDefs usages;

    public:
        ClassDef() = default;
        ~ClassDef() = default;

        ClassDef( const ClassDef& ) = delete;
        ClassDef( ClassDef&& ) = delete;

        ClassDef& operator=( const ClassDef& ) = delete;
        ClassDef& operator=( ClassDef&& ) = delete;

        bool is( cnode ) const override;

};

}

#endif