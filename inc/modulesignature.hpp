#ifndef __ModuleSignature__
#define __ModuleSignature__

#include "node.hpp"
#include "depdesc.hpp"
#include "dengine.hpp"

namespace alioth {

class modesc;

/**
 * @struct ModuleSignature : 模块签名
 * @desc :
 *  模块签名作为源文档,从源文档中解析出来的语法结构,模块描述符三者之间的桥梁而存在
 *  它来自源文档中的模块签名语法结构,作为从语法结构出发自下而上搜索时最先被检索到的虚根而存在
 */
struct ModuleSignature : public node {

    public:
        /**
         * @member name : 模块名称
         * @desc :
         *  记录模块的名称
         */
        const std::string name;

        /**
         * @member entry : 入口符号
         * @desc :
         *  可选的，记录入口符号
         */
        token entry;

        /**
         * @member deps : 依赖
         * @desc :
         *  描述模块对外界的依赖情况
         */
        depdescs deps;

    public:
        ModuleSignature( const std::string& pname );
        bool is( cnode type )const override;
};

using $ModuleSignature = agent<ModuleSignature>;

}

#endif