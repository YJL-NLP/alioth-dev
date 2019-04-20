#ifndef __xengine__
#define __xengine__

#include "jsonz.hpp"
#include "token.hpp"
#include <istream>

namespace alioth {

/**
 * @class Xengine : 词法分析引擎
 * @desc :
 *  leXical Engine --- 词法分析引擎
 *  词法分析引擎知悉词法规则
 *  向其他模块提供词法分析功能
 */
class Xengine {

    private:
        /**
         * @member __table : 词汇表
         * @desc :
         *  词汇表用于描述词法符号与书写形式的一一对应关系
         *  多数终结符都是拥有一一对应书写形式的
         *  词汇表会在第一个词法引擎实例被创建时初始化
         */
        static Jsonz __table;

    private:
        /**
         * @member pre : 字节预览
         * @desc :
         *  扫描过程中,每个决策开始之前,当前输入字节的预览
         */
        int pre;

        /**
         * @member off : 偏移量
         * @desc :
         *  此成员被"单词测试"和"前缀测试"使用
         */
        int off;

        /**
         * @member begl,begc : 当前源代码行列
         * @desc :
         *  在词法分析过程中跟踪记录当前行列
         *  此二者被用来定位词法符号
         */
        int begl;
        int begc;

        /**
         * @member target : 目标状态
         * @desc :
         *  在前缀测试中,若前缀匹配成功,跳转的目标状态值
         */
        int target;

        /**
         * @member fixed : 确定样本
         * @desc :
         *  在单词测试和前缀测试中使用的样本
         */
        std::string fixed;

        /**
         * @member state : 当前状态
         * @desc :
         *  词法分析有限状态机的当前状态
         */
        int state;

        /**
         * @member stay : 停滞口令
         * @desc :
         *  若此值为true,有限状态机停滞一个字节,再决策一次
         */
        bool stay;

        /**
         * @member T : 临时记号
         * @desc :
         *  词法分析过程中,正在构建的符号
         */
        token T;

        /**
         * @member hit : 命中符号
         * @desc :
         *  当"赋值测试"命中时,用于替换T.id的终结符类型
         */
        VT hit;

        /**
         * @member synst : 语法分析器状态
         * @desc :
         *  词法分析器内置微型语法分析器的状态
         */
        int synst;

        /**
         * @member ret : 词法分析产物
         * @desc :
         *  每次词法分析时用作产物的词法符号序列
         */
        tokens ret;

        /**
         * @member pis : 源代码输入流指针
         * @desc :
         *  此成员在词法分析流程中被使用
         *  在词法分析开始时被初始化.
         */
        std::istream* pis;

        /**
         * @member limit : 限制
         * @desc :
         *  此成员描述词法分析流程是否受到范围限制
         *  若受到范围限制,则词法分析仅分析模块签名
         */
        bool limit;

    private:

        /**
         * @method begin : 启动词法分析
         * @desc :
         *  此方法为启动一次词法分析流程准备环境.
         *  初始化所有所需的变量.
         * @param is : 源代码输入流
         * @param li : 限制
         */
        void begin( std::istream& is, bool li );

        /**
         * @method goon : 继续
         * @desc :
         *  考虑停滞口令后,输入一个字节,继续检查下一个字节
         *  若停滞口令有效,则失活停滞口令,并结束
         */
        void goon();

        /**
         * @method check : 确认输入
         * @desc :
         *  确认当前正在构建的词法符号已经完成,将其输入分析产物序列
         *  同时,考虑词法分析范围限制,使用微型语法分析算法做出状态决策
         * @param t : 确认的词法符号
         * @param s : 是否连同当前正在检查的预览字节一起确认输入
         */
        void check( VT t, bool s );

        /**
         * @method test : 开始一次单词测试
         * @desc :
         *  单词测试用于测试接下来的输入内容是否与词汇表中的指定单词的书写形式相同
         *  由当前已分析的前缀最多仅能引导一种确定词汇时,使用单词测试简化分析流程
         *  单词测试流程由词法分析算法包含
         *  此方法用于进入单词测试流程
         * @param v : 要测试的词法符号
         * @param o : 开始测试时已经扫描的前缀的偏移量
         */
        void test( VT v, int o );

        /**
         * @method prefix : 开始一次前缀测试
         * @desc :
         *  前缀测试用于测试输入序列是否满足指定前缀,若满足,则跳转到指定状态进行后续处理
         *  前缀测试用于应对前缀相同的多个不同词汇的集中扫描处理.
         *  此方法用于开始一次前缀测试
         * @param s : 前缀测试样板
         * @param o : 前缀测试开始的偏移量
         * @param t : 前缀测试通过后应当跳转的状态
         */
        void prefix( const char* s, int o, int t );

