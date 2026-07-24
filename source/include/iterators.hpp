#ifndef ITERATORS_HPP
#define ITERATORS_HPP

#include <type_traits>
#include <cstdint>

#include "helpers.hpp"

//******************************************************************************
// Iterator classes
//******************************************************************************

// IndexTupleIterator class

template<typename A, bool IsReadOnly>
class BasicIndexTupleIterator
{

public:

    using Element = A::Element;

    using AReference =
        std::conditional_t <
            A::is_owning_type(),
            std::conditional_t<IsReadOnly, const A&, A&>,
            const A&
        >;

    using APointer =
        std::conditional_t <
            A::is_owning_type(),
            std::conditional_t<IsReadOnly, const A*, A*>,
            const A*
        >;

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    explicit BasicIndexTupleIterator() = default;

    explicit BasicIndexTupleIterator (
        APointer p_a,
        const Extents<A::dimension()> &cursor = make_extents_filled<A::dimension()>(0)
    );

    auto operator*() const -> ElementReference;

    auto operator->() const -> ElementPointer;

    auto operator++() -> BasicIndexTupleIterator&;
    auto operator++(int) -> BasicIndexTupleIterator;

    auto operator--() -> BasicIndexTupleIterator&;
    auto operator--(int) -> BasicIndexTupleIterator;

    auto cursor() const -> const Extents<A::dimension()>&;
    auto cursor(const Extents<A::dimension()> &cursor) -> BasicIndexTupleIterator&;

    auto is_at_end() const -> const bool&;
    auto is_at_end(const bool &flag) -> BasicIndexTupleIterator&;

    auto p_a() const -> APointer;

    static auto begin_of(APointer p_a) -> BasicIndexTupleIterator<A, IsReadOnly>;
    static auto cbegin_of(APointer p_a) -> BasicIndexTupleIterator<A, true>;
    static auto end_of(APointer p_a) -> BasicIndexTupleIterator<A, IsReadOnly>;
    static auto cend_of(APointer p_a) -> BasicIndexTupleIterator<A, true>;

private:

    APointer                m_p_a       = nullptr;
    Extents<A::dimension()> m_cursor    = make_extents_filled<A::dimension()>(0);
    bool                    m_is_at_end = false;
};

template<typename AL, bool IsReadOnlyL, typename AR, bool IsReadOnlyR>
auto operator== (
    const BasicIndexTupleIterator<AL, IsReadOnlyL> &lhs,
    const BasicIndexTupleIterator<AR, IsReadOnlyR> &rhs
) -> bool
{
    return
        lhs.p_a()       == rhs.p_a()        &&
        lhs.cursor()    == rhs.cursor()     &&
        lhs.is_at_end() == rhs.is_at_end();
}

