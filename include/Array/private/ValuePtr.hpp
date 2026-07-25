#ifndef VALUE_PTR_HPP
#define VALUE_PTR_HPP

#include <type_traits>
#include <cstdint>
#include <utility>
#include <algorithm>

#include "forward_declarations.hpp"

//******************************************************************************
// ICloneable Interface
//******************************************************************************

#define DECLARE_CLONE(TYPE) \
    auto clone() const -> TYPE* override { return new TYPE(*this); }

class ICloneable
{

public:

    [[nodiscard]] virtual auto clone() const -> ICloneable* = 0;

    virtual ~ICloneable() = default;

};

//******************************************************************************
// ValuePtr
//******************************************************************************

// ValuePtr class template

// Base template

template<ValuePtrPointee T>
class ValuePtr
{

public:

    ValuePtr() = default;

    ValuePtr(const ValuePtr<T> &other);
    ValuePtr(ValuePtr<T> &&other) noexcept;

    auto operator=(const ValuePtr<T> &other) -> ValuePtr&;
    auto operator=(ValuePtr<T> &&other) noexcept -> ValuePtr&;

    ~ValuePtr();

    explicit ValuePtr(T *p);

    operator bool() const;

    auto get() const noexcept -> T*;
    auto release() -> T*;

    auto swap(ValuePtr<T> &other) noexcept -> void;
    auto reset(T *p = nullptr) -> void;

    auto operator*() const -> T&;
    auto operator->() const -> T*;

    template<typename... Args>
    static auto make(Args&&... args) -> ValuePtr<T>;

private:

    T *m_p = nullptr;

};

// Specialization for arrays

template<ValuePtrPointee T>
class ValuePtr<T[]>
{

public:

    ValuePtr() = default;

    ValuePtr(const ValuePtr<T[]> &other);
    ValuePtr(ValuePtr<T[]> &&other) noexcept;

    auto operator=(const ValuePtr<T[]> &other) -> ValuePtr&;
    auto operator=(ValuePtr<T[]> &&other) noexcept -> ValuePtr&;

    ~ValuePtr();

    explicit ValuePtr(T *p, int64_t n);

    operator bool() const;

    auto get() const noexcept -> T*;
    auto release() -> std::pair<T*, int64_t>;

    auto swap(ValuePtr<T[]> &other) noexcept -> void;
    auto reset(T *p = nullptr, int64_t n = 0) -> void;

    auto size() const -> int64_t;

    auto operator[](int64_t index) const -> T&;

    static auto make(int64_t n) -> ValuePtr<T[]>;

private:

    T *m_p = nullptr;
    int64_t m_n = 0;

};

// Base template

template<ValuePtrPointee T>
ValuePtr<T>::ValuePtr(const ValuePtr<T> &other)
{

    ValuePtr<T> copy;

    if constexpr (std::derived_from<T, ICloneable>)
    {
        copy.reset (
            other ? other->clone() : nullptr
        );
    }
    else
    {
        copy.reset (
            other ? new T(*other) : nullptr
        );
    }

    swap(copy);
}

template<ValuePtrPointee T>
ValuePtr<T>::ValuePtr(ValuePtr<T> &&other) noexcept
{
    swap(other);
}

template<ValuePtrPointee T>
auto ValuePtr<T>::operator=(const ValuePtr<T> &other) -> ValuePtr<T>&
{
    ValuePtr<T> copy(other);
    swap(copy);
    return *this;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::operator=(ValuePtr<T> &&other) noexcept -> ValuePtr<T>&
{
    swap(other);
    return *this;
}

template<ValuePtrPointee T>
ValuePtr<T>::~ValuePtr()
{
    reset();
}

template<ValuePtrPointee T>
ValuePtr<T>::ValuePtr(T *p)
: m_p(p)
{}

template<ValuePtrPointee T>
ValuePtr<T>::operator bool() const
{
    return m_p != nullptr;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::get() const noexcept -> T*
{
    return m_p;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::release() -> T*
{
    T *p = m_p;
    m_p = nullptr;
    return p;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::swap(ValuePtr<T> &other) noexcept -> void
{
    std::swap(m_p, other.m_p);
}

template<ValuePtrPointee T>
auto ValuePtr<T>::reset(T *p) -> void
{
    delete m_p;
    m_p = p;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::operator*() const -> T&
{
    return *m_p;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::operator->() const -> T*
{
    return m_p;
}

template<ValuePtrPointee T>
template<typename... Args>
auto ValuePtr<T>::make(Args&&... args) -> ValuePtr<T>
{
    return ValuePtr<T>(new T(std::forward<Args>(args)...));
}

// Specialization for arrays

template<ValuePtrPointee T>
ValuePtr<T[]>::ValuePtr(const ValuePtr<T[]> &other)
{
    ValuePtr<T[]> copy;

    copy.reset (
        other ? new T[other.size()] : nullptr,
        other.size()
    );

    std::copy_n(other.get(), other.size(), copy.get());

    swap(copy);
}

template<ValuePtrPointee T>
ValuePtr<T[]>::ValuePtr(ValuePtr<T[]> &&other) noexcept
{
    swap(other);
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::operator=(const ValuePtr<T[]> &other) -> ValuePtr<T[]>&
{
    ValuePtr<T[]> copy(other);
    swap(copy);
    return *this;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::operator=(ValuePtr<T[]> &&other) noexcept -> ValuePtr<T[]>&
{
    swap(other);
    return *this;
}

template<ValuePtrPointee T>
ValuePtr<T[]>::~ValuePtr()
{
    reset();
}

template<ValuePtrPointee T>
ValuePtr<T[]>::ValuePtr(T *p, int64_t n)
: m_p(p), m_n(n)
{}

template<ValuePtrPointee T>
ValuePtr<T[]>::operator bool() const
{
    return m_p != nullptr;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::get() const noexcept -> T*
{
    return m_p;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::release() -> std::pair<T*, int64_t>
{
    auto p = std::make_pair(m_p, m_n);

    m_p = nullptr;
    m_n = 0;

    return p;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::swap(ValuePtr<T[]> &other) noexcept -> void
{
    std::swap(m_p, other.m_p);
    std::swap(m_n, other.m_n);
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::reset(T *p, int64_t n) -> void
{
    delete[] m_p;
    m_n = 0;

    m_p = p;
    m_n = n;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::size() const -> int64_t
{
    return m_n;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::operator[](int64_t index) const -> T&
{
    return m_p[index];
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::make(int64_t n) -> ValuePtr<T[]>
{
    return ValuePtr<T[]>(new T[n], n);
}

#endif // VALUE_PTR_HPP
