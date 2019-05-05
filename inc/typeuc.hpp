#ifndef __typeuc__
#define __typeuc__

#include "agent.hpp"
#include "nameuc.hpp"

namespace alioth {

class typeuc;
class ClassDef;
class nameuc;
using $typeuc = agent<typeuc>;
using TypeID = unsigned long;
using $ClassDef = agent<ClassDef>;

/**
 * @class typeuc : type use case, 类型用例
 * @desc :
 *  用于描述数据类型的类型用例
 */
class typeuc : public thing {

    public:
        static const TypeID DeterminedType =            0b1000'0000'0000'0000'0000'0000;
        static const TypeID UndeterminedType =          0b0100'0000'0000'0000'0000'0000;

        static const TypeID UnknownType =               0b0000'0000'0000'0000'0000'0001 | UndeterminedType;           /** terminate */
        static const TypeID NamedType =                 0b0000'0000'0000'0000'0000'0010 | UndeterminedType;           /** terminate */
        static const TypeID UnsolvableType =            0x0000'0000'0000'0000'0000'0100 | UndeterminedType;           /** terminate */

        static const TypeID PointerType =               0b0000'0000'1000'0000'0000'0000 | DeterminedType;
        static const TypeID BasicType =                 0b0000'0000'0100'0000'0000'0000 | DeterminedType;
        static const TypeID CompositeType =             0b0000'0000'0010'0000'0000'0000 | DeterminedType;

        static const TypeID VoidType =                  0b0000'0000'0000'0000'0000'0001 | DeterminedType;             /** terminate */

        static const TypeID ConstraintedPointerType =   0b0000'0000'0000'0000'0000'0000 | PointerType;                /** terminate */
        static const TypeID UnconstraintedPointerType = 0b0000'0000'0000'0000'0000'0001 | PointerType;                /** terminate */

        static const TypeID IntegerType =               0b0000'0000'0000'1000'0000'0000 | BasicType;
        static const TypeID FloatPointType =            0b0000'0000'0000'0100'0000'0000 | BasicType;
        static const TypeID BooleanType =               0b0000'0000'0000'0000'0000'0001 | BasicType;                  /** terminate */

        static const TypeID UnsignedIntegerType =       0b0000'0000'0000'0000'1000'0000 | IntegerType;
        static const TypeID SignedIntegerType =         0b0000'0000'0000'0000'0100'0000 | IntegerType;

        static const TypeID Uint8 =                     0b0000'0000'0000'0000'0000'0001 | UnsignedIntegerType;        /** terminate */
        static const TypeID Uint16 =                    0b0000'0000'0000'0000'0000'0010 | UnsignedIntegerType;        /** terminate */
        static const TypeID Uint32 =                    0b0000'0000'0000'0000'0000'0100 | UnsignedIntegerType;        /** terminate */
        static const TypeID Uint64 =                    0b0000'0000'0000'0000'0000'1000 | UnsignedIntegerType;        /** terminate */

        static const TypeID Int8 =                      0b0000'0000'0000'0000'0000'0001 | SignedIntegerType;          /** terminate */
        static const TypeID Int16 =                     0b0000'0000'0000'0000'0000'0010 | SignedIntegerType;          /** terminate */
        static const TypeID Int32 =                     0b0000'0000'0000'0000'0000'0100 | SignedIntegerType;          /** terminate */
        static const TypeID Int64 =                     0b0000'0000'0000'0000'0000'1000 | SignedIntegerType;          /** terminate */

        static const TypeID Float32 =                   0b0000'0000'0000'0000'0000'0100 | FloatPointType;             /** terminate */
        static const TypeID Float64 =                   0b0000'0000'0000'0000'0000'1000 | FloatPointType;             /** terminate */

    public:
        TypeID id;
        nameuc name;
        anything sub;

    public:
        /**
         * @ctor : 构造函数
         * @desc :
         *  @form 1 : 构造拥有_id类型的数据类型
         *  @form 2 : 构造指针数据类型,指向s数据类型,constrainted决定是否被约束.
         *  @form 3 : 构造复合数据类型,def应当为类定义
         *  @form 4 : 构造具名数据类型,nm是名称用例
         *  @form 5 : 构造Unknown数据类型
         */
        typeuc( TypeID _id );
        typeuc( $typeuc s, bool constrainted );
        typeuc( $ClassDef def );
        typeuc( const nameuc& nm );
        typeuc() = default;
        typeuc( const typeuc& ) = default;
        typeuc( typeuc&& ) = default;
        ~typeuc() = default;

        typeuc& operator=( const typeuc& ) = default;
        typeuc& operator=( typeuc&& ) = default;

        /**
         * @method-set : 用于获取数据类型的简便方法
         * @desc :
         *  下述方法用于获取11种基础数据类型和指针类型以及空类型等数据类型描述结构
         */
        static $typeuc GetUnknownType();
        static $typeuc GetNamedType( const nameuc& nm );
        static $typeuc GetVoidType();
        static $typeuc GetBasicDataType( TypeID _id );
        static $typeuc GetBasicDataType( VT vt );
        static $typeuc GetPointerType( $typeuc _sub, bool constrainted = false );
        static $typeuc GetCompositeType( $ClassDef def );

        /**
         * @method getPointerTo : 获取指针数据类型
         * @desc:
         *  获取指向当前数据类型的指针数据类型
         * @param constrainted : 指针数据类型是否被约束为不可写
         * @return $typeuc : 若成功返回新的数据类型,继承当前数据类型的parse
         */
        $typeuc getPointerTo( bool constrainted = false );

        /**
         * @method getPointerDeepth : 深度
         * @desc :
         *  获取指针深度,若类型不是指针,则返回0
         */
        int getPointerDeepth()const;

        /**
         * @method is : 判断数据类型
         * @desc :
         *  判断数据类型是否满足要求
         */
        bool inline is( TypeID _id ) const {return (id&_id) == _id;}

        /**
         * @method setScope : 设置作用域
         * @desc :
         *  若有必要,为数据类型设置作用域
         */
        void setScope( $scope sc );
};

}

#endif