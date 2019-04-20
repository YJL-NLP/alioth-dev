#ifndef __yengine__
#define __yengine__

#include "alioth.hpp"
#include "modesc.hpp"
#include "lengine.hpp"
#include "depdesc.hpp"
#include "attrdef.hpp"
#include "enumdef.hpp"
#include "classdef.hpp"
#include "loopimpl.hpp"
#include "methoddef.hpp"
#include "branchimpl.hpp"
#include "methodimpl.hpp"
#include "constructimpl.hpp"
#include "flowctrlimpl.hpp"
#include "insblockimpl.hpp"
#include "modulegranule.hpp"
#include "expressionimpl.hpp"
#include "modulesignature.hpp"

namespace alioth {

/**
 * @class Yengine : 语法引擎
 * @desc :
 *  sYntax Engine --- 语法分析引擎
 *  语法引擎知悉所有的语法规则
 *  向其他模块提供语法分析功能
 */
class Yengine {

    private:
        /**
         * @struct state : 状态
         * @desc :
         *  描述一个有限状态机的状态
         */
        struct state {

            public:

                /**
                 * @member s : 状态名
                 * @desc :
                 *  用于实现基于状态转换的语法分析过程
                 */
                int s;

                /**
                 * @member c : 符号统计
                 * @desc :
                 *  记载当前状态下保留了多少文法符号
                 */
                int c;

            public:

                /**
                 * @method state : 构造方法
                 * @desc : 构造一个状态就意味着进入了一个新的状态
                 *      此动作的语义是,移进c个单词,进入s状态
                 * @param _s : 新状态的状态名
                 * @param _c : 新状态初始状态下挂起的单词总量
                 */
                state(int _s,int _c = 1);

                /**
                 * @operator int& : 类型转换方法
                 * @desc : 将state当成一个数字使用时,state的语义就代表它自己的状态名称
                 */
                operator int&();
        };

        /**
         * @class smachine : 状态机
         * @desc :
         *  描述一个用于实现LR文法分析算法的有限状态机
         *  此状态机面向状态归约,简化文法分析程序的设计
         */
        class smachine {

            private:
                /**
                 * @member states : 状态栈
                 * @desc :
                 *  状态栈中的状态序列
                 *  描述了归约,移出动作所需的信息
                 *  跟踪记录有限状态机的状态变化轨迹
                 *  如此,归约动作便不需要明确指向某个目标状态
                 *  而是明确归约的状态个数,按照顺序弹出状态栈即可
                 *  继而,归约动作在局部范围内的功能便成为了可复用
                 *  可重入的状态.
                 */
                chainz<state> states;

                /**
                 * @member it : 终结符序列迭代器
                 * @desc :
                 *  进行归约,移进等操作时,有限状态机需要从迭代器输入
                 *  记号,同时移动迭代器的输入指针
                 */
                tokens::iterator& it;
            
            public:
                /**
                 * @method smachine : 构造方法
                 * @desc : 状态栈系统必须跟一个有效的迭代器绑定在一起
                 * @param i : 终结符序列迭代器
                 */
                smachine(tokens::iterator& i);

                /**
                 * @method movi : 移进方法
                 * @desc : 移进c个单词,并进入s状态
                 *      c个单词被归属与s状态中挂起的单词
                 * @param s : 新的状态名称
                 * @param c : 移进单词的总量,通常是1
                 */
                void movi(int s, int c=1);

                /**
                 * @method movo : 移出方法
                 * @desc : 用于退回c个移进动作
                 *      主要用于在判定后决定撤销一个预动作时使用
                 *      如果移出的状态中包含了归约动作,则很有可能造成混乱,需要谨慎使用
                 * @param c : 要移出的状态个数
                 */
                void movo(int c = 1 );

                /**
                 * @method stay : 停滞方法
                 * @desc : 用于将更多的单词挂载到当前状态中
                 *      这有助于在一个状态中识别可选的重复的几个单词
                 *      因为若单词的个数不确定,就不能在运行之前给状态确定名称
                 * @param c : 要挂起的单词的个数
                 */
                void stay(int c = 1 );

                /**
                 * @method redu : 归约方法
                 * @desc : 用于将几个状态下的单词全部归约成一个非终结符
                 *      此动作也可能携带当前即将输入的一个单词
                 * @param c : 要归约的状态个数
                 *      c >= 0 --> 归约即将输入的单词,以及c个状态中挂起的单词
                 *      c < 0 --> 仅归约c个状态中挂起的单词
                 * @param n : 归约后,非终结符的id
                 */
                void redu(int c, VN n );

                /**
                 * @method size : 栈大小
                 * @desc :
                 *  栈大小反映着有限状态栈中的状态个数
                 *  此数值通常用于控制何时退出状态决策
                 */
                int size()const;

                /**
                 * @operator state& : 提取顶层状态
                 * @desc :
                 *  最后一个入栈的状态被视为顶层状态
                 *  通常情况下的状态决策都是基于顶层状态
                 *  进行的,所以此方法用于提供一种方便的途径
                 *  提取顶层状态
                 */
                operator state&();
        };
    
    protected:

        /**
         * @method constructParameterList : 构建参数列表
         * @desc :
         *  在方法定义,方法实现,运算符定义或运算符实现中,都有参数列表语法结构
         *  所以此结构的语法分析过程被单独实现为一个方法
         * @param it : 输入记号序列迭代器
         * @param log : 日志器
         * @param sc : 参数所在作用域
         * @param ps : 参数列表容器
         * @return bool : 构建是否成功
         */
        bool constructParameterList( tokens::iterator& it, Lengine::logs& log, $scope sc, morpheme::plist& ps );

