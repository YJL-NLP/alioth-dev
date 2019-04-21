# The Alioth programming language

> The is the offical repository of the Alioth project.  
> Home Page : https://dn-ezr.cn  
> Offical dev repository on GitHub : https://github.com/dn-ezr/alioth-dev  
> Offical dev repository on personal server : http://dn-ezr.cn:3000/dn-ezr/alioth-dev  
> And the repository of the original project : http://dn-ezr.cn:3000/dn-ezr/alioth

Most of the documents are written in Chinese currently, here's our apologies for your might existing trouble in reading them, the version in English coming soon.

![](doc/assets/i/icon_with_A.png)

Alioth编程语言是一种编译型静态强类型编程语言,目前基于LLVM作为后桥支持,生成机器码.

Alioth项目的目的是设计更简单,更高效的高级编程语言,并且对底层开发有良好的支持.

Alioth项目的主要工作包括Alioth语言的设计,Alioth编译器的实现,Alioth基础库的实现和Alioth辅助支持插件的实现.

Alioth编译器尚在开发阶段,如下提及的语法特性还没有完全得到支持或尚存在错误.

## 准备开发环境

1. 安装依赖环境

   1. 当前版本的Alioth编译器依赖于`LLVM-8.0.0`来生成机器码,您可能需要安装`LLVM8.0.0`开发环境

        ~~~bash
        #!/bin/bash

        # 获得llvm源码
        wget http://releases.llvm.org/8.0.0/llvm-8.0.0.src.tar.xz
        tar -xJf llvm-8.0.0.src.tar.xz
        mkdir llvm-8.0.0-src/build
        cd llvm-8.0.0-src/build

        # 开启必要的选项,准备构建环境
        cmake -DCMAKE_INSTALL_PREFIX=/usr -DLLVM_ENABLE_CXX1Y=ON -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_ENABLE_PEDANTIC=OFF ..

        # 使用多个任务流加速构建
        make -j

        # 安装llvm
        sudo make install
        ~~~

    2. 安装其他构建工具

        ~~~bash
        #!/bin/bash

        sudo apt-get install nasm make g++-8
        ~~~

2. 编译及安装Alioth

    ~~~bash
    #!/bin/bash

    #生成开发所必须的路径
    make initial

    #编译并安装
    make install
    ~~~


## 项目结构

* .vscode  
常见的能用于正常工作的vscode配置  
* app  
存放基于Alioth语法库开发的Alioth工具的源代码  
* doc  
存放与Alioth相关的文档与文案  
* inc  
Alioth语法库的头文件  
* root  
用于模拟Alioth根空间的路径
* src  
用于存放Alioth语法库的源代码  
* vsc-extensions  
用于存放vscode扩展的开发环境  

## Alioth的语法特性

Alioth最迷人的,就是她灵活自由的语法设计.

Alioth在**运算符**和**语句**的设计上花样百出,提供了大量可供使用的语法糖.

### 摘要

* 元素  
Alioth操作内存中对象所需的**媒介**,类似于C语言中**变量**的概念.
* 准方法  
参数数据类型未指定的方法.
* 虚方法  
返回值数据类型不唯一的方法.
* 模板字符串  
在字符串中使用占位符插入表达式.
* 构造运算符  
使用构造表达式调用类的构造运算符,使对象的构造更灵活优雅.
* 移动运算符  
当对象被闭包,地址发生变化时,调用移动运算符.
* 正反运算符  
影响表达式中,运算符重载宿主与运算符的位置关系.
* 成员运算符  
拦截对类成员的设置和提取动作,进行相应的处理.
* 列赋值语句  
使用逗号构造列,用于直接获取元组中的元素.
* 类型假设语句  
对未知数据类型的元素进行假设,创造数据类型确定的语境.
* 执行流展开语句  
将任务展开到多个不同的执行流执行.
* 可回滚的枚举  
基于数字的枚举可选的可以支持回滚.
* 谓词及谓词引导的定义分支
基于谓词的选择性定义

### 构造运算符

Alioth中,对象不通过**构造方法**构造,而是通过**构造运算符**进行构造

