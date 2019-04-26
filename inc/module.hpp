#ifndef __module__
#define __module__

/**
 * [2019/03/31 : 尚未启用]
 */
#include "modesc.hpp"

namespace alioth {

/**
 * @module : 模块
 * @desc :
 *  依照语义组织模块语法结构
 */
struct module : public ClassDef {

    public:

        /**
         * @member desc : 模块描述符
         * @desc :
         *  链接模块与模块描述符
         *  确保语法结构不会消失
         */
        $modesc desc;

        /**
         * @member external : 外源定义
         * @desc :
         *  通过语法树根合并融合进来的外源定义
         *  在语义分析阶段被写入
         */
        definitions external;

        /**
         * @member extmeta : 外部元定义
         * @desc :
         *  通过语法树根合并融合进来的其他模块的元定义
         */
        definitions extmeta;

        /**
         * @member mimpls : 实现列表
         * @desc :
         *  从每个语法树收集而来的实现语法结构
         */
        implementations impls;

        /**
         * @member entry : 包含入口方法名的模块签名
         * @desc :
         *  至多只能出现一次的模块签名中的入口标记
         */
        $ModuleSignature es;

    public:

        module() = default;
        ~module() = default;

        module( const module& ) = delete;
        module( module&& ) = delete;

        module& operator=( const module& ) = delete;
        module& operator=( module&& ) = delete;

        bool is( cnode )const override;

        anything getModule() override;
};

using $module = agent<module>;

}

#endif