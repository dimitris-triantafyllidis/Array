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

    using Element = Bare<A>::Element;

    static consteval auto is_read_only() -> bool
    {
        if constexpr (IsReadOnly) {
            return true;
        }
        else if constexpr ( ArrayType<A> ) {
            return std::is_const_v<A>;
        }
        else {
            return Bare<A>::is_read_only();
        }
    }

    static_assert(IsReadOnly || !is_read_only());

    static consteval auto propagates_stored_elements() -> bool
    {
        return Bare<A>::propagates_stored_elements();
    }

    using ElementAccess =
        std::conditional_t <
            !propagates_stored_elements(),
            typename Bare<A>::Element,
            std::conditional_t <
                is_read_only(),
                const Element&,
                Element&
            >
        >;

    using AReference =
        std::conditional_t <
            ArrayType<A>,
            std::conditional_t<IsReadOnly, const A&, A&>,
            const A&
        >;

    using ElementPointer = std::conditional_t<IsReadOnly, const Element*, Element*>;

    explicit BasicIndexTupleIterator (
        AReference a,
        const Extents<Bare<A>::dimension()> &cursor = make_extents_filled<Bare<A>::dimension()>(0)
    )
    : m_a(a),
      m_cursor(cursor)
    {}

    auto operator*() const -> ElementAccess
    {
        return m_a[m_cursor];
    }

    template<typename... I> requires ((sizeof...(I) == Bare<A>::dimension()) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementAccess
    {
        return operator[]({int64_t(i)...});
    }

    auto operator[](const Extents<Bare<A>::dimension()> &indices) const -> ElementAccess
    {
        return m_a[m_cursor + indices];
    }

    auto operator->() const -> ElementPointer
    requires (propagates_stored_elements())
    {
        return &(m_a[m_cursor]);
    }

    auto operator++() -> BasicIndexTupleIterator&
    {
        for (int64_t i = Bare<A>::dimension() - 1; i >= 1; i--)
        {
            if (++m_cursor[i] != m_a.extents(i))
            {
                return *this;
            }
            m_cursor[i] = 0;
        }

        m_cursor[0]++;

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
        for (int64_t i = Bare<A>::dimension() - 1; i >= 1; i--)
        {
            if (--m_cursor[i] >= 0)
            {
                return *this;
            }
            m_cursor[i] = m_a.extents(i) - 1;
        }

        m_cursor[0]--;

        return *this;
    }

    auto operator--(int) -> BasicIndexTupleIterator
    {
        BasicIndexTupleIterator<A, IsReadOnly> r(*this);
        --(*this);
        return r;
    }

    auto cursor() const -> const Extents<Bare<A>::dimension()>&
    {
        return m_cursor;
    }

    auto cursor(const Extents<Bare<A>::dimension()> &cursor) -> BasicIndexTupleIterator&
    {
        m_cursor = cursor;
        return *this;
    }

    auto is_at_end() const -> bool
    {
        return m_cursor[0] == m_a.extents(0);
    }

    auto is_at_end(const bool &flag) -> BasicIndexTupleIterator&
    {
        if ( flag )
        {
            m_cursor = make_extents_filled<Bare<A>::dimension()>(0);
        }
        m_cursor[0] = m_a.extents(0);
        return *this;
    }

    auto p_a() const -> const A*
    {
        return &m_a;
    }

    static auto begin_of(AReference a) -> auto
    {
        return BasicIndexTupleIterator<A, IsReadOnly>(a);
    }

    static auto cbegin_of(AReference a) -> auto
    {
        return BasicIndexTupleIterator<A, true>(a);
    }

    static auto end_of(AReference a) -> auto
    {
        BasicIndexTupleIterator<A, IsReadOnly> it(a);
        it.is_at_end(true);
        return it;
    }

    static auto cend_of(AReference a) -> auto
    {
        BasicIndexTupleIterator<A, true> it(a);
        it.is_at_end(true);
        return it;
    }

private:

    AReference                    m_a;
    Extents<Bare<A>::dimension()> m_cursor    = make_extents_filled<Bare<A>::dimension()>(0);
};

template<typename IL, bool IsReadOnlyL, typename IR, bool IsReadOnlyR>
auto operator== (
    const BasicIndexTupleIterator<IL, IsReadOnlyL> &lhs,
    const BasicIndexTupleIterator<IR, IsReadOnlyR> &rhs
) -> bool
{
    return
        lhs.p_a()    == rhs.p_a()     &&
        lhs.cursor() == rhs.cursor();
}

template<typename IL, bool IsReadOnlyL>
auto operator+= (
    BasicIndexTupleIterator<IL, IsReadOnlyL> &lhs,
    const Extents<Bare<IL>::dimension()> &rhs
) -> BasicIndexTupleIterator<IL, IsReadOnlyL>&
{
    lhs.cursor(lhs.cursor() + rhs);
    return lhs;
}

template<typename IL, bool IsReadOnlyL>
auto operator+ (
    const BasicIndexTupleIterator<IL, IsReadOnlyL> &lhs,
    const Extents<Bare<IL>::dimension()> &rhs
) -> BasicIndexTupleIterator<IL, IsReadOnlyL>
{
    auto r = lhs;
    r += rhs;
    return r;
}

template<typename IR, bool IsReadOnlyR>
auto operator+ (
    const Extents<Bare<IR>::dimension()> &lhs,
    const BasicIndexTupleIterator<IR, IsReadOnlyR> &rhs
) -> BasicIndexTupleIterator<IR, IsReadOnlyR>
{
    auto r = rhs;
    r += lhs;
    return r;
}

template<typename IL, bool IsReadOnlyL>
auto operator-= (
    BasicIndexTupleIterator<IL, IsReadOnlyL> &lhs,
    const Extents<Bare<IL>::dimension()> &rhs
) -> BasicIndexTupleIterator<IL, IsReadOnlyL>&
{
    lhs.cursor(lhs.cursor() - rhs);
    return lhs;
}

template<typename IL, bool IsReadOnlyL>
auto operator- (
    const BasicIndexTupleIterator<IL, IsReadOnlyL> &lhs,
    const Extents<Bare<IL>::dimension()> &rhs
) -> BasicIndexTupleIterator<IL, IsReadOnlyL>
{
    auto r = lhs;
    r -= rhs;
    return r;
}


#endif // ITERATORS_HPP