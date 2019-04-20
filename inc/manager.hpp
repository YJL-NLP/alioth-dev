#ifndef __manager__
#define __manager__

/**
 * manager.hpp
 * 作者 : 王雨泽 2018/10/01
 * 
 * manager用于对项目环境进行管理
 * 记录文档路径组织结构
 * 记录文档与模块之间的关联和依赖关系
 * 记录翻译器及翻译器配置文件
 */

#include "jsonz.hpp"
#include "alioth.hpp"
#include "chainz.hpp"
#include "modesc.hpp"
#include "dengine.hpp"
#include "lengine.hpp"
#include "sengine.hpp"
#include <memory>
#include <list>

namespace alioth {
using namespace std;

struct vsd : Dengine::vfd {
    string          define;     //源文档所属模块
};

class Manager;
using ManagerRef = shared_ptr<Manager>;

class Manager {

    public:

        enum BuildType {MACHINECODE,SYNTAXCODE};
    
    private:
        using missrec = tuple<$depdesc,$modesc>;  //模块丢失的记录:丢失模块,需求来源
        using multrec = tuple<string,$modesc>;   //依赖有多个选项的记录:依赖模块名,依赖来源
        using urchrec = tuple<string,$modesc>;   //应用不可达的记录:应用名,依赖来源

        using missrecs = chainz<missrec>;
        using multrecs = chainz<multrec>;
        using urchrecs = chainz<urchrec>;

    private:
        string              appname;        //本次构建的目标应用名称
        Lengine             mlengine;       //日志引擎
        Dengine             mdengine;       //文档引擎
        Sengine             msengine;       //语义引擎
        modescs             mwork;          //工作空间中的模块描述符
        modescs             mroot;          //根空间中的模块描述符
        map<string,modescs> mapps;          //各个应用空间的模块描述符

        /**
         * 下述内容是每次构建时的构建时记录内容
         */
        list<string>        mnames;         //构建目标名单
        missrecs            missing;        //构建过程中,模块丢失的记录
        multrecs            multing;        //构建过程中,依赖二义性的记录
        urchrecs            urching;        //构建过程中,应用不可达的记录
        modescs             mpadding;       //用于检查引用栈的队列
    
    private:

        //下列方法用于在不重复的情况下记录三种错误
        void recordMissing($depdesc D, $modesc M, Lengine::logs& log );
        void recordMultipleChoices( string D, $modesc M, Lengine::logs& log );
        void recordUnreachable( string A, $modesc M, Lengine::logs& log );

        //下述方法用于检查一个模块作为引用是否引起循环引用,是否引起层次错误.
        bool verifyDependenceStack( $modesc dep, Lengine::logs& log );

    public:
        Manager();
        Manager(const Manager&) = delete;
        Manager(Manager&&) = delete;
        ~Manager();

        bool setAppName( const string& name );
        string getAppName() const;

        /**
         * @method verifyAppName : 验证应用名称
         * @desc : 要构建一个应用
         *      应用名不能为空,不能与"alioth"重名
         *      应用名也至少不能和已经安装的应用重名
         *      此方法要检查应用空间的信息,所以应该在
         *      正确配置了Dengine之后再使用.
         * @return int:
         *      0 应用名没问题
         *      1 应用名为空
         *      2 应用名与alioth重名
         *      3 应用名与已安装的应用重名
         */
        int verifyAppName()const;

        /**
         * @method getDocumentEngine : 获取manager所绑定的文档引擎
         * @desc : 获取manager所绑定的文档引擎的引用
         * @return Dengine& : 绑定的文档引擎的引用
         */
        Dengine& getDocumentEngine();

        /**
         * @method getLogEngine : 获取manager所绑定的日志引擎
         * @desc : 获取manager所绑定的日志引擎的引用
         * @return Lengine& : 绑定的日志引擎的引用
         */
        Lengine& getLogEngine();

        /**
         * @method config : 配置Manager
         * @desc : 从配置文件中读取配置信息,配置Manager的各个部分
         *      此方法会根据配置内容,调用其他配置方法
         * @param conf : 配置信息,通常来自doc/alioth.json
         * @return bool : 配置是否成功
         */
        bool config( Jsonz conf );

