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
 * @struct tcp : Type Convert Path， 类型转换路径 
 * @desc :
 *  类型转换路径，记录两种类型之间的转换路径
 */
struct tcp : public thing { 

    public:
        const ConvertAction ca;
        const $typeuc dst,src;
        const agent<tcp> next;

    public: 
        tcp( ConvertAction c = Noneed, $typeuc d = nullptr, $typeuc s = nullptr,agent<tcp> n = nullptr):ca(c),dst(d),src(s),next(n){}
        tcp( const tcp& an ):tcp(an.ca,an.dst,an.src,an.next?new tcp(*an.next):nullptr) {}
        tcp( const tcp& an, agent<tcp> n ):tcp(an.ca,an.dst,an.src,n){}
        tcp( tcp&& an ) = delete;
        ~tcp() = default;

        tcp& operator=(const tcp&) = delete;
        tcp& operator=(tcp&&) = delete;
};

using $tcp = agent<tcp>; // tcp 代理

/** 
 * @struct TypeConvertDiagram : 类型转换图
 * @desc:
 *  记录所有可能存在的数据类型，以及它们之间可行的转换路径
 */

struct TypeConvertDiagram {

    public:
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
        chainz<$tcp> medge;

        /**
         * @member min,mout : 出入边缓冲
         * @desc :
         *  缓冲节点的出入边关系
         *  加快搜索速度
         */
        map<$typeuc,chainz<$tcp>> min;
        map<$typeuc,chainz<$tcp>> mout;

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
        map<tuple<$typeuc,$typeuc>,$tcp> mcachep;
};

}
#endif