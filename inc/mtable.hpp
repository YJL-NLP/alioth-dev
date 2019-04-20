#ifndef __mtable__
#define __mtable__

#include "dengine.hpp"

namespace alioth {

/**
 * @struct mtable : module descriptor table --- 模块描述符表
 * @desc :
 *  [2019/03/30] : 尚未启用
 *  重新设计的模块描述符表用于隔离语法结构细节和Manager
 */
struct mtable {

    public:
        /**
         * @member fdesc : 文件描述符
         * @desc :
         *  文件描述符描述了模块描述符表所处的空间
         */
        Dengine::vfdm fdesc;

};

}

#endif