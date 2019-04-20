#ifndef __nameuc__
#define __nameuc__

/**
 * nameuc 名称用例
 * 
 * sname可能是对定义域名的呼唤,也可能是对一个定义的呼唤.
 * 它可能出现在模板参数列表中,基类列表,表达式中.
 */

#include "node.hpp"
#include "agent.hpp"
#include "token.hpp"
#include "lengine.hpp"

namespace alioth {

class eproto;
using $eproto = agent<eproto>;
using eprotos = chainz<$eproto>;

/**
 * @class nameuc: name use case --- 名称用例
 * @desc : 名称呼唤可以用于作为数据类型的一部分
 *      其更重要的作用是作为各种表达式的重要成分存在.
 *      nameuc可以用于呼唤类定义,方法定义,枚举定义,元素
 * 
 *      理论上,name成员只能为LABEL终结符,但是
 *      为了适应llvm的no-exception特性,此处不
 *      进行检查,使用者应当注意此成员的有效性
 * 
 *      当名称作为一种语法结果,名称的作用域字段应该保存
 *      最靠近名称的作用域的引用.
 *      因为这个作用域影响的不是名称中由(::)链接的名称,这些名称会被request和find正确处理.
 *      这个作用域影响的是名称的第一层以及整串名称所有的模板参数,这些内容的搜索开端应当从书写
 *      作用域开始,不能受到(::)的影响.
 */
class nameuc : public thing {

    public:
        friend class Yengine;

    public:
        struct atom {
            token   name;       //名称
            eprotos tmpl;       //模板参数
            
            atom() = default;
            atom(const atom& ) = default;
            atom(atom&& ) = default;
            ~atom() = default;

            template<typename ...Args>
            atom( token pname, Args ... args ):name(pname) {
                ( tmpl.construct(args), ... );
            }

            /**
             * @operator bool : 判断原子是否有效
             * @desc : 有效的原子,必须拥有LABEL作为name
             */
            operator bool() const;
        };
    private:
        chainz<atom>        msequence;  //名称序列
        $scope              mscope;     //作用域

    public:
        template<typename T,typename ...Args> nameuc( T&& a, Args&& ... args ) { msequence.construct(-1,forward<T>(a)) , ( msequence.construct(forward<Args>(args)), ... ); }
        nameuc() = default;
        nameuc(const nameuc& ) = default;
        nameuc(nameuc&&) = default;
        ~nameuc() = default;

        nameuc& operator=(const nameuc&) = default;
        nameuc& operator=(nameuc&&) = default;
        nameuc& operator=(const atom&);
        nameuc& operator=(atom&&);

        /**
         * @operator bool : 判断语义名称是否有效
         * @desc : 当语义名称中的每个原子片段都有效时,语义名称才有效
         *  同时,语义名称也不能为空
         */
        operator bool()const;

        /**
         * @operator [] : 取名称的中段某个原子
         */
        atom& operator [] (int index);
        atom operator [] ( int index)const;

        /**
         * @method size : 求名称长度
         */
        int size()const;
        explicit operator int()const;

        /**
         * @operator / : 去掉n层后缀,或去掉后缀直到长度不大于-n为止
         */
        nameuc operator / ( int n )const;
        nameuc& operator /= (int n );

        /**
         * @operator % : 去掉n层前缀,或去掉前缀直到长度不大于-n为止
         */
        nameuc operator % ( int n )const;
        nameuc& operator %= (int n );

        /**
         * @operator * : 链接名称
         */
        nameuc operator*(const nameuc& )const;
        nameuc& operator*=(const nameuc& );

        /**
         * @method setScope : 设置作用域
         * @desc :
         *  此方法为名称用例中的所有成分设置作用域
         *  不同于语法结构,名称用例的作用域设置是覆盖式的
         *  不考虑原值是否有效
         * @param sc : 作用域
         */
        void setScope( $scope sc );

        /**
         * @method getScope : 获取作用域
         * @desc :
         *  提供获取作用域的方法
         * @return $scope : 作用域
         */
        $scope getScope()const;
};

}

#endif