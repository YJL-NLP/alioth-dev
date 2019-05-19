#ifndef __dengine__
#define __dengine__

#include "chainz.hpp"
#include "alioth.hpp"
#include "jsonz.hpp"

#include <memory>
#include <iostream>
#include <functional>

#ifdef __WINDOWS__
#define dirdvc '\\'
#define dirdvs "\\"
#else
#define dirdvc '/'
#define dirdvs "/"
#endif

namespace alioth {
using namespace std;

/**
 * @enum vspace : 虚拟空间,alioth将所有文件以 vspace+ / path* / name 的形式组织
 * @desc : 请求文档流时,描述文档来源
 *      流获取方法应当根据文档源对传入的文档名进行处理
 *      得到正确的文档名,也就是说,传入流获取方法的文档名是虚拟文件路径
 *      虚拟路径需要组合使用如 Project|Arc 或 Root|Inc
 */
using vspace = unsigned int;
static const vspace Arc = 0x01;       //静态链接库子路径,可配置,仅对Work分支起效
static const vspace Bin = 0x02;       //可执行文件子路径,可配置,仅对Work分支起效
static const vspace Doc = 0x04;       //文档子路径,可配置,仅对Work分支起效
static const vspace Lib = 0x08;       //动态链接库子路径,可配置,仅对Work分支起效
static const vspace Inc = 0x10;       //模型文件子路径,可配置,仅对Work分支起效
static const vspace Obj = 0x20;       //中间目标文件子路径,可配置,仅对Work分支起效
static const vspace Src = 0x40;       //源代码文件子路径,可配置,仅对Work分支起效
static const vspace Work = 0x1000;    //当前项目路径,可配置的分支
static const vspace Root = 0x2000;    //alioth根目录,可配置的分支
static const vspace Apps = 0x4000;    //指定此文档应当来自某个app

using uistream = unique_ptr<istream>;
using uostream = unique_ptr<ostream>;

/**
 * @class Dengine : 文档引擎
 * @desc :
 *  alioth管理器使用文档引擎实现文档抽象
 *  dengine使得文档可以来自任何地方
 *  与虚拟路径相关的一些概念:
 *      module : 模块,alioth编程语言编译的最小单位
 *      root : 指alioth的安装目录,其中包含了标准库和工具链
 *      apps : 指通过alioth安装的应用包
 *      work : 被认为是当前项目路径
 *      文档 : 指被认为是alioth源代码文档的文件
 *      文件 : 能打开关闭,能读写的抽象的数据流
 *      实际项目管理中,版本号应当存在于配置文件中
 */
class Dengine {

    public:

        /**
         * @struct vfd : 虚拟文件描述符
         * @desc :
         *  在alioth虚拟路径中,用于描述一个文件的概要信息
         */
        struct vfdm {
            public:
                string          name;       //文件名,带路径的全名
                string          app;        //当文件来自某个app时,也即space中包含Apps时有效,是app名
                vspace          space;      //文件所在的空间

            public:
                string detectApp(const string& capp )const; //capp为当前应用名
                bool operator == ( const vfdm& an )const;
                bool operator != ( const vfdm& an )const;
        };
        struct vfd : public vfdm {
            public:
                time_t          mtim;       //最后一次修改时间,单位s
                size_t          size;       //文件大小,单位B
        };
        using vfds = chainz<vfd>;

        using IstreamGetter = function<uistream(const string&, vspace,const string&)>;
        using OstreamGetter = function<uostream(const string&, vspace,const string&)>;
        using VfileEnumer   = function<vfds(vspace, const string&, const string&)>;
        using VfileStater   = function<bool(const string&,vspace,const string&,vfd&)>;

