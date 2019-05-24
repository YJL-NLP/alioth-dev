#ifndef __lengine__
#define __lengine__

/**
 * Lengine 日志引擎
 * 
 * 日志引擎主要用于对日志信息进行管理,转换,输出
 * 
 * 日志引擎系统由日志模板,日志器,日志项和日志引擎构成
 * 
 * 日志模板描述了一种日志类型携带多少个参数,如何组织和展示这些参数的信息
 * 
 * 日志项则保存了对应的日志模板的日志模板号,并携带了所需的参数
 * 
 * 日志器主要用于将一系列源自于同一份源文档的日志项绑定在一起
 * 
 * 日志引擎则用于通过日志模板所描述的逻辑,将日志项翻译成对应的日志对象
 * 产生Json对象,用于描述一条日志信息.
 */

#include "jsonz.hpp"
#include "token.hpp"
#include "agent.hpp"

#include <map>

namespace alioth {
using namespace std;

class Lengine {

    public:
        enum Severity {
            Error = 1,
            Warning,
            Information,
            Hint,
        };

        //日志模板号 --- 若已经存在需要表达的信息,请尽量不要添加更多重复的日志模板
        enum Logt {
            E0 = 0,             //日志测试

            // 1 - 50           //来自eproto解析过程的任何错误
            E1 = 1,
            E2,
            E3,
            E4,
            E5,
            E6,
            // 51 - 100         //来自import解析过程的的任何错误
            E51 = 51,
            E52,
            E53,
            // 101 - 200        //来自manager解析过程的任何错误
            E101 = 101,
            E102,
            E103,
            E104,
            E105,
            E106,
            E107,
            E108,
            E109,
            E110,
            // 201 - 300        //来自module解析过程的任何错误
            E201 = 201,
            E202,
            E203,
            E204,
            // 301 - 400        //来自method和metdef解析过程的任何错误
            E301 = 301,
            E302,
            E303,
            E304,
            E305,
            E306,
            E307,
            E308,
            E309,
            E310,
            E311,
            E312,
            // 401 - 500        //来自seme解析过程的任何错误
            E401 = 401,
            // 501 - 600        //来自dtype解析过程的任何错误
            E501 = 501,
            E502,
            E503,
            E504,
            // 601 - 700        //来自block解析过程的任何错误
            // 701 - 800        //来自enum解析过程的任何错误
            E701 = 701,
            E702,
            E703,
            // 801 - 900        //来自classd解析过程的任何错误
            E801 = 801,
            E802,
            E803,
            E804,
            E805,
            E806,
            E807,
            // 1001 - 2000      //语法解析错误
            E1001=1001,         
            E1002,              
            E1003,              
            E1004,              
            E1005,              
            E1006,
            E1007,
            E1008,
            E1009,     
            E1010,         

            // 2001             //整理后的错误号

            E2001 = 2001,      //错误,名称被占用
            H2002,             //提示,另一个使用
            E2003,             //错误,名称二义性
            E2004,             //错误,未找到定义
            E2005,             //错误,基类引起循环依赖
            E2006,             //错误,基类不是类
            E2007,             //错误,模板参数总量不匹配
            E2008,
            E2009,
            E2010,
            E2011,
            E2012,
            E2013,
            E2014,
            E2015,
            E2016,
            E2017,
            E2018,
            E2019,
            E2020,
            E2021,
            E2022,
            E2023,
            E2024,
            E2025,
            E2026,
            E2027,
            E2028,
            E2029,
            E2030,
            E2031,
            E2032,
            E2033,
            E2034,
            E2035,
            E2036,
            E2037,
            E2038,
            E2039,
            E2040,
            E2041,
            E2042,
            E2043,
            E2044,
            E2045,
            E2046,
            E2047,
            E2048,
            E2049,
            E2050,
            E2051,
            E2052,
            E2053,
            E2054,
            E2055,
            E2056,
            E2057,
            E2058,
            E2059,
        };

        struct logi {
            Logt        tno;    //日志模板号
            tokens      arg;    //日志参数
            chainz<tuple<string,logi>>
                        sub;    //子日志

