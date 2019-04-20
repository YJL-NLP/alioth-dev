#ifndef __eproto__
#define __eproto__

#include "node.hpp"
#include "nameuc.hpp"
#include "lengine.hpp"
#include "llvm/IR/Type.h"

#include <vector>

namespace alioth {

struct ClassDef;
class dtype;
class eproto;
using $dtype = agent<dtype>;
using $eproto = agent<eproto>;
using $ClassDef = agent<ClassDef>;

using constraints = vector<bool>;

/**
 * @enum etype : 元素类型
 * @desc :
 *  元素类型只有可枚举的四种实际类型,外加一种待推导的空类型
 */
enum etype {
    UDF = (int)VT::SPACE,   //UDF用于没有指定元素类型的情况,根据数据类型可能被修正为其他类型
    VAR = (int)VT::VAR,
    PTR = (int)VT::PTR,
    REF = (int)VT::REF,
    VAL = (int)VT::VAL,
};

/**
 * @enum cdtype : category of data type 数据类型分类
 * @desc :
 *  由不同书写详细程度,数据类型语法结构可被分为三类
 */
enum cdtype {
    UNKNOWN,    //在条件允许的语境下,忽略数据类型不写,数据类型被暂定为UNKNOWN,在语义分析阶段进行推导
    BASIC,      //直接使用关键字将数据类型确定为基础数据类型
    NAMED,      //使用名称用例引用一个被定义过的数据类型
};

/**
 * @struct dtype : 描述数据类型的语法结构
 * @desc :
 *  数据类型并不算是能单独存在的语法结构
 *  但数据结构包含名称用例,名称用例的回溯依赖于作用域定位
 *  所以数据类型结构在语法树中必须存在一个锚定位置
 */
struct dtype : public thing {

    public:
        /**
         * @member cate : 数据类型类别
         * @desc :
         *  此成员决定了其他成员的有效性,及整个结构体的语义
         */
        cdtype      cate;

        /**
         * @member name : 命名数据类型名称
         * @desc :
         *  此成员就是数据类型必须成为一个语法结构的原因
         *  名称用例必须与一个语法树定位搭配才能正确分析其语义
         */
        nameuc      name;

        /**
         * @member basc : 基础数据类型关键字
         * @desc :
         *  当数据类型被标定为一个基础数据类型时此成员才有效
         */
        token       basc;

        /**
         * @member cons : 数据类型约束序列
         * @desc :
         *  约束序列一旦存在内容,就意味着数据类型是一个指针
         *  数据类型约束序列的每一项描述对应指针层次对其所绑定的对象的读写权限
         */
        constraints cons;
    
    public:

        /**
         * @constructor : 提供一系列默认构造方法
         */
        dtype( const dtype& ) = default;
        dtype( dtype&& ) = default;
        dtype() = default;
        ~dtype() = default;

        /**
         * @method load : 推导"加载"动作产生的数据类型
         * @desc :
         *  此方法模拟对指针指向的数据进行加载的动作,推导指针加载后形成的数据类型
         *  实质上就是减少n层指针约束
         * @param level : 要推导的加载层数,若超出实际存在的约束层次则失败
         * @return $dtype : 由加载动作产生的新的数据类型,与当前数据类型存在于相同的作用域
         */
        $dtype load( unsigned level = 1 ) const;

        /**
         * @method store : 推导"储存"动作产生的数据类型
         * @desc :
         *  添加一层指针约束
         * @param cst : 是否约束其写权限
         * @param csts : 一次执行一串存储推导
         * @return $dtype : 产生的新的数据类型,在同一作用域
         */
        $dtype store( bool cst = false ) const;
        $dtype store(const constraints& csts ) const;

        static $dtype MakeUp( $scope sc, const nameuc& name, const constraints& cons = constraints() );
        static $dtype MakeUp( $scope sc, const token& base, const constraints& cons = constraints() );

        /**
         * @method setScope : 设置作用域
         * @desc :
         *  数据类型内包含了名称用例,名称用例需要管理作用域
         */
        void setScope( $scope sc );
};

/**
 * @struct eproto : 元素原型
 * @desc :
 *  元素原型描述了一个元素除了名字以外所有的特性
 */
struct eproto : public thing {

    public:

        /**
         * @member type : 元素的元素类型
         */
        etype   elmt;

        /**
         * @member dtyp : 数据类型
         * @desc :
         *  元素的数据类型仅描述元素所绑定的对象的数据类型
         *  不描述元素本身的特性
         */
        $dtype  dtyp;

        /**
         * @member cons : 元素约束
         * @desc :
         *  实质上,元素就是所绑定对象的指针,所以对元素自身存在一个约束
         *  对于VAL和REF此标记被认为始终存在,因为语义上不能更换引用所绑定的对象
         */
        token   cons;

    public:

        /**
         * @constructor : 构造方法
         * @desc :
         *  提供各中默认构造方法
         */
        eproto( const eproto& ) = default;
        eproto( eproto&& ) = default;
        eproto() = default;
        ~eproto() = default;

        /**
         * @method MakeUp : 组装一个元素原型
         * @desc :
         *  根据已知信息组装一个元素原型
         *  此方法用于不能连续分析一个元素原型的语境
         *  比如参数定义,类属性定义等
         * @param sc : 作用域
         * @param tele : 元素类型
         * @param tdat : 数据类型
         * @param fconst : 元素约束
         * @return $eproto : 组装好的元素原型
         */
        static $eproto MakeUp( $scope sc, etype tele, $dtype tdat, const token& fconst = VT::R_ERR );

        /**
         * @method setScope : 设置作用域
         * @desc :
         *  为数据类型的内部名称设置作用域
         */
        void setScope( $scope sc );
};

}

#endif