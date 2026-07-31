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

    explicit BasicIndexTupleIterator (
        APointer p_a,
        const Extents<A::dimension()> &cursor = make_extents_filled<A::dimension()>(0)
    )
    : m_p_a(p_a),
    m_cursor(cursor),
    m_is_at_end(p_a->size() == 0)
    {}

    auto operator*() const -> ElementReference
    {
        return (*m_p_a)[m_cursor];
    }

    auto operator->() const -> ElementPointer
    {
        return &(*m_p_a)[m_cursor];
    }

    auto operator++() -> BasicIndexTupleIterator&
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

    auto operator++(int) -> BasicIndexTupleIterator
    {
        BasicIndexTupleIterator<A, IsReadOnly> r = *this;
        ++(*this);
        return r;
    }

    auto operator--() -> BasicIndexTupleIterator&
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

    auto operator--(int) -> BasicIndexTupleIterator
    {
        BasicIndexTupleIterator<A, IsReadOnly> r = *this;
        --(*this);
        return r;
    }

    auto cursor() const -> const Extents<A::dimension()>&
    {
        return m_cursor;
    }

    auto cursor(const Extents<A::dimension()> &cursor) -> BasicIndexTupleIterator&
    {
        m_cursor = cursor;
        return *this;
    }

    auto is_at_end() const -> const bool&
    {
        return m_is_at_end;
    }

    auto is_at_end(const bool &flag) -> BasicIndexTupleIterator&
    {
        m_is_at_end = flag;
        return *this;
    }

    auto p_a() const -> APointer
    {
        return m_p_a;
    }

    static auto begin_of(APointer p_a) -> BasicIndexTupleIterator<A, IsReadOnly>
    {
        return BasicIndexTupleIterator<A, IsReadOnly>(p_a);
    }

    static auto cbegin_of(APointer p_a) -> BasicIndexTupleIterator<A, true>
    {
        return BasicIndexTupleIterator<A, true>(p_a);
    }

    static auto end_of(APointer p_a) -> BasicIndexTupleIterator<A, IsReadOnly>
    {
        BasicIndexTupleIterator<A, IsReadOnly> it(p_a);
        it.is_at_end(true);
        return it;
    }

    static auto cend_of(APointer p_a) -> BasicIndexTupleIterator<A, true>
    {
        BasicIndexTupleIterator<A, true> it(p_a);
        it.is_at_end(true);
        return it;
    }

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

template<typename A>
BasicIndexTupleIterator(A&) -> BasicIndexTupleIterator<A, false>;

template<typename A>
BasicIndexTupleIterator(const A&) -> BasicIndexTupleIterator<A, true>;

#endif // ITERATORS_HPP