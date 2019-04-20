#ifndef __alioth__
#define __alioth__

/**
 * @namespace alioth : alioth命名空间
 * @desc :
 *  开发alioth编程语言所需的所有部件都部署在alioth命名空间中
 *  alioth开发组件中的类,主要分为信息载体和引擎两类.
 *  每个引擎都是一个模块的主体,实现了众多核心功能.
 *  而每个信息载体,是各个模块间沟通的基本单位,为了方便承载和共享信息
 *  通常设计为开放成员的形式,并仅包含一些简单的功能用于简化开发
 */

#include "chainz.hpp"
#include <tuple>

namespace alioth {

#define __alioth_ver__  std::tuple<int,int>(0,8)
#define __alioth_ver_str__ "0.8"
#define __aliothc_ver__ std::tuple<int,int>(0,1)
#define __aliothc_ver_str__ "0.1"

/**
 * @function null : 检查指针是否为空
 * @desc :
 *  检查一个指针是否为空
 *  当Jsonz等模块找不到对象时,它们并不抛出异常
 *  而是返回一个地址为空的对象引用
 *  对这些方法的返回值直接检查地址,会引起编译器的警告
 *  但是这些检查实际上是有意义的,所以使用null方法来抹除警告
 */
inline bool null( const void* p ) { return p == nullptr; }

}

#endif