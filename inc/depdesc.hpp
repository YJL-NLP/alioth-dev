#ifndef __depdesc__
#define __depdesc__

#include "node.hpp"
#include "chainz.hpp"
#include "token.hpp"
#include "lengine.hpp"

namespace alioth  {

class modesc;

using $modesc = agent<modesc>;

/**
 * @struct depdesc : 依赖描述符
 * @desc :
 *  依赖描述符用于描述依赖关系是模块签名语法结构的子结构
 *  同时,模块描述符也可以用于依赖关系闭包期间,和闭包之后
 *  用于关联相互依赖的模块
 */
struct depdesc : public node {
    public:
        /**
         * @member name : 模块名称
         * @desc :
         *  据本模块所知,依赖所指向的模块的名称
         */
        token       name;

        /**
         * @member alias : 依赖别名
         * @desc :
         *  依赖别名是指定依赖关系时对模块创建的别名
         *  用于解决需要依赖两个同名模块是出现的命名
         *  冲突
         */
        token       alias;

        /**
         * @member dest : 目标描述符
         * @desc :
         *  当依赖关系被确定后,可以将模块描述符以依赖关系
         *  绑定在一起,为后续跨越模块进行名称搜索作准备
         */
        $modesc     dest;

        /**
         * @member self : 自身
         * @desc :
         *  作为依赖描述符,在语法树开始构建之前就会被使用
         *  所以依赖描述符不能被设计为语法树节点,依赖描述符
         *  直接与所在模块的模块描述符绑定
         *  此成员至少应当在被追加到模块描述符时设置
         */
        $modesc     self;

        /**
         * @member from : 模块来源
         * @desc :
         *  据本模块所知,依赖所指向的模块来自哪个应用或哪个空间
         *  若此成员为"."则模块来自工作空间"Work"
         *  若此成员为"alioth"则模块来自根空间"Root"
         *  若此成员为空字符串,则根据"Work"->"Root"-"Apps"顺序搜索匹配的唯一的依赖
         *  若此成员为其他字符串,则首先搜索指定的应用空间,再搜索模块
         */
        token   mfrom;

    public:
        depdesc() = default;
        depdesc( const depdesc& ) = default;
        depdesc( depdesc&& ) = default;
        ~depdesc() = default;

        /**
         * @method literal : 字面符号
         * @desc : 返回其他语法结构希望引用此模块时
         *      需要使用的字面符号
         *      若存在别名,返回别名
         *      若不存在别名,返回模块名
         *      若别名是this关键字,返回VT::iTHIS
         *      表示要求此模块中的内容可以不带模块前缀使用,
         *          可以说是将此模块中的所有定义的定义域当成是本模块
         *              在符号查找时,模块描述符在本模块找不到符号,会向管理器请求符号路由,到所有别名为this的模块里寻找符号
         *          这种请求会使得在模块描述符将所有内容语法树完全扫描建立后,
         *          跨越模块检查全局名称冲突.
         * @return : 字面符号
         */
        token literal()const;

        /**
         * @method from : 字面from
         * @desc : 
         *      mfrom可以使用 一个点,或一个变量名,或一个单引号或双引号包括的名称
         *      其表达的意义都是一种规则.
         *      from的作用是从不同终结符中提取用户真正想要的应用名称
         * @param tr : 当此参数为true同时请求模块描述符成功时,依赖会被本地依赖"."可以被转换为应用名
         * @return string : 纯粹的应用名,没有双引号之类的包围
         */
        string from( bool tr );

        bool is( cnode ) const override;
};

using $depdesc = agent<depdesc>;
using depdescs = chainz<$depdesc>;

}
#endif