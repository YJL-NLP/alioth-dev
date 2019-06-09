#ifndef __stack__
#define __stack__

#include "imm.hpp"
#include "implementation.hpp"

namespace alioth {

struct StackSection {
    public:
        $implementation title;

        /**
         * @member elements : 元素表
         * @desc :
         *  记录局部元素与名称的对应关系
         */
        map<$ConstructImpl, $imm> elements;

        /**
         * @member instances : 实例
         * @desc :
         *  严格按照出现次序，将所有的需要析构的实例记录起来，包括元素
         */
        imms instances;
};


using ScopeStack = chainz<StackSection>;

}

#endif