#ifndef __modesc__
#define __modesc__

#include "agent.hpp"
#include "classdef.hpp"
#include "modulesignature.hpp"
#include "modulegranule.hpp"

namespace alioth {

class Manager;

/**
 * @class modesc : 描述符
 * @desc :
 *  描述符用于管理同属于一个模块的所有语法结构
 *  是编译为可执行文件的最小单位
 *  在依赖分析阶段,描述符也充当占位符
 */
class modesc : public thing {

    public:

        /**
         * @member program : 程序名称
         * @desc :
         *  描述模块所属的程序的名称
         */
        const std::string program;

        /**
         * @member name : 模块名称
         * @desc :
         *  描述模块的名称
         */
        const std::string name;

        /**
         * @member mmanager : 管理器
         * @desc :
         *  保留管理器指针,当发生了需要向管理器上报的事件时
         *  通过此指针向管理器报告事件
         */
        Manager* const manager;

        /**
         * @member syntrees : 模块列表
         * @desc :
         *  模块列表保存了从源文档分析的各个语法树
         *  这些模块结构保存了与源文档之间的关联关系
         *  在报错时需要被使用,所以不能丢弃
         *  但是这些模块中的语法结构,一经分析完成,立刻
         *  就被模块描述符夺取,统一管理
         */
        chainz<$ModuleGranule>    syntrees;

        /**
         * @member deps : 依赖表
         * @desc :
         *  在依赖补全阶段被使用,用于描述模块的依赖
         */
        depdescs deps;
        
    private:

        /**
         * @member mdocs : 源文档列表
         * @desc :
         *  在构建描述符表阶段,管理器负责填充此字段
         *  源文档列表记录了所有描述了此模块的源文档
         *  在语法分析阶段,模块描述符会依次从这些源
         *  文档中读取内容构建语法树
         */
        Dengine::vfds               mdocs;
    
    public:

        /**
         * @constructor : 构造方法
         * @desc : 模块描述符构造之初就应该知道自己属于哪个管理器
         * @param mana : 描述符所属的管理器,在构造期间,若prog名未给出,描述符会从管理器中读取当前构建应用的名字
         * @param name : 模块名
         * @param fd : 目标文件的文件描述符
         * @param prog : 应用名,若未指定,构造方法从管理器读取当前应用名
         */
        modesc( Manager& mana, const string& name, const string& prog = "" );
        modesc( const modesc& ) = delete;
        modesc( modesc&& ) = delete;
        ~modesc();

        /**
         * @method appendDocument : 绑定源文档
         * @desc : 将模块与源文档绑定
         *      此处存储的所有文档描述符
         *      其space会被截断为本地space也就是Work,Root或Apps标志位会被抹平
         * @return bool : 添加是否成功
         */
        bool appendDocument( Dengine::vfd desc );

        /**
         * @method getDocuments : 返回已绑定的文件描述符
         * @desc : 此方法在写入模块描述符或读取模块文件时使用
         * @param tr : 若此项为真
         *      所有文件描述符都会根据模块描述符
         *      与当前应用的关系,补全space
         *      也就是说,这里输出的文件描述符都能直接使用
         * @return Dengine::vfds : 返回修正过的文件描述符
         */
        Dengine::vfds getDocuments(bool tr = true )const;

        /**
         * @method constructAbstractSyntaxTree : 构建抽象语法树
         * @desc :
         *  一份源文档只能被解析为单独的语法树结构,语法树结构与文档保持着联系,如此便能追溯语法结构所属的文档
         *  一个模块可以由多份源文档描述,由此会产生多个语法树,但诸多语法树的全部内涵才能表述一个完整的模块
         *  模块描述符从更高的抽象层次接管每个语法树的子树,使得以模块描述符为根,形成抽象语法树,从模块描述符出发,能检索所有语法结构
         *  语法结构原属语法树并未销毁,自语法结构出发,能回溯所属语法树,进而定位语法结构被书写的源文档
         *  语法树与抽象语法树之间存在着自下而上的联系,所以最终,从语法结构出发也可以找到语法结构所属的模块描述符
         *  透明类的定义内容在此阶段被并入模块定义序列
         * @param log : 日志仓库
         * @return bool : 若构建成功,则构建产物存入模块描述符内,返回true,否则返回false
         */
        bool constructAbstractSyntaxTree( Lengine::logr& log );
};

using $modesc = agent<modesc>;

class modescs : public chainz<$modesc> {
    public:    
        string  aname;      //应用名称
        int     mtim;       //加载时,从模块描述符表中读取的时间戳

    public:
        modescs();
        modescs( const modescs& );
        modescs( modescs&& );
        ~modescs() = default;

        modescs& operator=( const modescs& );
        modescs& operator=( modescs&& );
};

}

#endif