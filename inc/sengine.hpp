#ifndef __sengine__
#define __sengine__

#include "imm.hpp"
#include "modesc.hpp"
#include "module.hpp"
#include "attrdef.hpp"
#include "loopimpl.hpp"
#include "methoddef.hpp"
#include "methodimpl.hpp"
#include "branchimpl.hpp"
#include "operatordef.hpp"
#include "flowctrlimpl.hpp"
#include "operatorimpl.hpp"
#include "typeconvertdiagram.hpp"
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <memory>

namespace alioth {
using namespace llvm;
/**
 * @class Sengine : 语义分析引擎
 * @desc :
 *  语义引擎是前桥的末端,它被用于封装后桥实现细节,桥接管理器与后桥
 * 
 * [THOUGHT] 2019/03/30 : 由于没有friend关键字,设想设计如下语法特性,类C的实例对类C::D的实例的任何成员都有无限制的访问权限
 *  这样才使得在类中定义类的语法特性更有价值
 * 
 * [PLAN:DONE 2019/03/31] : 给类属性添加书写顺序索引,如此便可以不依照书写顺序阻止属性,而是依照其逻辑层次,将元属性归为一个整体
 */
class Sengine {

    private:
        /** 产生符号时,可选的后缀 */
        enum Decorate { None, Meta, Entity };

        /** 搜索语法树时,决定搜索滤镜 */
        enum Len{ ThisClass, SuperClass, NormalClass };

        /** 检查数据类型兼容性时,判断场景 */
        enum Situation{ Passing, Calculating, Returning };

        /** 检查表达式语义时，表达式所处的位置特征 */
        enum Position{
            LeftOfAssign,   //在赋值运算符左侧
            BeforeMember,   //在成员运算符左侧
            AsInit,         //作为初始值
            AsProc,         //作为过程
            AsParam,        //作为参数
            AsOperand,      //作为通常的运算子
            AsRetVal,       //作为返回值
            Floating };     //漂浮于指令块
        
    public:

        /**
         * @class ModuleTrnsUnit : 模块翻译单元
         * @desc :
         *  模块翻译单元是语义引擎对后桥处理单元的封装
         *  以前桥产生的抽象语法树为原料,产生后桥能处理的单元
         *  管理器可以以模块翻译单元为单位管理
         */
        using ModuleTrnsUnit = std::shared_ptr<Module>;

    private:
        
        /**
         * @member mctx : llvm上下文环境
         * @desc :
         *  llvm上下文环境可以用于全局保存所有的类型信息
         */
        LLVMContext mctx;

        /**
         * @member mtmachine : llvm目标机器
         * @desc :
         *  在产生目标代码时需要使用的资源
         */
        TargetMachine* mtmachine;

        /**
         * @member mttraiple : 目标平台描述
         * @desc :
         *  此描述在生成目标平台代码时使用
         */
        string mttraiple;

        /**
         * @member mcurmod : 当前模块
         * @desc :
         *  当执行实现语义分析时,此成员被视为当前实现所在的模块
         */
        ModuleTrnsUnit mcurmod;

        /**
         * @member mlrepo : 日志仓库
         * @desc :
         *  语义分析过程中产生的所有日志都存储在这里
         */
        Lengine::logr mlogrepo;

        /**
         * @member mrepo : 模块仓库
         * @desc :
         *  所有需要进行语义分析的模块
         */
        map<$modesc,$module> mrepo;

        /**
         * @member mtrepo : 翻译单元仓库
         * @desc :
         *  翻译单元仓库保存所有产生的翻译单元
         */
        map<$modesc,ModuleTrnsUnit> mtrepo;

        /**
         * @member mlocalV : 局部元素表
         * @desc :
         *  由构建语句产生的元素在此绑定
         *  此表可以用于判断执行流是否已经掠过构建语句
         */
        map<$ConstructImpl,Value*> mlocalV;

        /**
         * @member mnamedT : 命名类型符号表
         * @desc :
         *  此表用于登记所有具名数据类型
         *  任何情况下都可以为此表填充新的元素
         *  若此表中元素已经存在,不能创建新的类型
         */
        map<string,Type*> mnamedT;

        /**
         * @member mmethodP : 方法原型表
         * @desc :
         *  此表用于缓冲方法与其原型的关系
         *  此表仅应当被特定的函数维护,不可直接访问
         */
        map<$MethodImpl,$MethodDef> mmethodP;