    private:
        string              mdirRoot;  //默认路径前缀 /alioth/
        string              mdirWork;  //默认路径前缀 ./
        string              mdirArc;   //配置后只对Work前缀起效,默认路径 arc/
        string              mdirBin;   //配置后只对Work前缀起效,默认路径 bin/
        string              mdirDoc;   //配置后只对Work前缀起效,默认路径 doc/
        string              mdirLib;   //配置后只对Work前缀起效,默认路径 lib/
        string              mdirInc;   //配置后只对Work前缀起效,默认路径 inc/
        string              mdirObj;   //配置后只对Work前缀起效,默认路径 obj/
        string              mdirSrc;   //配置后只对Work前缀起效,默认路径 src/

        IstreamGetter   midoc;  //获取输入流的回调方法
        OstreamGetter   modoc;  //获取输出流的回调方法
        VfileEnumer     medoc;  //枚举文档名的回调方法
        VfileStater     msdoc;  //查文件信息的回调方法

    public:
        Dengine();
        Dengine( const Dengine& ) = delete;
        Dengine( Dengine&& ) = delete;
        ~Dengine();

        /**
         * @method setMethodGetIs : 设置文档输入流获取器
         * @desc :
         *  选择一个自定义的方法作为文档输入流获取器
         *  默认情况下,文档输入流获取器由默认文档引擎中的DefaultMethodGetInputDoc充当
         * @param methodIDoc : 用来作为获取器的方法
         * @return bool : 设置是否成功
         */
        bool setMethodGetIs( IstreamGetter methodIDoc );

        /**
         * @method setMethodGetOs : 设置文档输出流获取器
         * @desc :
         *  选择一个自定义的方法作为文档输出流获取器
         *  默认情况下,文档输出流获取器由默认文档引擎中的DefaultMethodGetOutputDoc充当
         * @param methodODoc : 用来作为获取器的方法
         * @return bool : 设置是否成功
         */
        bool setMethodGetOs( OstreamGetter methodODoc );


        /**
         * @method setMethodEnumFile : 设置文件枚举器
         * @desc : 
         *  选择一个自定义的方法作为文件枚举器
         *  默认情况下,文件枚举器由默认文档引擎中的DefaultMethodEnumFile充当
         * @param methodEDoc : 用来作为枚举器的方法
         * @return bool : 设置是否成功
         */
        bool setMethodEnumFile( VfileEnumer methodEDoc );

        /**
         * @method setMethodStatFile : 设置文件属性检查器
         * @desc :
         *  选择一个自定义的方法作为文件属性检查器
         *  默认情况下,文件属性枚举其由默认文档引擎中的DefaultMethodStatFile充当
         * @param methodSDoc : 用于作为属性检查器的方法
         * @return bool : 设置是否成功
         */
        bool setMethodStatFile( VfileStater methodSDoc );


        uistream    getIs( const string&, vspace, const string& app = "" )const;
        int         getIfd( const string&, vspace, const string& app = "" )const;
        uostream    getOs( const string&, vspace, const string& app = "" )const;
        int         getOfd( const string&, vspace, const string& app = "" )const;
        vfds        enumFile( vspace, const string& subdir, const string& app = "" )const;
        bool        statFile( const string& fname, vspace, const string& app, vfd& )const;
        string      getPath(const string&, vspace, const string& app = "" )const;
        
        uistream    getIs( const vfdm& fdm )const;
        int         getIfd( const vfdm& fdm )const;
        uostream    getOs( const vfdm& fdm )const;
        int         getOfd( const vfdm& fdm )const;
        vfds        enumFile( const vfdm& fdm )const;
        string      getPath(const vfdm& fdm )const;

        /**
         * @method setSpacePath : 对默认文档引擎进行基础设置
         * @desc : 
         *  默认文档引擎所使用的每一个项目路径都是可配置的
         *  通过此方法配置标准库或安装库的子路径会失败
         *  通过此方法配置默认文档引擎的路径
         * @param source : 要配置的文档路径
         * @param dir : 配置的值
         * @return bool : 配置是否成功
         */
        bool setSpacePath( vspace space, string dir );

        bool config( Jsonz conf );
};

}
#endif