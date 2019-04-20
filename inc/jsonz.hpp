#ifndef __jsonz__
#define __jsonz__

/**
 * jsonz是为了无异常环境设计的json解析库
 * 
 * 由于没有异常机制的支持,jsonz的很多函数都不存在
 * 很明确的错误状态,使用者有责任在使用jsonz对象之前
 * 对对象的类型进行检查
 * 
 * 关于十六进制转义序列的规定
 *      json字符串中,若十六进制转义序列的高两位为0,则不会被插入最终的内容字符串中,只有低2位所代表的一个字节被插入
 *      json字符串内容中,若出现需要转义的字符串,最多只转义一个字节.
 * 
 * 本库使用Abon2.0语法,所有Abon1.0语法的文档都不能正常解析
 */

#include "chainz.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <functional>

using abon = std::vector<char>;

enum JType {
    JNull,
    JArray,
    JBoolean,
    JInteger,
    JObject,
    JReal,
    JString,
};

class Jsonz {

    private:
        JType   mtype;
        void*   mdata;
    
    public:
        Jsonz(JType type = JNull);
        Jsonz(const std::string& str);
        Jsonz( const Jsonz& an );
        Jsonz( Jsonz&& an );
        ~Jsonz();
        void clear();

        static Jsonz fromJsonStream( std::istream&, int* = nullptr );
        static Jsonz fromAbonStream( std::istream&, int* = nullptr );

        JType tell()const;
        bool is(JType)const;

        explicit operator int()const;   //若类型不匹配,返回0
        explicit operator double()const;//若类型不匹配,返回0
        explicit operator bool()const;  //若类型不匹配,返回false
        operator std::string()const;    //若类型不匹配,返回空字串

        //可以通过赋值运算改变Jsonz的类型
        Jsonz& operator=(const Jsonz&);
        Jsonz& operator=(Jsonz&&);
        Jsonz& operator=(bool);
        Jsonz& operator=(int);
        Jsonz& operator=(double);
        Jsonz& operator=(const std::string&);
        Jsonz& operator=(const char*);

        bool insert( const Jsonz&, int index = 0 );     //插入一个值,若对象不是数组,则失败,若位置超过范围,用null填充
        bool insert( Jsonz&&, int index = 0 );          //插入一个值,若对象不是数组,则失败,若位置超过范围,用null填充
        size_t count()const;                            //查容器内容总量,若对象不是容器,返回0
        size_t count( const std::string& )const;        //查看容器是否拥有某节点,若对象类型不匹配,则返回0
        Jsonz& operator[](const std::string& key);      //若对应对象不存在,则创建对象,若类型不匹配,引用地址为空.
        const Jsonz& at(const std::string& key)const;
        Jsonz& operator[](int);                         //若对应位置不存在,则使用null填充,若类型不匹配,引用地址为空
        const Jsonz& at(int)const;

        int foreach( std::function<bool(Jsonz&)> );
        int foreach( std::function<bool(const Jsonz&)> )const;
        int foreach( std::function<bool(const std::string&,Jsonz&)> );
        int foreach( std::function<bool(const std::string&,const Jsonz&)> )const;

        bool drop( int index );
        bool drop( const std::string& key );

        std::string toJson() const;
        abon toAbon() const;

        template<JType t, typename ...Args>
        bool test(const std::string& key, Args... args)const {
            if( count(key) == 0 ) return false;
            if( at(key).is(t) )
                return (... && test<t>(args));
            else
                return false;
        }
        template<JType t>
        bool turn(const std::string& key, std::function<void(const Jsonz&)> br, std::function<void()> el = nullptr )const {
            if( test<t>(key) and br ) {
                br(at(key));
                return true;
            } else if( el ) {
                el();
            }
            return false;
        }
        template<JType t>
        bool turn(const std::string& key, std::function<void(Jsonz&)> br, std::function<void()> el = nullptr ) {
            if( test<t>(key) and br ) {
                br(operator[](key));
                return true;
            } else if( el ) {
                el();
            }
            return false;
        }
};

#endif