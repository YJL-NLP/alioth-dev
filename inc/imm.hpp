#ifndef __imm__
#define __imm__

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include "methoddef.hpp"
#include "eproto.hpp"

namespace alioth {
using namespace llvm;

/**
 * @struct imm : 立即单元
 * @desc :
 *  立即单元是表达式的计算产物
 *  它可能是立即对象,也可能是立即寻址
 *  比如一个元素本身,就是一个立即寻址
 *  一个基础数据类型的加法运算中间结果就是一个立即对象
 *  立即对象直接存储数据,也直接存储数据类型
 *  立即寻址存储地址,存储元素原型
 */
struct imm;
using $imm = agent<imm>;
struct imm : thing {

    public:
        enum immt { 
            ety, // v 是类实体的GV,属于address
            adr, // v 是元素,对于VAR和PTR存储了对象的地址Obj*, 对于REF和VAL存储了对象的指针的地址Obj**
            val, // v 是对象,对于VAR和PTR存储了对象本身Obj, 对于REF和VAL存储了对象的指针Obj*
            fun, // v 是函数
        };

    private:
        /**
         * @member t : 立即单元类型
         * @desc :
         *  此标记标定此立即单元是否为立即对象或其他类型的立即单元
         */
        immt t;
        /**
         * @member v : 地址或值
         * @desc :
         *  若此单元为立即寻址,v保存元素,元素的本质就是对象的地址
         *  此值可以用于产生load或store指令
         *  若此单元是立即对象,v保存对象
         *  若此对象为函数,则v实际上是函数入口GV
         */
        Value* v;

        /**
         * @member p : 元素原型或数据类型
         * @desc :
         *  若单元为立即寻址或立即对象,则其中存储元素原型eproto
         *  若单元为实体,则其中存储类定义ClassDef
         *  若单元为立即函数,则其中存储函数原型MethodDef
         */
        anything p;
    
    public:

        /**
         * @member h : 宿主
         * @desc :
         *  可选的,如果此对象表示成员运算的结果,h指向成员运算的宿主,避免重复计算
         */
        agent<imm> h;

    public:
        imm() = default;
        imm( immt T, Value* V, anything P, agent<imm> H = nullptr );
        imm( const imm& ) = default;
        imm( imm&& ) = default;
        imm& operator=( const imm& ) = default;
        imm& operator=( imm&& ) = default;
        ~imm() = default;

        static $imm address( Value* addr, $eproto proto, agent<imm> host = nullptr );
        static $imm object( Value* obj, $eproto proto );
        $eproto eproto()const;

        static $imm entity( Value* addr, $ClassDef def );
        $ClassDef metacls()const;
        
        
        static $imm function( Value* fp, $MethodDef prototype, agent<imm> host = nullptr );
        $MethodDef prototype()const;

        Value* raw()const;
        
        /** 
         * 获取可以直接参与运算的Value*
         * 1. 对于立即寻址,执行load 
         * 2. 对于引用和右值,再执行load
         * 3. 对于实体,执行load
         */
        Value* asobject( IRBuilder<>& builder )const;

        /**
         * 获取可以执行store存储对象的Value*
         * 1. 对于立即对象,REF和VAL,直接返回v,其他返回空
         * 2. 对于立即寻址,REF和VAL,执行load后返回,其他直接返回
         * 3. 对于实体,直接返回
         */
        Value* asaddress( IRBuilder<>& builder )const;

        /**
         * 获取可用于传参Value*
         * 1. 对立即寻址,执行load
         * 2. 对立即对象,直接返回
         * 3. 对实体,直接返回
         */
        Value* asparameter( IRBuilder<>& builder, $eproto req = nullptr )const;

        Value* asfunction()const;

};

using imms = chainz<$imm>;

}
#endif