#ifndef __eproto__
#define __eproto__

#include "node.hpp"
#include "nameuc.hpp"
#include "typeuc.hpp"
#include "lengine.hpp"
#include "llvm/IR/Type.h"

#include <vector>

namespace alioth {

struct ClassDef;
class eproto;
using $eproto = agent<eproto>;
using $ClassDef = agent<ClassDef>;

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
        $typeuc  dtype;

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
        static $eproto MakeUp( $scope sc, etype tele, $typeuc tdat, const token& fconst = VT::R_ERR );

        /**
         * @method setScope : 设置作用域
         * @desc :
         *  为数据类型的内部名称设置作用域
         */
        void setScope( $scope sc );
};

}

#endif