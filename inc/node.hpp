#ifndef __node__
#define __node__

#include "alioth.hpp"
#include "agent.hpp"
#include <string>

namespace alioth {

struct node;
using $scope = node*;
using $node = agent<node>;

class Manager;

/**
 * @enum cnode : category of node --- 语法结构类别
 * @desc :
 *  用于将各种语法结构的身份表示到
 *  一个平坦的命名空间,简化语法结构
 *  身份识别的判断
 */
enum cnode {
    MODULE,
    DEPENDENCY,
    SIGNATURE,
    GRANULE,
    DEFINITION,
    IMPLEMENTATION,

    CLASSDEF,
    ENUMDEF,
    METHODDEF,
    OPERATORDEF,
    ATTRIBUTEDEF,

    METHODIMPL,
    CONSTRUCTIMPL,
    OPERATORIMPL,
    EXPRESSIONIMPL,
    CONTROLIMPL,
    BLOCKIMPL,
    BRANCHIMPL,
    LOOPIMPL
};

/**
 * @struct node : 语法树节点
 * @desc :
 *  任何语法结构,都是语法树的节点
 *  node结构体作为基类存在,使用
 *  统一的策略存储了语法树节点与子树
 *  根之间的关联关系.
 */
struct node : public thing {

    private:

        /**
         * @member mscope : 作用域
         * @desc :
         *  语法树结构中,语法树节点的父节点被视为
         *  此语法树节点的作用域
         *  [TODO]: 应当明确区分所有父节点与带有作用域语义
         *      的父节点之间的差别,使用"isscope"方法判断
         *      某父节点是否能用作作用域,来准确提取某节点的
         *      作为作用域存在的父节点.
         */
        $scope mscope;

    protected:

        /**
         * @explicit-constructor node : 显式构造函数
         * @desc :
         *  语法树节点在构造时必须知悉父节点
         * @param sc : 父节点,有时候,父节点也是语法结构所在的作用域
         */
        explicit node( $scope sc = nullptr );

    protected:

        /**
         * @constructor node : 构造方法
         * @desc :
         *  提供默认的构造方法用于拷贝和移动语境
         */
        node( const node& ) = default;
        node( node&& ) = default;

        /**
         * @operator = : 赋值运算符
         * @desc :
         *  提供默认的拷贝运算符
         */
        node& operator=( const node& ) = default;
        node& operator=( node&& ) = default;

    public:

        /**
         * @destructor node : 析构方法
         * @desc ;
         *  用于支持抽象类析构的虚方法
         */
        virtual ~node() = default;

        /**
         * @method is : 判断语法结构
         * @desc :
         *  用于判断语法结构身份的虚函数
         * @param type : 语法结构类型
         * @return bool : 语法结构是否与传参所表示的类型相符
         */
        virtual bool is( cnode type )const = 0;

        /**
         * @method setScope : 设置作用域
         * @desc :
         *  此方法仅在作用域无效时才成功
         *  也即无论通过何种途径,作用域只被设置一次
         *  此方法被设置为final虚函数是为了不接受重载
         * @param sc : 作用域
         * @return bool : 设置是否成功
         */
        virtual bool setScope( $scope sc )final;

        /**
         * @method getScope : 获取作用域
         * @desc :
         *  获取作用域
         * @return $scope : 作用域指针
         */
        virtual $scope getScope()const final;

        /**
         * @method getDocPath : 获取文档路径
         * @desc :
         *  此方法从当前语法找到模块语法结构
         *  从模块语法结构中提取文档路径.
         * @return string : 文档路径字符串
         */
        virtual std::string getDocPath()const;

        virtual anything getGranule();
        virtual anything getModule();
        virtual Manager* getManager();
};

}

#endif