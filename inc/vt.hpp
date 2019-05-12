#ifndef __vt__
#define __vt__

namespace alioth {

/**
 * @enum-class VT : 终结符ID
 * @desc : 用于简单区分不同的终结符的枚举
 */
enum class VT {
    R_ERR,               //保留终结符,错误的文法符号
    R_BEG,               //保留终结符,文法开始
    R_END,               //保留终结符,文法结束

    SPACE,               //任何空白
    COMMENT,             //注释
    
    MODULE,              //模块声明
    ENTRY,               //入口声明

    /*元素关键字*/
    OBJ,                 //对象
    PTR,                 //指针
    REF,                 //引用
    REL,                 //重载
    
    METHOD,              //方法
    CLASS,               //类
    ENUM,                //枚举
    OPERATOR,            //运算符重载
    ASM,                 //汇编关键字

    /*基础数据类型*/
    INT8,
    INT16,
    INT32,
    INT64,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    FLOAT32,
    FLOAT64,
    BOOL,
    VOID,                    //空类型

    /*值及字面内容*/
    iNULL,                  //空指针
    iTRUE,                  //布尔值,真
    iFALSE,                 //布尔值,假
    iTHIS,                  //宿主对象引用
    iINTEGERb,              //二进制整数
    iINTEGERo,              //八进制整数
    iINTEGERn,              //十进制整数
    iINTEGERh,              //十六进制整数
    iFLOAT,                 //浮点数
    iCHAR,                  //单引号字符
    iSTRING,                //双引号字符串
    LABEL,                  //符号

    /*属性关键字*/
    CONST,                  //常量
    META,                   //概念的
    PUBLIC,PRIVATE,         //公开,私有
    //GETTER,SETTER,          //获取器,设置器 使用约定符替代
    CDECL,STDCALL,FASTCALL,THISCALL,    //调用约定
    //ABSTRACT,               //抽象关键字 使用约定符替代

    /*流控制关键字*/
    ASSUME,              //流控制关键字,引导假设语句
    OTHERWISE,           //流控制关键字,引导假设语句的旁支
    IF,                  //执行流控制关键字,如果
    ELSE,                //执行流控制关键字,否则
    LOOP,                //执行流控制关键字,循环
    BREAK,               //执行流控制关键字,跳出
    CONTINUE,            //执行流控制关键字,跳过
    RETURN,              //执行流控制关键字,返回
    SWITCH,              //执行流控制关键字
    CASE,                //执行流控制关键字
    DEFAULT,             //执行流控制关键字

    /*对象管理关键字*/
    NEW,                 //创建对象
    DELETE,              //删除对象

    /*语气符*/
    FORCE,               //强语义 !
    ASK,                 //弱语义 ?

    /*异常处理*/
    EXCEPTION,           //抛出异常

    /*界符*/
    WHERE,               //定位 #
    CONV,                //约定符号 $
    SCOPE,               //域符 ::
    COLON,               //冒号
    SEMI,                //分号
    COMMA,               //逗号
    OPENA,               //'(',打开参数列表
    CLOSEA,              //')',关闭参数列表
    OPENL,               //'[',打开定位
    CLOSEL,              //']',关闭定位
    OPENS,               //'{',打开域
    CLOSES,              //'}',关闭域
    AS,                  //'as',类型转换
    TREAT,               //'as!',类型对待
    AT,                  //'@',标签界定符

    /*运算符*/
    MEMBER,              //'.',成员运算符
    RANGE,               //'..',范围运算符
    ETC,                 //'...',可变参数包
    ASSIGN,              //赋值运算符
    ASSIGN_PLUS,         //加法赋值运算符
    ASSIGN_MINUS,        //减法赋值运算符
    ASSIGN_MUL,          //乘法赋值运算符
    ASSIGN_DIV,          //除法赋值运算符
    ASSIGN_MOL,          //求余赋值运算符
    ASSIGN_SHL,          //按位左位移赋值运算符
    ASSIGN_SHR,          //按位右位移赋值运算符
    ASSIGN_bAND,         //按位与赋值运算符
    ASSIGN_bOR,          //按位或赋值运算符
    ASSIGN_bXOR,         //按位异或赋值运算符
    PLUS,                //加法运算符
    //prePLUS,             //正号
    MINUS,               //减法运算符
    //preMINUS,            //负号
    MUL,                 //乘法运算符
    DIV,                 //除法运算符
    MOL,                 //求余运算符
    SHL,                 //按位左位移运算符
    SHR,                 //按位右位移运算符
    INCRESS,             //自增运算符
    DECRESS,             //自减运算符
    BITAND,              //按位与运算符
    BITOR,               //按位或运算符
    BITXOR,              //按位异或运算符
    BITREV,              //按位取反运算符
    AND,                 //逻辑与运算符
    OR,                  //逻辑或运算符
    NOT,                 //逻辑非运算符
    XOR,                 //逻辑异或运算符
    EQ,                  //相等比较运算符
    NE,                  //不等比较运算符
    LT,                  //小于比较运算符
    GT,                  //大于比较运算符
    LE,                  //小于等于比较运算符
    GE,                  //大于等于比较运算符
    //ADDRESS,             //取址运算符
    //REFER,               //间接引用运算符
};

}

#endif