构造运算符的设计使得对象的构造更加灵活,优雅.

~~~alioth
module Cons

class Person {

    enum public Gender {
        MALE
        FEMALE
    }

    var static nlist map<string,ptr Person>;

    var mname       string;
    var mage        int32;
    var mgender     Gender;
    var mtags       list<string>;

    operator {} ( name string, age int32, gender Gender = MALE, tags list<string> = [] ) { 
        mname:name, mage:age, mgender:gender |
        if( nlist.has(name) ) !! "This person named $(name) is already existing";
        nlist << &this;
    }
}

method entry main() int32 {
    var person_one Person = {
        name:"Eric",
        age:21
    };

    var person_two = {Person|name:"Julia",gender:Person::FEMALE,age:21};

    var person_tree = {Person|
        name:"Jack",
        age:21,
        tags:["cool","dalao"]
    }
}
~~~

上述代码简单的展示了构造运算符的定义和使用,实际上构造运算符拥有如下特性:

* 构造运算符使用"{}"调用,其中参数的顺序可以打乱
* 列构造运算符使用"[]"调用,接受的实际上是可变参数包
* 当构造表达式的类型可以推导,构造表达式中的类型声明可省
* 字符串拥有特殊的使用双引号的构造表达式
* 构造运算符可以携带初始化列表,用于指定类成员的构造表达式

实际上,一共存在四种不同作用的构造运算符,它们的定义方法如下:

~~~alioth
operator {} ( ...... );     //构造运算符,使用"{}"调用
operator {=} ( another ); //拷贝构造运算符,带有拷贝语义,隐式调用
operator {<<} ( another ); //移动构造运算符,带有移动语义,隐式调用
operator [...] ( pack );    //列构造运算符,使用"[]"调用
~~~

除此之外,您可能比较关心上述例子代码中构造运算符重载的实现体中奇怪的语法.

~~~alioth
......
    operator {} ( name string, age int32, gender Gender = MALE, tags list<string> = [] ) { 
        mname:name, mage:age, mgender:gender |
        if( nlist.has(name) ) !! "This person named $(name) is already existing";
        nlist << name;
    }
......
~~~

实际上,构造运算符重载的实现体并不完全由可执行语句构成

* 构造运算符的实现体由**初始化列表**和**构造指令表**构成,使用'|'分割
* 若**构造指令表**为空,则'|'可省,否则'|'必须存在
* **初始化列表**未提及的成员元素使用默认构造运算符构造
* 当有且仅有一个**构造运算符**可以仅接受一个类型为**T**的参数时,对象可以直接使用不带大括号的类型为**T**的表达式**E**构造.

### 准方法与假设语句

当**方法**的某几个参数的**数据类型**在定义时并未给出,此方法是一个**准方法**.

在**准方法**中,您可以通过**假设**语句对参数的数据类型进行猜测,由此在假设成立的分支中,参数的数据类型可以被视为已知.

当产生一个对**准方法**的调用时,准方法的参数数据类型由于传参而被确定,编译器可以推断出哪些**假设**语句的分支必然执行,哪些必然跳过,由此产生一个参数数据类型确定的方法实体,以供调用.

当一个**准方法**被**闭包**,对它的调用从**符号调用**变成了**指针调用**,编译器也因此无法获悉准方法的语法树结构,从而无法提取参数数据类型确定的方法体以供调用.

因此,当**准方法**被**闭包**时,会产生一个**准方法对象**,准方法对象的参数数据类型也是未指定的,但是**准方法对象**中的**假设**语句被编译成了一种基于**RTTI**的类型判断语句,其开销等同于对数字进行全等判断的**if**语句.

~~~alioth
......

class Adaptor {
    method config( var conf ) bool {
        assume conf as string {
            var is = io::istream(conf);
            if( !is ) return false;

            var j = {Json|json:is};
            if( j.type != Json::Object ) return false;

            return config(j);
        } otherwise assume conf as Json {
            if( conf.type != Json::Object ) return false;

            ....
            ....
            ....

            return true;
        }
    }
}

