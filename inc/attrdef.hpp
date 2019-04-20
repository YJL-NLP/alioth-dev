#ifndef __attrdef__
#define __attrdef__

#include "definition.hpp"
#include "eproto.hpp"

namespace alioth {

struct AttrDef : public definition {

    public:
        /**
         * @member meta : 元标记
         * @desc :
         *  若此成员有效,则属性被视为元属性
         */
        token meta;

        /**
         * @member proto : 元素原型
         * @desc :
         *  属性的元素原型
         *  元素原型的作用是描述从类解引用使用属性时的访问权限
         *  实际上所有的属性都被存储为对象
         */
        $eproto proto;

    public:

        bool is( cnode ) const override;
    
};

using $AttrDef = agent<AttrDef>;
using AttrDefs = chainz<$AttrDef>;

}
#endif