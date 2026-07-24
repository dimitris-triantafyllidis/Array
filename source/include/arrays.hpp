#ifndef ARRAYS_HPP
#define ARRAYS_HPP

#include "forward_declarations.hpp"

#include "helpers.hpp"

//******************************************************************************
// Array class
//******************************************************************************

/**
 * @brief Primary class template declaration for `Array`.
 *
 * @tparam T  Type of the array's elements.
 * @tparam D  Number of dimensions.
 * @tparam E  The extents of the array, of type `Extents<D>`. Should be either filled with values smaller than `dynamic_extent` for static arrays,
 *            or with the value `dynamic_extent` for dynamic arrays. Filled with the `dynamic_extent` sentinel value by default.
 * @tparam L  A type implementing the actual read and write operations, determining the memory layout.
 *            Defaults to `Affine<D>`.
 */

// Helper functions to copy from initializer lists, for D = 1, D = 2, D = 3

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr void array_copy_from_initializer_list(Array<T, 1, E, L> &a, std::initializer_list<T> l)
{
    if (!(l.size() == a.extents(0)))
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }
    int64_t i = 0;
    for (const auto &e : l)
    {
        a[i] = e;
        i++;
    }
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr void array_copy_from_initializer_list(Array<T, 2, E, L> &a, std::initializer_list<std::initializer_list<T>> ll)
{
    if (!(ll.size() == a.extents(0)))
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }
    int64_t i = 0;
    for (const auto &l : ll)
    {
        if (!(l.size() == a.extents(1)))
        {
            throw_with_context<std::domain_error>("Domain error. Check source location.");
        }
        int64_t j = 0;
        for (const auto &e : l)
        {
            a[i, j] = e;
            j++;
        }
        i++;
    }
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr void array_copy_from_initializer_list(Array<T, 3, E, L> &a, std::initializer_list<std::initializer_list<std::initializer_list<T>>> lll)
{
    if (!(lll.size() == a.extents(0)))
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }
    int64_t i = 0;
    for (const auto &ll : lll)
    {
        if (!(ll.size() == a.extents(1)))
        {
            throw_with_context<std::domain_error>("Domain error. Check source location.");
        }
        int64_t j = 0;
        for (const auto &l : ll)
        {
            if (!(l.size() == a.extents(2)))
            {
                throw_with_context<std::domain_error>("Domain error. Check source location.");
            }
            int64_t k = 0;
            for (const auto &e : l)
            {
                a[i, j, k] = e;
                k++;
            }
            j++;
        }
        i++;
    }
}

template <typename T, int64_t D, Extents<D> E, typename L, bool S>
struct ArrayMembers;

template <typename T, int64_t D, Extents<D> E, typename L>
struct ArrayMembers<T, D, E, L, false>
{
    using Layout = L;

    Layout layout;
    ValuePtr<T[]> elements;
};

template <typename T, int64_t D, Extents<D> E, typename L>
struct ArrayMembers<T, D, E, L, true>
{
    using Layout = L;

    static constexpr Layout layout = Layout(E);
    std::array<T, layout.size_allocated()> elements;
};

template<typename T, int64_t D, Extents<D> E, typename L>
class Array
{

    static_assert(D > 0);
    static_assert(extents_valid(E));

public:

    using Element = T;
    using Layout = L;

    using Owning = Array;

    static consteval auto dimension() -> int64_t;

    static consteval auto is_owning_type() -> bool;

    static consteval auto is_of_static_extents() -> bool;

    static consteval auto type_extents() -> Extents<D>;

    Array();

    explicit Array(const Extents<D> &extents) requires (all_of_extents_dynamic(E));

    template<std::integral... ExtentsPack>
    requires (all_of_extents_dynamic(E))
    Array(ExtentsPack... extents_pack);

    constexpr Array (
        std::initializer_list<T> l
    ) requires (D == 1);

    constexpr Array (
        std::initializer_list <
            std::initializer_list<T>
        > ll
    ) requires (D == 2);

    constexpr Array (
        std::initializer_list <
            std::initializer_list <
                std::initializer_list<T>
            >
        > lll
    ) requires (D == 3);

    template<ViewType V>
    Array(const V &v);

    template<typename T1>
    Array(const Array<T1, D, E, L> &other);

    template<typename Exp>
    requires ( ExpressionNodeType<Exp> && !ArrayType<Exp> && !ViewType<Exp> )
    Array(const Exp &e);

    template<typename T1>
    auto operator=(const Array<T1, D, E, L> &other) -> Array&;

