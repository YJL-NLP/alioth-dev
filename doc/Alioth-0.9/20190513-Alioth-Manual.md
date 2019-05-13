---
title: Alioth Manual
date: 2019/05/13
author: GodGnidoc
encoding: utf8
language: Chinese
---

# 0. 序

截至本文档开始编辑的时候，Alioth编程语言已经拥有了超过**38**种语法特性，本文档的作用是对Alioth编程语言的特性进行全面的描述，提供一份可以用于学习**Alioth**编程语言的手册。

本文会尽可能详尽的讲解所有的语法特性，但读者最好已经具备了一定的编程基础，才能更轻松的阅读本文。

**截至本文撰写开始，Alioth编程语言的版本是`alioth-0.9`，本文介绍`alioth-0.9`的全部特性，不论编译器是否已经支持这种特性。**

[TOC]

# 1. 开始

Alioth编程语言的编译器可以在[Alioth官方网站](https://dn-ezr.cn)被找到，请注意，由于Alioth编程语言还在设计和实验的阶段，语法特性的改变一般不向下兼容。如果您下载的编译器适配语言版本高于当前版本，则很可能遇到一些语法特性不被支持的现象，您可以寻找最新的开发手册，或寻找开发手册对应的编译器版本。

让我们从**Hello world**开始,看看**Alioth**的源码如何被组织起来吧。

~~~alioth
module Hello entry SayHello : io

class Hello {
	method SayHello( argc int32, argv **int8 ) int32
}

method Hello SayHello( argc int32, argv **int8 ) int32 {
	io.println("Hello world!");
}
~~~

噢不，这看起来太糟糕了，一个**Hello world**居然需要这么多行代码！

不要担心，这是由于**Alioth**编程语言提供了一些语法盐来约束编码规范。这些语法盐在小规模的程序上看起来很笨重，很愚蠢，实际上在大规模的程序开发任务中，**Alioth**编程语言还是很轻便的。

让我们先来解读一下这份源代码的语义吧。



