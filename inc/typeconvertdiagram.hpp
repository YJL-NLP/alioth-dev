#ifndef __typeconvertdiagram__
#define __typeconvertdiagram__

#include "typeuc.hpp"

namespace alioth {

/** 
 * @enum ConvertAction :类型转换动作 
 * @desc:
 *  描述类型转换所需的动作
 */
enum ConvertAction{ 
    Nocando, 
    Noneed, 
    Callctor, 
    Callasop, 
    Truncate, 
    Extend，
};

/** 
 * @struct dcp : TypeType Convert Path， 类型转换路径 
 * @desc :
 *  类型转换路径，记录两种类型之间的转换路径
 */
struct dcp : public thing { 

    public: 
        const ConvertAction ca;
        const $typeuc dst,src;
        const agent<dcp> next;

    public: 
        dcp( ConvertAction c = Noneed, $typeuc d = nullptr, $typeuc s = nullptr,agent<dcp> n = nullptr):ca(c),dst(d),src(s),next(n){}
        dcp( const dcp& an ):dcp(an.ca,an.dst,an.src,an.next?new dcp(*an.next):nullptr) {}
        dcp( const dcp& an, agent<dcp> n ):dcp(an.ca,an.dst,an.src,n){}
        dcp( dcp&& an ) = delete;
        ~dcp() = default;

        dcp& operator=(const dcp&) = delete;
        dcp& operator=(dcp&&) = delete;
};

using $dcp = agent<dcp>; // dcp 代理

/** 
 * @class TypeConvertDiagram : 类型转换图
 * @desc:
 *  记录所有可能存在的数据类型，以及它们之间可行的转换路径
 */

class TypeConvertDiagram {

    private:
        /**
         * @member mnode : 节点
         * @desc :
         *  节点是图的主要构成部分之一
         *  此处存储的所有节点都不包含有效的pharse,并且不存在于语法树中
         *  其作用是仅用于搜索转换路径
         * 
         *  由于节点实际上并没有什么太有效的索引，所以使用平坦结构存储
         */
        chainz<$typeuc> mnode;

        /**
         * @member medge : 边
         * @desc :
         *  边是图的主要构成部分之一
         *  此处存储的边都是孤立的链表节点，不能用作链
         */
        chainz<$dcp> medge;

        /**
         * @member mcachen : 节点缓冲
         * @desc :
         *  此容器用于缓冲已经出现过的数据类型和模板节点的对应关系。
         *  避免每次搜索之前都要先查询数据类型对应的节点
         */
        map<$typeuc,$typeuc> mcahcen;

        /**
         * @member mcachep : 路径缓冲
         * @desc :
         *  此容器缓冲已经被搜索过的两个节点之间的最短路径
         *  请不要在其他位置修改这些路径的信息，那会影响到全局范围
         */
        map<tuple<$typeuc,$typeuc>,$dcp> mcachep;

    public:
        TypeConvertDiagram();
};

}
#endif