        /**
         * @method constructParameterDefinition : 构建参数定义
         * @desc :
         *  从记号输入序列构建一个参数定义
         * @param it : 输入记号序列迭代器
         * @param log : 日志器
         * @param sc : 参数所在作用域
         * @return $paramd : 参数定义
         */
        $ConstructImpl constructParameterDefinition( tokens::iterator& it, Lengine::logs& log, $scope sc );

        /**
         * @method constructParameterImplementation : 构建参数实现
         * @desc :
         *  从记号序列构建一个参数实现
         * @param it : 输入记号序列的到期
         * @param log : 日志器
         * @param sc ; 参数所在的作用域
         * @return $ConstructImpl : 参数定义
         */
        $ConstructImpl constructParameterImplementation( tokens::iterator& it, Lengine::logs& log, $scope sc );

        /**
         * @method constructDataType : 构建数据类型
         * @desc :
         *  从记号输入序列构建数据类型语法结构
         * @param it : 输入序列
         * @param log : 日志器
         * @param sc : 作用域
         * @param absorb : 是否将'<'视为模板参数列表的开头符号
         * @return $dtype : 产生的数据结构
         */
        $dtype constructDataType( tokens::iterator& it, Lengine::logs& log, $scope sc, bool absorb );

        /**
         * @method constructElementPrototype : 构建元素原型
         * @desc :
         *  从记号输入序列构造元素原型
         *  此方法适用于元素原型被连续书写的情景
         *  比如方法返回值类型,或as表达式的运算子等
         * @param it : 输入序列
         * @param log : 日志器
         * @param sc : 作用域
         * @param bool : 是否将'<'视为模板参数列表的开头符号
         * @return $eproto : 产生的元素原型
         */
        $eproto constructElementPrototype( tokens::iterator& it, Lengine::logs& log, $scope sc, bool absorb );

        /**
         * @method constructAtomicNameUseCase : 构建原子名称用例
         * @desc :
         *  从记号输入序列构造原子名称用例
         * @param it : 输入序列
         * @param log : 日志器
         * @param sc : 作用域
         * @param bool : 是否将'<'视为模板参数列表的开头
         * @return nameuc::atom : 原子名称用例
         */
        nameuc::atom constructAtomicNameUseCase( tokens::iterator& it, Lengine::logs& log, $scope sc, bool absorb );

    public:

        /**
         * @method constructSyntaxTree : 从记号序列构建语法树
         * @desc :
         *  从记号序列构建语法树,此语法树不一定描述一个模块的所有语法结构
         *  但是,它一定描述了一个源文档中的所有语法结构
         * @param is : 记号输入序列
         * @param log : 日志器
         * @return $ModuleGranule : 从源文档分析出来的语法树仅拥有模块颗粒的抽象地位
         */
        $ModuleGranule constructSyntaxTree( tokens& is, Lengine::logs& log );

        /**
         * @method detectModuleSignature : 检测模块签名
         * @desc :
         *  从记号序列检测合法的模块签名
         *  此方法用于在模块依赖分析阶段以更小的开销
         *  分析模块的依赖关系
         * @param is : 记号序列
         * @param log : 日志器
         * @param mname : 用于承接模块名称的输出参数
         * @param deps : 用于承接模块依赖关系的输出参数
         * @return $ModuleSignature : 若分析成功,此处产生一个模块签名
         */
        $ModuleSignature detectModuleSignature( tokens& is, Lengine::logs& log );
        $ModuleSignature constructModuleSignature( tokens::iterator& it, Lengine::logs& log, $scope scope );

        /**
         * @method constructDependencyDescriptor : 从记号序列构建依赖描述符
         * @desc :
         *  从记号序列构建一个依赖描述符
         * @param it : 记号输入序列迭代器
         * @param log : 日志器
         * @param scope : 用于寻找文档路径的作用域
         * @return $depdesc : 构建的依赖描述符
         */
        $depdesc constructDependencyDescriptor( tokens::iterator& it, Lengine::logs& log, $scope scope );

        /**
         * @method construct* : 各语法结构的构建方法
         * @desc :
         *  从记号序列构建各种语法结构
         * @param it : 记号序列迭代器
         * @param log : 日志器
         * @param scope : 作用域
         * @return $* : 各种语法结构的代理
         */
        $MethodImpl constructMethodImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope );
        $ClassDef constructClassDefinition( tokens::iterator& it, Lengine::logs& log, $scope scope, int wn );
        $AttrDef constructAttributeDefinition( tokens::iterator& it, Lengine::logs& log, $ClassDef sc, int wn );
        $EnumDef constructEnumDefinition( tokens::iterator& it, Lengine::logs& log, $scope scope, int wn );
        $MethodDef constructMethodDefinition( tokens::iterator& it, Lengine::logs& log, $scope scope, int wn );
        $InsBlockImpl constructInstructionBlockImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope );
        $FlowCtrlImpl constructFlowControlImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope );
        $ExpressionImpl constructExpressionImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope );
        $ConstructImpl constructConstructImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope );
        $LoopImpl constructLoopimplementation( tokens::iterator& it, Lengine::logs& log, $scope scope );
        $BranchImpl constructBranchImplementation( tokens::iterator& it, Lengine::logs& log, $scope scope);
        /**
         * @method constructNameUseCase : 构建名称用例
         * @desc :
         *  构建一个名称用例
         * @param it : 记号输入序列迭代器
         * @param log : 日志器
         * @param sc : 作用域
         * @param absorb : 是否将'<'视为模板参数列表
         * @return nameuc : 名称用例
         */
        nameuc constructNameUseCase( tokens::iterator& it, Lengine::logs& log, $scope sc, bool absorb );
        
        int prio(const token& it)const;
};

}

#endif