......
~~~

* **assume**和**otherwise**关键字用于引导**类型假设**语句
* **类型假设语句**中**as**用于判断元素所指对象的数据类型是否继承于目标类型
* **类型假设语句**中**as!** 用于判断元素数据类型是否继承于目标类型
* 准方法是至少一个参数的数据类型待推导的方法
* 调用准方法时,所有参数的数据类型必须确定
* 调用准方法时,若存在一个参数,其数据类型不能被任何**类型假设**分支匹配,则产生语法错误.

### 虚方法

当**方法**返回值的**数据类型**未指定,方法为**虚方法**.

虚方法使用**RTTI**机制,允许一个方法可能返回多个类型的数据.

在底层实现中,**虚方法**比定义时多一个**指针参数**,指向由**调用者**在其**栈**中开辟的用于存储**虚方法**的**返回值对象**的内存空间.

由于虚方法的所有**返回语句**都**已知**,所以虚方法可能返回的对象**最大所需的空间大小**可知,调用者开辟的空间应该足以容纳这个尺寸的对象.

~~~alioth
......

class Json {
    class public Nothing {}

    method static fromJsonStream( ref json istream ) {
        var ret_value Json;

        ......
        //Syntax error occures
        return {Nothing|};
        ......

        return ret_value;
    }
}

......
~~~

* 虚方法的返回值**数据类型**不指定,但**元素类型**是确定的
* 虚方法的返回值可以有任何类型,调用者使用**假设**语句匹配其类型.
* 虚方法的底层实现使用**rax**寄存器返回**类型ID**,使用指针参数**构造返回的对象**  
所以如果返回符合类型,其开销完全没有变化,如果返回基础数据类型,则开销相比之下有所增加.
* 若**虚方法**无论如何总是返回**同一种数据类型**,它会自动退化成为普通方法

### 执行流展开语句

**Alioth**提供了一种语法结构,描述一种将执行流分散开,执行不同任务的行为.

这种语句称为**执行流展开语句**,由底层库对其进行支持.

~~~alioth
method mailto( var name string ) bool {
    return network::mailto ({
        address: "$(name)@dmail.com",
        content: "Hello there this is a group mail."
    });
}

method sayhi( ) {
    var names list<string> = ["Eric","lzl","Tracy"];
    [ names ] mailto > method( ref name string, var res bool ) {
        if( res ) io << "mail to $(name) succeeded";
        else io << "mail to $(name) failed";
    };
}
~~~

**执行流展开语句**的语法如下:

~~~ebnf
source = integer | container | tuple | vararg;
execution unwind = "[" target [":" host] "]" job ">" callback";";
~~~

* 在执行块中,使用"[]"引导执行流展开语句
* **target**表示**展开目标**  
**展开目标**可以是**可变参数包**,**元组**,**容器**或**整数**;  
若**展开目标**是整数,其语义是将执行流展开成**整数**份,也就是说**任务**将被调用**整数**次,接受的参数是**任务id**.  
否则,**执行流展开语句**为**展开目标**中的每个元素产生一个执行流,并为之调用**任务**.  
* **host**表示**展开宿主**,用于选择不同的执行流展开语句的底层实现,在跨平台开发时有用.
* **job**是**任务**  
**任务**必须能接受**展开目标**内容中的任何元素作为唯一的参数.  
根据选择的**展开宿主**不同,**任务**有可能在任何地方被执行  
**执行流展开语句**没有义务保证所有**任务**都同时在线,极端情况下,它们有可能被安排在一个**循环**里,顺次执行.
* **callback**是**回调**  
**回调**最多可以接受两个参数,每个执行流结束之后都被调起一次  
第一个参数,必须与**任务**所接受的参数原型相同  
第二个参数,若存在,必须与**任务**的返回值类型相同  
  * **回调**是原子操作,底层库保证一个**回调**执行时,不会被同源回调打断
  * **回调**一定在原线程的原执行流上被执行,把对于环境的关联都放在**回调**中,就能减少**跨越执行流的闭包**,确保数据安全.