    template<typename Exp>
    requires ( ExpressionNodeType<Exp> && !ArrayType<Exp> && !ViewType<Exp> )
    auto operator=(const Exp &e) -> Array&;

    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) constexpr auto operator[](I... i) const -> const T&;
    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) constexpr auto operator[](I... i) -> T&;

    constexpr auto operator[](const Extents<D> &indices) const -> const T&;
    constexpr auto operator[](const Extents<D> &indices) -> T&;

    constexpr auto extents() const -> const Extents<D>&;
    constexpr auto extents(const int64_t &i) const -> const int64_t&;

    constexpr auto layout() const -> const Layout&;

    constexpr auto size() const -> int64_t;
    constexpr auto size_stored() -> int64_t;
    constexpr auto size_allocated() const -> int64_t;

    constexpr auto p_elements() const -> const T*;
    constexpr auto p_elements() -> T*;

    constexpr auto begin() -> IndexTupleIterator<Array>;
    constexpr auto begin() const -> ReadOnlyIndexTupleIterator<Array>;
    constexpr auto cbegin() const -> ReadOnlyIndexTupleIterator<Array>;

    constexpr auto end() -> IndexTupleIterator<Array>;
    constexpr auto end() const -> ReadOnlyIndexTupleIterator<Array>;
    constexpr auto cend() const -> ReadOnlyIndexTupleIterator<Array>;

    template<typename... NewExtents> requires ((sizeof...(NewExtents) == D) && (std::is_integral_v<NewExtents> && ...))
    auto resize(NewExtents... extents) -> void requires (all_of_extents_dynamic(E));

    auto resize(const Extents<D> &extents) -> void requires (all_of_extents_dynamic(E));

private:

    ArrayMembers<T, D, E, L, all_of_extents_static(E)> m;

};

template<typename T, int64_t D, Extents<D> E, typename L>
consteval auto Array<T, D, E, L>::dimension() -> int64_t
{
    return D;
}

template<typename T, int64_t D, Extents<D> E, typename L>
consteval auto Array<T, D, E, L>::is_owning_type() -> bool
{
    return true;
}

template<typename T, int64_t D, Extents<D> E, typename L>
consteval auto Array<T, D, E, L>::is_of_static_extents() -> bool
{
    return all_of_extents_static(E);
}

template<typename T, int64_t D, Extents<D> E, typename L>
consteval auto Array<T, D, E, L>::type_extents() -> Extents<D>
{
    return E;
}

template<typename T, int64_t D, Extents<D> E, typename L>
Array<T, D, E, L>::Array()
{
    if constexpr (all_of_extents_dynamic(E))
    {
        m.layout   = Layout(make_extents_filled<D>(0));
        m.elements = ValuePtr<T[]>(nullptr, 0);
    }
}