        /**
         * @method loadModescTable : 加载模块描述符表
         * @desc : 从指定的空间向指定的目标加载模块描述符
         * @param space : 指定要加载模块描述符的空间
         *      取值范围是Work,Root,Apps;
         * @param app : 指定应用名称
         *      当space取Apps时,app有效,若app为空,表示为所有应用分别加载模块描述符表
         * @param apps : 指定此参数表示将所有应用对应的模块描述符加载出来
         * @param descs : 指定此参数表示将加载的模块描述符表存入此对象,而不是默认目标中
         * @param table : 指定此参数表示从此对象中读取模块描述符表
         * @return bool : 执行是否成功
         */
        bool loadModescTable( vspace space, const string& app = "" );
        bool loadModescTable( map<string,modescs>& apps );
        bool loadModescTable( modescs& descs, Jsonz table );

        /**
         * @method buildModescTable : 构建模块描述符表
         * @desc : 构建模块描述符表的主要目的是保持信息同步
         *      构建动作会移除实际上不存在的模块的描述符或实际上不存在的文件的描述符
         *      构建动作会补全没有添加过的描述符,更新应该修改的描述符信息
         * @param log : 对于后缀名为.alioth却不能读取源文档首部的文件
         *      给出报错信息.
         * @param space : 构建的目标空间
         *      取值范围:Work,Root,Apps
         * @param app : 应用名称
         *      当space取Apps时,此参数有效
         *      当app为空时,表示为所有应用空间构建模块描述符
         * @return bool : 执行是否成功
         */
        bool buildModescTable( Lengine::logr& log, vspace space, const string& app = "" );

        /**
         * @method printModescTable : 打印模块描述符表
         * @desc : 模块描述符表携带了很多信息
         *      包括当前应用,每个模块的名称,依赖,源文档描述符
         * @param space : 指定空间
         *      取值范围是Work,Root,Apps
         * @param app : 当space取值Apps时有效
         *      此值为空时,表示为每个应用分别打印模块描述符表
         * @param  table : 若指定此值,模块描述符表被打印到此对象中
         *      若方法正在为所有应用打印模块描述符表,则table会变成一个Object,每个键都是一个应用名
         * @return bool:
         */
        bool printModescTable( vspace space, const string& app = "" );
        bool printModescTable( Jsonz& table, vspace space, const string& app = "" );

        void specifyModule( const string& name );

        /**
         * @method Build : 构建目标模块
         * @desc : 以目标模块为根,构建依赖树,执行构建流程.
         * @param type : 构建类型
         * @param log : 日志器集合
         * @return AERRNO : 返回错误码
         */
        bool Build( const BuildType type, Lengine::logr& log );

        /**
         * @method completDependencies : 补全依赖
         * @desc : 补全所有构建目标模块的依赖
         *          1. 检查模块的依赖层次是否超过3层
         *          2. 检查模块的依赖是否出现循环依赖
         *          3. 补全来自应用的模块,及其依赖
         * @param name : 若指定此参数,表示为工作空间中名为name的模块扫描依赖,返回值中会包含这个模块
         * @param log : 一旦发现依赖问题,错误日志就会被填写.
         * @param desc : 若指定此参数,只扫描此模块的依赖,返回值中不包含这个模块
         *      管理器应该将空的描述符与构建队列里的描述符对比,只添加新的描述符进去
         *      同时应该保证构建队列中的描述符都是加载了完整的语法树
         * @param output: 用于承受输出,方法同时也使用此容器进行查重,传参时请慎重考虑容器的内容
         * @return int : 若任务彻底失败,则返回值小于0
         *                否则返回值表示出现错误的依赖的个数
         */
        int completDependencies( const string& name, Lengine::logr& log, modescs& output );
        int completDependencies( $modesc desc, Lengine::logr& log, modescs& output );

        /**
         * @method closeUpDependency : 闭合依赖项
         * @desc :
         *  根据依赖补全规则闭合依赖项
         * @param dep : 要闭合的依赖项,它必须与模块描述符相关联
         * @return int :
         *  0 : 成功
         *  1 : 依赖模块丢失
         *  2 : 程序不可达
         *  3 : 可选项不唯一
         */
        int closeUpDependency( $depdesc dep );
};

}
#endif