        /**
         * @member mtcd : 类型转换图
         * @desc :
         *  用于存放所有数据类型之间的转换关系
         */
        TypeConvertDiagram mtcd;

        /**
         * @member flag_terminate : 终结标志
         * @desc :
         *  当分析了流控制语句,设置终结标志
         *  当因为终结标志的存在,跳过了一个终结指令的生成,清除终结标志
         *  当终结标志存在,其他
         *  指令都被跳过
         */
        bool flag_terminate;

    private:

        /**
         * @method performDefinitionSemanticValidation : 执行语义检查
         * @desc :
         *  检查模块语义,产生所有后桥能使用的类型定义
         */
        bool performDefinitionSemanticValidation( $modesc mod );

        /**
         * @method performDefinitionSemanticValidation : 执行定义语义检查
         * @desc :
         *  检查类定义的有效性,产生后桥类型,此动作会同时产生对象类型和元类型
         * @param clas : 类定义
         * @return bool : 动作是否成功
         */
        bool performDefinitionSemanticValidation( $ClassDef clas );

        /**
         * @method performImplementationSemanticValidation : 执行实现语义检查
         * @desc :
         *  为类生成元对象,作为后端全局变量存在
         * @param clas : 类定义
         * @return bool : 动作是否成功
         */
        bool performImplementationSemanticValidation( $ClassDef clas );

        /**
         * @method performImplementationSemanticValidation : 执行实现语义检查
         * @desc :
         *  一个通用函数,用于处理块中可见的任何实现
         */
        bool performImplementationSemanticValidation( $implementation impl, IRBuilder<>& builder, Position pos = Floating );

        /**
         * @method performImplementationSemanticValidation : 执行实现语义检查
         * @desc :
         *  为指令块产生中间表示
         *  在离开指令块之前,任何中间表示都应该产生"离开块"语句块,此语句块用于销毁在
         *  指令块中产生的元素
         */
        bool performImplementationSemanticValidation( $InsBlockImpl impl ,IRBuilder<>& builder);

        /**
         * @method performImplementationSemanticValidation : 执行实现语义检查
         * @desc :
         *  为流控制语句执行语义分析,并产生流控制语句中间表示
         *  由于不论任何类型的流控制语句,都会离开当前的基本块,所以流控制语句产生的中间表示
         *  一定需要产生离开块语句
         */
        bool performImplementationSemanticValidation( $FlowCtrlImpl impl ,IRBuilder<>& builder);