            template<typename ...Args>
            logi( Logt t = E0, Args&&... args):tno(t){
                (arg.construct(-1,forward<Args>(args)), ...);
            }

            template<typename ...Args>      //注意,此方法返回自身引用,与其他类似方法的语义不同.
            logi& operator()(const string& p, Logt t, Args&& ... args){
                sub.insert({p,logi(t,forward<Args>(args)...)},-1);
                return *this;
            }
        };

        class logs : public chainz<logi> {
            public:
                const string path;
            logs(const string p):path(p){}
            ~logs(){}

            template<typename ...Args>
            logi& operator()(Logt t, Args&& ... args){
                insert(logi(t,forward<Args>(args)...),-1);
                return operator[](-1);
            }
        };

        class logr :public thing, public chainz<logs> {  //日志仓库
            public:logs& operator()(const string& p);
        };

    private:

        /**
         * @struct logc : 日志调用
         */
        struct logc {
            int             tno;    //模板号
            int             pat;    //路径
            chainz<int>     arg;    //传参
        };

        /**
         * @struct logt : 日志模板
         * @desc : 用于描述一种模式,日志引擎要遵循这种
         *      模式组织日志项中携带的信息.
         */
        struct logt {
            Severity    sev;    //严重性

            /**
             * @member beg : 起始位置
             * @desc : 描述当日志项携带正确参数时
             *      日志引擎应该如何从中提取起始位置的信息
             *      可选的格式如下:
             *          "n"| (("b"|"e"), num)
             *      起始位置取n表示没有确切的行列起始位置
             *      否则,取num号参数的起始或结束位置为起始位置
             *      使用b或e来区分.
             */
            string      beg;

            /**
             * @member end : 终止位置
             * @desc : 描述当日志项携带正确参数时
             *      日志引擎应该如何从中提取终止位置的信息
             *      其格式和语义与beg相同,请参考beg相关的定义
             */
            string      end;
            
            /**
             * @member msg : 消息模板
             * @desc : 描述了如何将日志项中的参数组织成
             *      人类可读的报错信息.
             *      其中可以包含转义字符和占位符
             *      占位符用于描述对参数的使用,语法如下
             *      "%",['r'|'R'|
             *           'g'|'G'|
             *           'b'|'B'|
             *           'y'|'Y'|
             *           'p'|'P'|
             *           'c'|'C'],num
             *      其中,可省的可以在参数下标之前加一个字母
             *      用于表示参数应有的颜色,若环境和情形支持
             *      这些颜色会被实现出来.
             *      不同的字母用于表示不同的颜色,对应的大写
             *      格式表示体现颜色的同时加粗,颜色表如下
             *          r : red     -- 红色
             *          g : green   -- 绿色
             *          b : blue    -- 蓝色
             *          y : yellow  -- 黄色
             *          p : purple  -- 紫色
             *          c : cyan    -- 青色
             * 
             *      请注意,msg最多只能携带10个参数,下标超过9的变量不会被正常引用
             */
            string      msg;

            /**
             * @member sub : 协助信息
             * @desc : 有些日志整体信息分布在不同的严重性,不同的源文档上
             *      使用协助信息将日志的表达能力增强.
             */
            chainz<logc>    sub;
        };

    private:
        map<int,logt>   mrepo;          //日志模板仓库,将日志模板和日志模板号对应起来
        bool            mecolor = true; //是否启用日志模板中的颜色.
        bool            mepath = true;  //是否启用文件路径
        string          mpath;          //日志文件路径,此值会在翻译日志库时自动变化.

    public:
        Lengine() = default;
        Lengine( const Lengine& ) = delete;
        Lengine( Lengine&& ) = delete;
        ~Lengine() = default;

        bool config( const Jsonz& conf );
        void color( bool );
        void path( bool );
        void path( const string& p);
        
        logs fordoc( const string& fname );

        template<typename ...Args>
        static logi log( Logt t, Args&&... args ) {
            logi i;
            i.tno = t;
            (i.arg.construct(-1,forward<Args>(args)), ...);
            return move(i);
        }

        Jsonz operator()( const logr& lr);
        Jsonz operator()( const logs& ls);
        Jsonz operator()( const logi& li);
};

}
#endif