template<typename T, int64_t D, Extents<D> E, typename L>
Array<T, D, E, L>::Array(const Extents<D> &extents) requires (all_of_extents_dynamic(E))
: m {
    .layout = Layout(extents)
}
{
    if (!(extents_valid<D>(extents)))
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }

    m.elements = ValuePtr<T[]>::make(m.layout.size_allocated());
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<std::integral... ExtentsPack>
requires (all_of_extents_dynamic(E))
Array<T, D, E, L>::Array(ExtentsPack... extents_pack)
: m {
    .layout = Layout({int64_t(extents_pack)...})
}
{
    static_assert(sizeof...(extents_pack) == D);

    if (!(extents_valid<D>({int64_t(extents_pack)...})))
    {
        m.layout = Layout(make_extents_filled<D>(0));
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }

    m.elements = ValuePtr<T[]>::make(m.layout.size_allocated());
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr Array<T, D, E, L>::Array (
    std::initializer_list<T> l
) requires (D == 1)
{
    if constexpr (all_of_extents_dynamic(E))
    {
        Extents<D> extents = {
            int64_t(l.size())
        };
        m.layout   = Layout(extents);
        m.elements = ValuePtr<T[]>::make(m.layout.size_allocated());
    }
    array_copy_from_initializer_list(*this, l);
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr Array<T, D, E, L>::Array (
    std::initializer_list <
        std::initializer_list<T>
    > ll
) requires (D == 2)
{
    if constexpr (all_of_extents_dynamic(E))
    {
        Extents<D> extents = {
            int64_t(ll.size()),
            int64_t(ll.begin()->size())
        };
        m.layout   = Layout(extents);
        m.elements = ValuePtr<T[]>::make(m.layout.size_allocated());
    }
    array_copy_from_initializer_list(*this, ll);
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr Array<T, D, E, L>::Array (
    std::initializer_list <
        std::initializer_list <
            std::initializer_list<T>
        >
    > lll
) requires (D == 3)
{
    if constexpr (all_of_extents_dynamic(E))
    {
        Extents<D> extents = {
            int64_t(lll.size()),
            int64_t(lll.begin()->size()),
            int64_t(lll.begin()->begin()->size())
        };
        m.layout   = Layout(extents);
        m.elements = ValuePtr<T[]>::make(m.layout.size_allocated());
    }
    array_copy_from_initializer_list(*this, lll);
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<ViewType V>
Array<T, D, E, L>::Array(const V &v) : Array(v.extents())
{
    ReadOnlyIndexTupleIterator view_it(v);
    IndexTupleIterator it(*this);

    for (int64_t i = 0; i < size(); i++)
    {
        *it++ = *view_it++;
    }
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename T1>
Array<T, D, E, L>::Array(const Array<T1, D, E, L> &other)
{
    if constexpr (all_of_extents_dynamic(E))
    {
        m.layout   = other.layout();
        m.elements = ValuePtr<T[]>::make(other.size_allocated());
    }
    std::copy_n(other.p_elements(), other.size(), p_elements());
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename Exp>
requires ( ExpressionNodeType<Exp> && !ArrayType<Exp> && !ViewType<Exp> )
Array<T, D, E, L>::Array(const Exp &e)
{
    if constexpr (!is_of_static_extents())
    {
        m.layout = Layout(e.extents());
        m.elements = ValuePtr<T[]>::make(m.layout.size_allocated());
    }
    else if (extents() != e.extents())
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }

    auto it = begin();

    for (int64_t i = 0; i < size(); i++)
    {
        *it++ = e[it.cursor()];
    }
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename T1>
auto Array<T, D, E, L>::operator=(const Array<T1, D, E, L> &other) -> Array&
{
    if constexpr (std::is_same_v<T, T1>)
    {
        if(this == &other)
        {
            return *this;
        }
    }

    if constexpr (all_of_extents_dynamic(E))
    {
        m.layout = other.layout();
        m.elements = ValuePtr<T[]>::make(other.size_allocated());
    }
    std::copy_n(other.p_elements(), other.size(), p_elements());

    return *this;
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename Exp>
requires ( ExpressionNodeType<Exp> && !ArrayType<Exp> && !ViewType<Exp> )
auto Array<T, D, E, L>::operator=(const Exp &e) -> Array&
{
    if constexpr (!is_of_static_extents())
    {
        if (extents() != e.extents())
        {
            Array resized = Array(e.extents());
            std::swap(*this, resized);
        }
    }
    else if (extents() != e.extents())
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }

    auto it = begin();

    for (int64_t i = 0; i < size(); i++)
    {
        *it++ = e[it.cursor()];
    }

    return *this;
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...))
constexpr auto Array<T, D, E, L>::operator[](I... i) const -> const T&
{
    return m.elements[m.layout.offset({int64_t(i)...})];
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...))
constexpr auto Array<T, D, E, L>::operator[](I... i) -> T&
{
    return m.elements[m.layout.offset({int64_t(i)...})];
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::operator[](const Extents<D> &indices) const -> const T&
{
    return m.elements[m.layout.offset(indices)];
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::operator[](const Extents<D> &indices) -> T&
{
    return m.elements[m.layout.offset(indices)];
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::extents() const -> const Extents<D>&
{
    return m.layout.extents();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::extents(const int64_t &i) const -> const int64_t&
{
    return m.layout.extents()[i];
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::layout() const -> const Layout&
{
    return m.layout;
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::size() const -> int64_t
{
    return m.layout.size();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::size_allocated() const -> int64_t
{
    return m.layout.size_allocated();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::p_elements() const -> const T*
{
    if constexpr (all_of_extents_dynamic(E))
        return m.elements.get();
    else
        return m.elements.data();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::p_elements() -> T*
{
    if constexpr (all_of_extents_dynamic(E))
        return m.elements.get();
    else
        return m.elements.data();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::begin() -> IndexTupleIterator<Array>
{
    return IndexTupleIterator<Array>::begin_of(this);
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::begin() const -> ReadOnlyIndexTupleIterator<Array>
{
    return ReadOnlyIndexTupleIterator<Array>::begin_of(this);
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::cbegin() const -> ReadOnlyIndexTupleIterator<Array>
{
    return begin();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::end() -> IndexTupleIterator<Array>
{
    return IndexTupleIterator<Array>::end_of(this);
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::end() const -> ReadOnlyIndexTupleIterator<Array>
{
    return ReadOnlyIndexTupleIterator<Array>::end_of(this);
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::cend() const -> ReadOnlyIndexTupleIterator<Array>
{
    return end();
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename... NewExtents> requires ((sizeof...(NewExtents) == D) && (std::is_integral_v<NewExtents> && ...))
auto Array<T, D, E, L>::resize(NewExtents... extents) -> void requires (all_of_extents_dynamic(E))
{
    resize({int64_t(extents)...});
}

template<typename T, int64_t D, Extents<D> E, typename L>
auto Array<T, D, E, L>::resize(const Extents<D> &extents) -> void requires (all_of_extents_dynamic(E))
{
    auto intersection_view = make_read_only_slice_view<D, make_extents_iota<D>(0)> (
        *this,
        make_extents_filled<D>(0),
        extents_intersection<D>(this->extents(), extents)
    );

    Array resized = Array(extents);

    std::println("!!!{}", intersection_view.size());

    for (auto it = intersection_view.cbegin(); it != intersection_view.cend(); it++)
    {
        resized[it.cursor()] = *it;
    }

    std::swap(*this, resized);
}

#endif // ARRAYS_HPP