        /**
         * @method performImplementationSemanticValidation : 执行实现语义检查
         * @desc :
         *  为表达式执行语义分析,返回表达式产生的中间结果
         * @param pos : 指定表达式的地位，在语义分析时，倾向于使表达式的结果在接下来的使用中有效。
         */
        $imm performImplementationSemanticValidation( $ExpressionImpl impl ,IRBuilder<>& builde, Position pos );
        imms processNameusageExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos );
        imms processMemberExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos );
        $imm processAssignExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos );
        $imm processValueExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos );
        $imm processCallExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos );
        $imm processCalcExpression( $ExpressionImpl impl, IRBuilder<>& builder, Position pos );

        
        /**
         * @method performImplementationSemanticValidation : 执行语义检查
         * @desc :
         *  对构造语句执行语义分析,此行为会改变localV符号表的内容
         *  进而会影响到generateLeaveBlock方法或generateLeaveMethod函数的行为
         */
        bool performImplementationSemanticValidation( $ConstructImpl impl ,IRBuilder<>& builder);
        
        /**
         * @method performImplementationSemanticValidation : 执行语义分析
         * @desc :
         *  对分支语句进行语义分析,产生跳转指令和对应的块,并把builder插入到分支结束之后的块中
         */
        bool performImplementationSemanticValidation( $BranchImpl impl ,IRBuilder<>& builder );

        /**
         * @method performImplementationSemanticValidation : 执行语义分析
         * @desc :
         *  对循环语句进行语义分析,产生循环跳转块并将builder插入到循环结束的基础块中
         */
        bool performImplementationSemanticValidation( $LoopImpl loop, IRBuilder<>& builder );

        /**
         * @method performDefinitionSemanticValidation : 执行定义语义检查
         * @desc :
         *  所有方法的实现语义分析都应当在方法实现的语义分析之前被执行
         *  以此生成方法入口
         */
        bool performDefinitionSemanticValidation( $MethodDef method );

        /**
         * @method preformDefinitinoSemanticValidation : 执行定义语义检查
         * @desc :
         *  分析运算符定义语义检查
         */
        bool performDefinitionSemanticValidation( $OperatorDef opdef );

        /**
         * @method performImplementationSemanticValidation : 执行实现语义检查
         * @desc :
         *  为方法定义生成方法入口
         *  元方法以类实体引用作为this参数
         *  实例方法以类实例引用作为this参数
         *  返回结构体的方法使用额外的指针参数传递数据
         */
        bool performImplementationSemanticValidation( $MethodImpl method );

        /**
         * @method performDefinitionSemanticValidation : 执行定义语义检查
         * @desc :
         *  为属性定义执行语义检查,属性不能为右值
         * @return Type* : 若成功,返回用于组成复合数据类型的类型指针
         */
        Type* performDefinitionSemanticValidation( $AttrDef attr );

        /**
         * @method generateTypeUsageAsAttr : 为属性产生类型
         * @desc :
         *  为属性产生数据类型用例
         *  对于命名数据类型,若不可达或不唯一均失败,但是不检查源定义的语义
         *  若失败,日志会被写入日志仓库
         */
        Type* generateTypeUsageAsAttribute( $eproto proto );

        /**
         * @method generateTypeUsageAsParameter : 为参数生成类型
         * @desc :
         *  为参数产生数据类型用例
         *  对于复合数据类型的变量,使用指针传递参数
         *  若失败,日志会被写入仓库
         */
        Type* generateTypeUsageAsParameter( $eproto proto );

        /**
         * @method generateTypeUsageAsReturnValue : 为返回值生成类型
         * @desc :
         *  若返回值类型为VAR NAMED,则使用指针在参数中开辟空间返回,并返回数字
         * @param proto : 元素原型
         * @param pts : 若方法返回结构体，函数会为参数列表追加指针
         */
        Type* generateTypeUsageAsReturnValue( $eproto proto, vector<Type*>& pts );

        /**
         * @method generateTypeUsage : 产生数据类型
         * @desc :
         *  若目标不可达,则报告错误,日志被写入日志仓库
         */
        Type* generateTypeUsage( $typeuc type );

        /**
         * @method determineTypeUsage : 推导数据类型
         * @desc :
         *  查询语法树,寻找命名数据类型对应的类定义
         *  若类定义不存在或存在多个,查询失败,日志被写入日志仓库
         * @param type : 若类型是命名的数据类型,则需要推导,其他直接返回
         *  若推导成功,源数据类型会被修改为CompositeType以缓冲推导结果
         *  若失败,源数据类型被设置成UnsolvableType并返回nullptr
         * @param expr : 待推导数据类型的表达式
         * @return : 推导的数据类型
         */
        $typeuc determineDataType( $typeuc type );
        // $typeuc determineDataType( $ExpressionImpl expr );
        $eproto determineElementPrototype( $eproto );

        /**
         * @method generateGlobalUniqueName : 产生全局唯一名称
         * @desc :
         *  为语法结构产生全局唯一名称
         */
        string generateGlobalUniqueName( $node, Decorate = None );

        /**
         * @method request : 请求语法结构
         * @desc :
         *  从语法树向上请求语法结构,此方法可以用于检查引用可达性,也可以用于获取目标语法结构
         *  对于同名语法结构,比如重载的方法,返回结果不止一个
         *  此方法会自动将搜索过程中产生的日志送入日志仓库,回送结果不包含日志
         *  使用不同的滤镜可以对不同身份的作用域区别搜索
         * @param name : 要搜索的名称
         * @param len : 滤镜
         *  ThisClass : 对类作用域有效,实现所属的当前类,搜索其中的成员,和基类,内部定义
         *  SuperClass : 对于基类,搜索基类和成员
         *  NormalClass : 只匹配内部定义
         *  注: 滤镜只影响向上搜索行为,而所有的向下搜索行为都不受影响
         *      通常情况下,request递归调用时会自动设置滤镜,不需要手动操作
         * @param sc : 要搜索的作用域,若此值为空,则从name中提取作用域.
         * @return everything : 返回若干语法结构
         */
        everything request( const nameuc& name, Len len, $scope sc = nullptr );

        /**
         * @method requestThisClass : 请求当前类
         * @desc :
         *  此方法使用request方法,便捷地获取实现的当前类
         *  此方法尝试使用mmethodP表提高查询速度,但绝不修改mmethodP表
         *  若存在任何问题,只返回空,不存储日志
         */
        $ClassDef requestThisClass( $implementation impl );

        /**
         * @method requestPrototype : 请求原型
         * @desc :
         *  此方法维护和查询mmethodP表,寻找实现所在的方法的原型
         */
        $MethodDef requestPrototype( $implementation met );

        /**
         * @method requestThis : 请求宿主
         * @desc :
         *  使用方法实现符号获取方法入口
         *  使用方法的第一个参数作为返回值
         */
        Value* requestThis( $implementation impl );

        /**
         * @method checkEquivalent : 检查类型等价性
         */
        bool checkEquivalent( $eproto dst, $eproto src );
        bool checkEquivalent( $typeuc dst, $typeuc src );

        /**
         * @method checkCompatibility : 检查数据类型兼容性
         * @desc :
         *  在给定条件下,判断两种数据类型是否相互兼容,返回一条转换路径单链表
         *  若中途出现其他错误,则返回空
         * @param dst : 目标数据类型
         * @param src : 源数据类型
         * @param s : 情况
         * @return $tcp : 若检测失败,则返回空,否则返回路径链表头代理
         *  注意区分[不可转换]与[检测失败]两种情况的不同.
         */
        // $tcp checkCompatibility( $typeuc dst, $typeuc src, Situation s );
        // $tcp checkCompatibility( $eproto dst, $eproto src, Situation s );

        /**
         * @method tcd_get_node : 获取类型转换图上的节点
         * @desc :
         *  此方法要determine数据类型
         *  然后调用checkEquivalent检查全等性
         *  若存在全等的数据类型，则返回之
         *  若不存在全等的数据类型，则创建一个返回之。
         *  若数据类型无效，则返回空
         */
        $typeuc tcd_get_node( $typeuc t );

        /**
         * @method tcd_add_edge : 添加一条边
         * @desc :
         *  此方法会调用tcd_get_node 来确定节点
         *  若已经存在一模一样的边，则无动作，返回成功
         *  若数据类型无效，则返回失败
         */
        bool tcd_add_edge( $typeuc dst, $typeuc src, ConvertAction ca );

    public:

        /**
         * @constructor : 构造函数
         * @desc :
         *  初始化环境
         */
        Sengine();

        /**
         * @method loadModuleDefinition : 装载模块定义
         * @desc :
         *  此方法将模块描述符装入语义分析器,并整理语义结构
         * @param mod : 要装入的模块
         * @return int : 是否成功装入了此模块,若此模块已经装入了,则返回成功,无动作.
         *  0: 装入失败
         *  1: 装入成功
         *  2: 装入成功，发现入口
         */
        int loadModuleDefinition( $modesc mod );
        
        /**
         * @method performDefinitionSemanticValidation : 执行定义语义校验
         * @desc :
         *  对已装入的所有模块进行全局的定义语义检查
         *  此阶段会产生LLVM能使用的所有类型信息
         * @return bool : 是否通过语义校验
         */
        bool performDefinitionSemanticValidation();

        /**
         * @method performImplementationSemanticVlidation : 执行实现语义分析
         * @desc :
         *  执行语义分析,将产生模块翻译单元
         *  此阶段产生模块中所有的函数和全局变量
         *  此阶段需要使用前一阶段产生的类型信息
         * @param descs : 目标模块
         * @return ModuleTrnsUnit : 翻译单元
         */
        ModuleTrnsUnit performImplementationSemanticValidation( $modesc desc, Dengine& dengine );

        /**
         * @method triggerBackendTranslation : 触发后端翻译
         * @desc :
         *  触发后端翻译,将结果送入文件描述符指定的文件中
         * @param unit : 要翻译的单元
         * @param fd : 文件描述符
         * @param dengine : 文档引擎
         * @return bool : 是否成功
         */
        bool triggerBackendTranslation( ModuleTrnsUnit unit, Dengine::vfdm fd, Dengine& dengine );

        /**
         * @method getLog : 获取所有日志
         * @desc :
         *  所有步骤所产生的日志都被记录在日志容器,可以统一提取
         * @return Lengine::logr : 日志仓库
         */
        Lengine::logr getLog();
};

}

#endif