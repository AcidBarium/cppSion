#pragma once

#include <memory>
#include <string>
#include <vector>

struct Type
{
    enum class Kind
    {
        Builtin,
        Pointer,
        Array
    };

    enum class Builtin
    {
        Void,
        Int,
        Double,
        Bool
    };

    Kind kind = Kind::Builtin;
    Builtin builtin = Builtin::Int;
    std::shared_ptr<Type> element; // for pointer/array
    std::size_t arraySize = 0;     // for array

    static Type makeBuiltin(Builtin b)
    {
        Type t;
        t.kind = Kind::Builtin;
        t.builtin = b;
        return t;
    }

    static Type makePointer(const Type &elem)
    {
        Type t;
        t.kind = Kind::Pointer;
        t.element = std::make_shared<Type>(elem);
        return t;
    }

    static Type makeArray(const Type &elem, std::size_t size)
    {
        Type t;
        t.kind = Kind::Array;
        t.element = std::make_shared<Type>(elem);
        t.arraySize = size;
        return t;
    }
};

struct FunctionSignature
{
    std::string name;
    Type returnType;
    std::vector<Type> parameters;
};

class TypeRegistry
{
public:
    Type intType() const { return Type::makeBuiltin(Type::Builtin::Int); }
    Type doubleType() const { return Type::makeBuiltin(Type::Builtin::Double); }
    Type boolType() const { return Type::makeBuiltin(Type::Builtin::Bool); }
    Type voidType() const { return Type::makeBuiltin(Type::Builtin::Void); }
};

inline bool operator==(const Type &a, const Type &b)
{
    if (a.kind != b.kind)
    {
        return false;
    }

    if (a.kind == Type::Kind::Builtin)
    {
        return a.builtin == b.builtin;
    }

    if (a.kind == Type::Kind::Pointer)
    {
        return a.element && b.element && *a.element == *b.element;
    }

    if (a.kind == Type::Kind::Array)
    {
        return a.arraySize == b.arraySize && a.element && b.element && *a.element == *b.element;
    }

    return false;
}

inline bool operator!=(const Type &a, const Type &b)
{
    return !(a == b);
}
