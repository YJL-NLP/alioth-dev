#ifndef __modulegranule__
#define __modulegranule__

#include "node.hpp"
#include "definition.hpp"
#include "implementation.hpp"
#include "modulesignature.hpp"

namespace alioth {

/**
 * @struct ModuleGranule : 模块颗粒
 * @desc :
 *  从单一源文档提取的语法树所包含的内容小于等于模块的全部内涵
 *  所以称之为模块颗粒,用于和其他颗粒共同构成完整的模块
 */
struct ModuleGranule : public node {

    public:
        /**
         * @member desc : 描述符
         * @desc :
         *  通过描述符指针与模块描述符保持连通
         */
        modesc* desc;
        
        /**
         * @member document : 源文档描述符
         * @desc :
         *  记录构建模块的源文档描述符
         */
        Dengine::vfdm document;

        /**
         * @member signature : 模块签名
         * @desc :
         *  这是每份源文档中的第一个有效语法结构
         *  确定源文档所描述的模块的名字
         *  描述模块对其他模块的依赖情况
         */
        $ModuleSignature signature;

        /**
         * @member defs : 定义语法结构
         */
        definitions defs;

        /**
         * @member impls : 实现语法结构
         */
        implementations impls;

    public:
        bool is( cnode c ) const override;

        std::string getDocPath() const override;
        anything getGranule()override;
        Manager* getManager()override;

};

using $ModuleGranule = agent<ModuleGranule>;

}

#endif