template<typename A, bool IsReadOnly>
BasicIndexTupleIterator<A, IsReadOnly>::BasicIndexTupleIterator(APointer p_a, const Extents<A::dimension()> &cursor)
: m_p_a(p_a),
  m_cursor(cursor),
  m_is_at_end(p_a->size() == 0)
{}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator*() const -> ElementReference
{
    return (*m_p_a)[m_cursor];
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator->() const -> ElementPointer
{
    return &(*m_p_a)[m_cursor];
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator++() -> BasicIndexTupleIterator&
{
    m_is_at_end = false;

    for (int64_t i = A::dimension() - 1; i >= 0; i--)
    {
        if (m_cursor[i] < m_p_a->extents(i) - 1)
        {
            m_cursor[i]++;
            return *this;
        }
        else
        {
            m_cursor[i] = 0;
        }
    }

    m_is_at_end = true;

    return *this;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator++(int) -> BasicIndexTupleIterator
{
    BasicIndexTupleIterator<A, IsReadOnly> r = *this;
    ++(*this);
    return r;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator--() -> BasicIndexTupleIterator&
{
    m_is_at_end = false;

    for (int64_t i = A::dimension() - 1; i >= 0; i--)
    {
        if (m_cursor[i] >= 1)
        {
            m_cursor[i]--;
            break;
        }
        else
        {
            m_cursor[i] = m_p_a->extents(i) - 1;
        }
    }

    return *this;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator--(int) -> BasicIndexTupleIterator
{
    BasicIndexTupleIterator<A, IsReadOnly> r = *this;
    --(*this);
    return r;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::cursor() const -> const Extents<A::dimension()>&
{
    return m_cursor;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::cursor(const Extents<A::dimension()> &cursor) -> BasicIndexTupleIterator&
{
    m_cursor = cursor;
    return *this;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::is_at_end() const -> const bool&
{
    return m_is_at_end;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::is_at_end(const bool &flag) -> BasicIndexTupleIterator&
{
    m_is_at_end = flag;
    return *this;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::p_a() const -> APointer
{
    return m_p_a;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::begin_of(APointer p_a) -> BasicIndexTupleIterator<A, IsReadOnly>
{
    return BasicIndexTupleIterator<A, IsReadOnly>(p_a);
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::cbegin_of(APointer p_a) -> BasicIndexTupleIterator<A, true>
{
    return BasicIndexTupleIterator<A, true>(p_a);
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::end_of(APointer p_a) -> BasicIndexTupleIterator<A, IsReadOnly>
{
    BasicIndexTupleIterator<A, IsReadOnly> it(p_a);
    it.is_at_end(true);
    return it;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::cend_of(APointer p_a) -> BasicIndexTupleIterator<A, true>
{
    BasicIndexTupleIterator<A, true> it(p_a);
    it.is_at_end(true);
    return it;
}

template<typename A>
BasicIndexTupleIterator(A&) -> BasicIndexTupleIterator<A, false>;

template<typename A>
BasicIndexTupleIterator(const A&) -> BasicIndexTupleIterator<A, true>;


// ContiguousElementIterator class

template<typename A, bool IsReadOnly>
class BasicContiguousElementIterator
{

public:

    using Element = A::Element;

    using AReference =
        std::conditional_t <
            A::is_owning_type(),
            std::conditional_t<IsReadOnly, const A&, A&>,
            const A&
        >;

    using APointer =
        std::conditional_t <
            A::is_owning_type(),
            std::conditional_t<IsReadOnly, const A*, A*>,
            const A*
        >;

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    explicit BasicContiguousElementIterator() = default;

    explicit BasicContiguousElementIterator (APointer p_a, int64_t offset = 0);

    auto operator*() const -> ElementReference;

    auto operator->() const -> ElementPointer;

    auto operator++() -> BasicContiguousElementIterator&;
    auto operator++(int) -> BasicContiguousElementIterator;

    auto operator--() -> BasicContiguousElementIterator&;
    auto operator--(int) -> BasicContiguousElementIterator;

    auto p_a() const -> APointer;
    auto p_e() const -> ElementPointer;

    static auto begin_of(APointer p_a) -> BasicContiguousElementIterator<A, IsReadOnly>;
    static auto cbegin_of(APointer p_a) -> BasicContiguousElementIterator<A, true>;
    static auto end_of(APointer p_a) -> BasicContiguousElementIterator<A, IsReadOnly>;
    static auto cend_of(APointer p_a) -> BasicContiguousElementIterator<A, true>;

private:

    APointer       m_p_a = nullptr;
    ElementPointer m_p_e = nullptr;
};

template<typename AL, bool IsReadOnlyL, typename AR, bool IsReadOnlyR>
auto operator== (
    const BasicContiguousElementIterator<AL, IsReadOnlyL> &lhs,
    const BasicContiguousElementIterator<AR, IsReadOnlyR> &rhs
) -> bool
{
    return lhs.p_e() == rhs.p_e();
}

template<typename A, bool IsReadOnly>
BasicContiguousElementIterator<A, IsReadOnly>::BasicContiguousElementIterator(APointer p_a, int64_t offset)
: m_p_a(p_a), m_p_e(p_a->p_elements() + offset)
{}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::operator*() const -> ElementReference
{
    return *m_p_e;
}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::operator->() const -> ElementPointer
{
    return m_p_e;
}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::operator++() -> BasicContiguousElementIterator&
{
    m_p_e++;
    return *this;
}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::operator++(int) -> BasicContiguousElementIterator
{
    BasicContiguousElementIterator<A, IsReadOnly> r = *this;
    ++(*this);
    return r;
}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::operator--() -> BasicContiguousElementIterator&
{
    m_p_e--;
    return *this;
}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::operator--(int) -> BasicContiguousElementIterator
{
    BasicContiguousElementIterator<A, IsReadOnly> r = *this;
    --(*this);
    return r;
}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::p_a() const -> APointer
{
    return m_p_a;
}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::p_e() const -> ElementPointer
{
    return m_p_e;
}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::begin_of(APointer p_a) -> BasicContiguousElementIterator<A, IsReadOnly>
{
    return BasicContiguousElementIterator<A, IsReadOnly>(p_a);
}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::cbegin_of(APointer p_a) -> BasicContiguousElementIterator<A, true>
{
    return BasicContiguousElementIterator<A, true>(p_a);
}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::end_of(APointer p_a) -> BasicContiguousElementIterator<A, IsReadOnly>
{
    return BasicContiguousElementIterator<A, IsReadOnly>(p_a, p_a->size_allocated());
}

template<typename A, bool IsReadOnly>
auto BasicContiguousElementIterator<A, IsReadOnly>::cend_of(APointer p_a) -> BasicContiguousElementIterator<A, true>
{
    return BasicContiguousElementIterator<A, true>(p_a, p_a->size_allocated());
}

template<typename A>
BasicContiguousElementIterator(A&) -> BasicContiguousElementIterator<A, false>;

template<typename A>
BasicContiguousElementIterator(const A&) -> BasicContiguousElementIterator<A, true>;

#endif // ITERATORS_HPP