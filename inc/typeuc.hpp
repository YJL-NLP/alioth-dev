#ifndef __typeuc__
#define __typeuc__

#include "agent.hpp"
#include "nameuc.hpp"

namespace alioth {

class typeuc;
class ClassDef;
class nameuc;
using $typeuc = agent<typeuc>;
using TypeID = unsigned long long;
using $ClassDef = agent<ClassDef>;

/**
 * @class typeuc : type use case, 类型用例
 * @desc :
 *  用于描述数据类型的类型用例
 */
class typeuc : public thing {

    private:
        static const TypeID TypeIdBitH =                0x80'00'00'00'00'00'00'00;
        static const TypeID TypeIdBitL =                0x00'00'00'00'00'00'00'01;

    public:
        static const TypeID DeterminedType =            (TypeIdBitH >> 0x00);
        static const TypeID UndeterminedType =          (TypeIdBitH >> 0x01);
        static const TypeID SimpleType =                (TypeIdBitH >> 0x02) | DeterminedType;
        static const TypeID PointerType =               (TypeIdBitH >> 0x03) | SimpleType;
        static const TypeID BasicType =                 (TypeIdBitH >> 0x04) | SimpleType;
        static const TypeID IntegerType =               (TypeIdBitH >> 0x05) | BasicType;
        static const TypeID FloatPointType =            (TypeIdBitH >> 0x06) | BasicType;
        static const TypeID UnsignedIntegerType =       (TypeIdBitH >> 0x07) | IntegerType;
        static const TypeID SignedIntegerType =         (TypeIdBitH >> 0x08) | IntegerType;

        static const TypeID UnknownType =               (TypeIdBitL << 0x00) | UndeterminedType;
        static const TypeID NamedType =                 (TypeIdBitL << 0x01) | UndeterminedType;
        static const TypeID UnsolvableType =            (TypeIdBitL << 0x02) | UndeterminedType;
        static const TypeID CompositeType =             (TypeIdBitL << 0x03) | DeterminedType;
        static const TypeID ConstraintedPointerType =   (TypeIdBitL << 0x04) | PointerType;
        static const TypeID UnconstraintedPointerType = (TypeIdBitL << 0x05) | PointerType;
        static const TypeID VoidType =                  (TypeIdBitL << 0x06) | BasicType;
        static const TypeID BooleanType =               (TypeIdBitL << 0x07) | BasicType;
        static const TypeID Uint8 =                     (TypeIdBitL << 0x08) | UnsignedIntegerType;
        static const TypeID Uint16 =                    (TypeIdBitL << 0x09) | UnsignedIntegerType;
        static const TypeID Uint32 =                    (TypeIdBitL << 0x0a) | UnsignedIntegerType;
        static const TypeID Uint64 =                    (TypeIdBitL << 0x0b) | UnsignedIntegerType;
        static const TypeID Int8 =                      (TypeIdBitL << 0x0c) | SignedIntegerType;
        static const TypeID Int16 =                     (TypeIdBitL << 0x0d) | SignedIntegerType;
        static const TypeID Int32 =                     (TypeIdBitL << 0x0e) | SignedIntegerType;
        static const TypeID Int64 =                     (TypeIdBitL << 0x0f) | SignedIntegerType;
        static const TypeID Float32 =                   (TypeIdBitL << 0x10) | FloatPointType;
        static const TypeID Float64 =                   (TypeIdBitL << 0x11) | FloatPointType;
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