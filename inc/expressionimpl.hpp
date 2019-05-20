#ifndef __expressionimpl__
#define __expressionimpl__

#include "implementation.hpp"
#include "nameuc.hpp"
#include "eproto.hpp"

namespace alioth {

struct ExpressionImpl : public implementation {

    public:
        enum type {
            VALUE,      //值

            /** 若指向类,参数,元素,返回实体指针,若指向属性,返回GEP */
            /** NAMEUSAGE自然情况下会指向类,此时返回类的实体指针GV */
            /** 若指向参数,说明NAMEUSAGE只有一层,此时返回方法参数的地址 */
            /** 若指向元素,说明NAMEUSAGE只有一层,此时返回元素的地址 */
            /** 若指向元属性,则使用元属性所在的类的实体计算GEP, 实际上一定是基类 */
            /** 若指向实例属性,则使用实例this计算GEP,若方法为meta则失败 */
            /** 若指向方法,可能存在多个结果,返回GV,传入方法的this一定与当前宿主相同(不考虑多重载偏移的情况下) */
            NAMEUSAGE,  //名称用例

            /** 返回GEP结果,不执行load运算 */
            MEMBER,     //成员运算被特别对待

            /** 产生中间表示时,直接返回运算结果 */
            INFIX,      //中缀运算符引导的表达式
            SUFFIX,     //后缀运算符引导的表达式
            PREFIX,     //前缀运算符引导的表达式

            ASSIGN,     //赋值运算被单独处理

            /** 返回方法的运算结果 */
            CALL,       //调用

            LIST,       //列表
            SCTOR,      //结构化构造表达式
            LCTOR,      //序列化构造表达式
            LAMBDA,     //lambda表达式
            CONVERT,    //显式类型转换
            TREATE,     //强制类型转换
        };

    public:
        type   type;           // 表达式类型

        /**
         * @member mmean : 中心语
         * @desc : 当表达式为一下类型时,表达对应语义
         *  VALUE --- 值的词法记号
         *  MEMBER --- 成员名
         *  INFIX --- 中缀运算符
         *  SUFFIX --- 后缀运算符
         *  PREFIX --- 前缀运算符
         * TRANSFORM --- 选择类型转换的语义(as)或(as!)
         */
        token  mean;

        /**
         * @member mname : 名
         * @desc : 当表达式为以下类型时,表达对应语义
         *  NAMEUSAGE --- 名称用例
         *  SCONSTRUCT --- 类名
         *  LCONSTRUCT --- 类名
         */
        nameuc   name;

        /**
         * @member msub : 表达式内容
         * @desc : 当表达式为一下类型时,表达对应语义
         *  INFIX --- 包含左右算子
         *  SUFFIX --- 包含运算宿主
         *  PREFIX --- 包含运算宿主
         *  CALL --- 第一个成员是被调过程表达式,其余成员是形参
         *  LIST --- 包含构成列表的所有表达式
         *  SCONSTRUCT --- [尚未定义]
         *  LCONSTRUCT --- [尚未定义]
         *  TRANSFORM --- [要进行类型转换的表达式]
         */
        chainz<agent<ExpressionImpl>>  sub;

        /**
         * @member mtarget : 目标数据类型
         * @desc : 
         */
        $eproto  target;         //  mtype 为treat，convert时有效

    public:
        bool is( cnode ) const override;
        
};

using $ExpressionImpl = agent<ExpressionImpl>;

}

#endif