        /**
         * @method assign : 开始一次赋值测试
         * @desc :
         *  当已分析的内容足以成为运算符但也可能与'='联结成为赋值运算符,使用赋值测试来统一处理逻辑,消除重复代码.
         *  此方法用于进入赋值测试所需的状态.
         * @param h : 当赋值测试命中时,成立的词法符号
         * @param m : 当赋值测试未通过时,成立的词法符号
         */
        void assign( VT h, VT m );

        /**
         * @method sequence : 开始一个序列扫描
         * @desc :
         *  序列扫描用于扫描由引号包含的,表示文本的内容
         * @param h : 当扫描命中时,成立的词法符号
         */
        void sequence( VT h );

        /**
         * @method islabelb : 判断字符是否能用作标识符的开头
         * @desc :
         *  此方法用于判断字节是否能用作标识符的开头
         * @param c : 要判断的字符
         * @return bool
         */
        static bool islabelb( int c );

        /**
         * @method islabel : 判断字符是否可以用作标识符的内容
         * @desc :
         *  此方法用于判断字符是否可用作标识符的内容
         * @param c : 要判断的字符
         * @return bool
         */
        static bool islabel( int c );

        /**
         * @method extractTokens : 从输入流提取词法符号序列
         * @desc :
         *  此方法使用词法分析算法从输入流提取词法符号序列
         *  此方法使用词法分析引擎存储中间结果,中间内容
         *  所以一个词法引擎实例不能在多线程中使用
         * @param is : 输入流
         * @param limit : 是否限制扫描范围,此参数为真,则只分析模块签名
         * @return tokens : 返回词法序列
         */
        tokens extractTokens( std::istream& is, bool limit );

        /**
         * @method init : 初始化
         * @desc :
         *  初始化词法分析引擎的公共基础资源
         */
        static void init();
    
    public:
        /**
         * @constructor : 构造方法
         * @desc :
         *  构造方法用于初始化词法引擎所需的资源,构造词法引擎实例
         */
        Xengine();

        /**
         * @destructor : 析构方法
         * @desc :
         *  析构方法会释放词法引擎所占用的资源.
         */
        ~Xengine() = default;

        /**
         * @method parseSourceCode : 解析源代码
         * @desc :
         *  分析源代码,从输入流产生一个词法记号序列
         * @param is : 源代码输入流
         * @return tokens : 产生的词法记号序列
         *  若分析过程中产生了词法错误,则tokens中包含无效记号R_ERR
         */
        tokens parseSourceCode( std::istream& is );

        /**
         * @method parseModuleSignature : 解析模块签名
         * @desc :
         *  从文本中分析模块签名,产生词法记号序列
         *  后续内容会被放弃,此方法内置了小规模的语法分析器来实现此功能
         * @param is : 源代码输入流
         * @return tokens : 产生的词法记号序列
         *  其中仅包含模块签名所对应的词法记号
         */
        tokens parseModuleSignature( std::istream& is );

        /**
         * @method written : 书写格式
         * @desc :
         *  written方法用于将词法符号的书写形式写入字符串
         *  对于在词汇表上的词法符号,本方法返回词法符号表
         *  对词汇表不能一一对应的词法符号,本方法返回文本内容
         * @param t : 词法符号
         * @return std::string : 书写格式字符串
         */
        static std::string written( const token& t );

        /**
         * @method isLabel : 检查标识符
         * @desc :
         *  检查一个字符串是否能作为标识符存在
         * @param s : 要检查的字符串
         * @return bool : 检查结果
         */
        static bool isLabel( const std::string& s );

        /**
         * @method extractText : 提取文本
         * @desc :
         *  此方法用于从字面字符串或字面字符中提取文本
         *  过程中会处理转义字符
         *  此方法不考虑由$引导的内置表达式的存在
         * @param t : 要提取的词法符号
         * @return std::string : 文本内容
         */
        static std::string extractText( const token& t );

        /**
         * @method priority : 提取优先级
         * @desc :
         *  此方法用于提取运算符的优先级
         *  若传入的词法记号不是运算符,则方法返回0
         *  运算符优先级越高,返回值越高
         * @param t : 要检查的词法符号
         * @return int : 词法符号的优先级
         */
        static int priority( const token& t );
};

}

#endif