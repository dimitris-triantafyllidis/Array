#ifndef VIEWS_HPP
#define VIEWS_HPP

#include <type_traits>
#include <numeric>

#include "helpers.hpp"
#include "iterators.hpp"

//******************************************************************************
// View classes
//******************************************************************************

/**
 * @brief `BasicSliceView` class template.
 *
 * @tparam A                  The array-like type of the object we want the view to refer to, typically an `Array` or another `View`.
 * @tparam IsReadOnly         Whether the view is read-only.
 * @tparam D                  Dimension of the view. Must be equal to or less than the dimension of `A`.
 * @tparam ViewIndexSubspace  An `Extents<D>` specifying the axis indices along which the view will extend.
 *                            Must be a strictly increasing sequence.
 */

template <
    typename A,
    bool IsReadOnly,
    int64_t D,
    Extents<D> ViewIndexSubspace
>
class BasicSliceView
{

    static_assert(D > 0);
    static_assert(D <= A::dimension());
    static_assert(axis_selection_valid<A::dimension(), D>(ViewIndexSubspace));
    static_assert(ViewIndexSubspace[D - 1] < A::dimension());

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

    using Owning =
        std::conditional_t <
            A::is_owning_type(),
            A,
            typename A::Owning
        >;

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    BasicSliceView() = default;

    explicit BasicSliceView (
        AReference array,
        const Extents<A::dimension()> &origin  = make_extents_filled<A::dimension()>(0),
        const Extents<D>              &extents = make_extents_filled<D>(dynamic_extent),
        const Extents<D>              &strides = make_extents_filled<D>(1)
    );

    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementReference;

    auto operator[](const Extents<D> &indices) const -> ElementReference;

    static consteval auto dimension()            -> int64_t;
    static consteval auto is_owning_type()       -> bool;
    static consteval auto is_of_static_extents() -> bool;
    static consteval auto type_extents()         -> Extents<D>;

    auto is_identity() const -> bool;
    auto is_identity_chain() const -> bool;

    auto origin() const -> const Extents<A::dimension()>&;
    auto origin(const Extents<A::dimension()> &origin) -> BasicSliceView&;

    auto origin(const int64_t &i) const -> const int64_t&;
    auto origin(const int64_t &i, const int64_t &v) -> BasicSliceView&;

    auto extents() const -> const Extents<D>&;
    auto extents(const Extents<D> &extents) -> BasicSliceView&;

    auto extents(const int64_t &i) const -> const int64_t&;
    auto extents(const int64_t &i, const int64_t &v) -> BasicSliceView&;

    auto strides() const -> const Extents<D>&;
    auto strides(const Extents<D> &strides) -> BasicSliceView&;

    auto strides(const int64_t &i) const -> const int64_t&;
    auto strides(const int64_t &i, const int64_t &v) -> BasicSliceView&;

    auto size() const -> int64_t;

    auto map(const Extents<D> &indices) const -> Extents<A::dimension()>;

    constexpr auto p_elements() const -> ElementPointer;

    auto begin() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicSliceView>,
            IndexTupleIterator<BasicSliceView>
        >;

    auto cbegin() const -> ReadOnlyIndexTupleIterator<BasicSliceView>;

    auto end() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicSliceView>,
            IndexTupleIterator<BasicSliceView>
        >;

    auto cend() const -> ReadOnlyIndexTupleIterator<BasicSliceView>;

private:

    APointer m_p_array = nullptr;

    Extents<A::dimension()> m_origin  = {};
    Extents<D>              m_extents = {};
    Extents<D>              m_strides = {};

};

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::BasicSliceView (
    AReference array,
    const Extents<A::dimension()> &origin,
    const Extents<D>              &extents,
    const Extents<D>              &strides
)
: m_p_array ( &array  ),
  m_origin  ( origin  ),
  m_extents ( extents ),
  m_strides ( strides )
{
    if (all_of_extents_dynamic(extents))
    {
        for (int64_t i = 0; i < extents.size(); i++)
        {
            m_extents[i] = array.extents(ViewIndexSubspace[i]);
        }
    }
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...))
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::operator[](I... i) const -> ElementReference
{
    return operator[]({int64_t(i)...});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::operator[](const Extents<D> &indices) const -> ElementReference
{
    return (*m_p_array)[map(indices)];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::dimension() -> int64_t
{
    return D;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::is_owning_type() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::is_of_static_extents() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::type_extents() -> Extents<D>
{
    return make_extents_filled<D>(dynamic_extent);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::is_identity() const -> bool
{
    if constexpr (dimension() == A::dimension())
    {
        return (
            ( origin()  == make_extents_filled<dimension()>(0) ) &&
            ( extents() == m_p_array->extents()                ) &&
            ( strides() == make_extents_filled<dimension()>(1) )
        );
    }

    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::is_identity_chain() const -> bool
{
    if constexpr (A::is_owning_type())
    {
        return is_identity();
    }
    else
    {
        return is_identity() && m_p_array->is_identity_chain();
    }
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::origin() const -> const Extents<A::dimension()>&
{
    return m_origin;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::origin(const Extents<A::dimension()> &origin) -> BasicSliceView&
{
    m_origin = origin;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::origin(const int64_t &i) const -> const int64_t&
{
    return m_origin[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::origin(const int64_t &i, const int64_t &v) -> BasicSliceView&
{
    m_origin[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::extents() const -> const Extents<D>&
{
    return m_extents;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::extents(const Extents<D> &extents) -> BasicSliceView&
{
    m_extents = extents;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::extents(const int64_t &i) const -> const int64_t&
{
    return m_extents[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::extents(const int64_t &i, const int64_t &v) -> BasicSliceView&
{
    m_extents[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::strides() const -> const Extents<D>&
{
    return m_strides;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::strides(const Extents<D> &strides) -> BasicSliceView&
{
    m_strides = strides;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::strides(const int64_t &i) const -> const int64_t&
{
    return m_strides[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::strides(const int64_t &i, const int64_t &v) -> BasicSliceView&
{
    m_strides[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::size() const -> int64_t
{
    return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::map(const Extents<D> &view_indices) const -> Extents<A::dimension()>
{
    Extents<A::dimension()> array_indices = m_origin;

    for (int64_t k = 0; k < D; k++)
    {
        array_indices[ViewIndexSubspace[k]] += view_indices[k] * m_strides[k];
    }

    return array_indices;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
constexpr auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::p_elements() const -> ElementPointer
{
    return m_p_array->p_elements();
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::begin() const ->
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicSliceView>,
        IndexTupleIterator<BasicSliceView>
    >
{
    return
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicSliceView>,
            IndexTupleIterator<BasicSliceView>
        >::begin_of(this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::cbegin() const -> ReadOnlyIndexTupleIterator<BasicSliceView>
{
    return ReadOnlyIndexTupleIterator<BasicSliceView>::cbegin_of(this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::end() const ->
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicSliceView>,
        IndexTupleIterator<BasicSliceView>
    >
{
    return
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicSliceView>,
            IndexTupleIterator<BasicSliceView>
        >::end_of(this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::cend() const -> ReadOnlyIndexTupleIterator<BasicSliceView>
{
    return ReadOnlyIndexTupleIterator<BasicSliceView>::cend_of(this);
}

template<int64_t D, Extents<D> ViewIndexSubspace, typename A>
auto make_slice_view (
    A& array,
    const Extents<A::dimension()> &origin  = make_extents_filled<A::dimension()>(0),
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent),
    const Extents<D> &strides = make_extents_filled<D>(1)
)
{
    return SliceView<A, D, ViewIndexSubspace>(array, origin, extents, strides);
}

template<int64_t D, Extents<D> ViewIndexSubspace, typename A>
auto make_read_only_slice_view (
    A& array,
    const Extents<A::dimension()> &origin  = make_extents_filled<A::dimension()>(0),
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent),
    const Extents<D> &strides = make_extents_filled<D>(1)
)
{
    return ReadOnlySliceView<A, D, ViewIndexSubspace>(array, origin, extents, strides);
}

/**
 * @brief `BasicBroadcastView` class template.
 *
 * @tparam A                  The array-like type of the object we want the view to refer to, typically an `Array` or another `View`.
 * @tparam IsReadOnly         Whether the view is read-only.
 * @tparam D                  Dimension of the view. Must be equal to or bigger than the dimension of `A`.
 * @tparam AIndexSubspace     An `Extents<A::dimension()>` specifying the view axis indices along which the viewed object extends.
 *                            Must be a strictly increasing sequence.
 */

template <
    typename A,
    bool IsReadOnly,
    int64_t D,
    Extents<A::dimension()> AIndexSubspace
>
class BasicBroadcastView
{

    static_assert(D > 0);
    static_assert(D >= A::dimension());
    static_assert(axis_selection_valid<D, A::dimension()>(AIndexSubspace));
    static_assert(AIndexSubspace[A::dimension() - 1] < D);

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

    using Owning =
        std::conditional_t <
            A::is_owning_type(),
            A,
            typename A::Owning
        >;

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    BasicBroadcastView() = default;

    explicit BasicBroadcastView (
        AReference array,
        const Extents<D> &extents
    );

    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementReference;

    auto operator[](const Extents<D> &indices) const -> ElementReference;

    static consteval auto dimension()            -> int64_t;
    static consteval auto is_owning_type()       -> bool;
    static consteval auto is_of_static_extents() -> bool;
    static consteval auto type_extents()         -> Extents<D>;

    auto is_identity() const -> bool;
    auto is_identity_chain() const -> bool;

    auto extents() const -> const Extents<D>&;
    auto extents(const Extents<D> &extents) -> BasicBroadcastView&;

    auto extents(const int64_t &i) const -> const int64_t&;
    auto extents(const int64_t &i, const int64_t &v) -> BasicBroadcastView&;

    auto size() const -> int64_t;

    auto map(const Extents<D> &indices) const -> Extents<A::dimension()>;

    constexpr auto p_elements() const -> ElementPointer;

    auto begin() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicBroadcastView>,
            IndexTupleIterator<BasicBroadcastView>
        >;

    auto cbegin() const -> ReadOnlyIndexTupleIterator<BasicBroadcastView>;

    auto end() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicBroadcastView>,
            IndexTupleIterator<BasicBroadcastView>
        >;

    auto cend() const -> ReadOnlyIndexTupleIterator<BasicBroadcastView>;

private:

    APointer m_p_array = nullptr;

    Extents<D> m_extents = {};
};

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::BasicBroadcastView (AReference array, const Extents<D> &extents)
: m_p_array ( &array ), m_extents(extents)
{}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...))
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::operator[](I... i) const -> ElementReference
{
    return operator[]({int64_t(i)...});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::operator[](const Extents<D> &indices) const -> ElementReference
{
    return (*m_p_array)[map(indices)];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::dimension() -> int64_t
{
    return D;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::is_owning_type() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::is_of_static_extents() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::type_extents() -> Extents<D>
{
    return make_extents_filled<D>(dynamic_extent);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::is_identity() const -> bool
{
    if constexpr (dimension() == A::dimension())
    {
        return extents() == m_p_array->extents();
    }

    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::is_identity_chain() const -> bool
{
    if constexpr (A::is_owning_type())
    {
        return is_identity();
    }
    else
    {
        return is_identity() && m_p_array->is_identity_chain();
    }
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::extents() const -> const Extents<D>&
{
    return m_extents;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::extents(const Extents<D> &extents) -> BasicBroadcastView&
{
    m_extents = extents;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::extents(const int64_t &i) const -> const int64_t&
{
    return m_extents[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::extents(const int64_t &i, const int64_t &v) -> BasicBroadcastView&
{
    m_extents[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::size() const -> int64_t
{
    return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::map(const Extents<D> &view_indices) const -> Extents<A::dimension()>
{
    Extents<A::dimension()> array_indices = {};

    for (int64_t i = 0; i < A::dimension(); i++)
    {
        array_indices[i] = view_indices[AIndexSubspace[i]] % m_p_array->extents(i);
    }

    return array_indices;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
constexpr auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::p_elements() const -> ElementPointer
{
    return m_p_array->p_elements();
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::begin() const ->
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicBroadcastView>,
        IndexTupleIterator<BasicBroadcastView>
    >
{
    return
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicBroadcastView>,
            IndexTupleIterator<BasicBroadcastView>
        >::begin_of(this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::cbegin() const -> ReadOnlyIndexTupleIterator<BasicBroadcastView>
{
    return ReadOnlyIndexTupleIterator<BasicBroadcastView>::cbegin_of(this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::end() const ->
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicBroadcastView>,
        IndexTupleIterator<BasicBroadcastView>
    >
{
    return
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicBroadcastView>,
            IndexTupleIterator<BasicBroadcastView>
        >::end_of(this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::cend() const -> ReadOnlyIndexTupleIterator<BasicBroadcastView>
{
    return ReadOnlyIndexTupleIterator<BasicBroadcastView>::cend_of(this);
}

template<int64_t D, Extents<D> AIndexSubspace, typename A>
auto make_broadcast_view (
    A& array,
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent)
)
{
    return BroadcastView<A, D, AIndexSubspace>(array, extents);
}

template<int64_t D, Extents<D> AIndexSubspace, typename A>
auto make_read_only_broadcast_view (
    A& array,
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent)
)
{
    return ReadOnlyBroadcastView<A, D, AIndexSubspace>(array, extents);
}

/**
 * @brief `BasicIdentityView` class template.
 *
 * @tparam A            The array-like type of the object we want the view to refer to, typically an `Array` or another `View`.
 * @tparam IsReadOnly   Whether the view is read-only.
 */

template <
    typename A,
    bool IsReadOnly
>
class BasicIdentityView
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

    using Owning =
        std::conditional_t <
            A::is_owning_type(),
            A,
            typename A::Owning
        >;

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    BasicIdentityView() = default;

    explicit BasicIdentityView (
        AReference array
    );

    template<typename... I> requires ((sizeof...(I) == A::dimension()) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementReference;

    auto operator[](const Extents<A::dimension()> &indices) const -> ElementReference;

    static consteval auto dimension()            -> int64_t;
    static consteval auto is_owning_type()       -> bool;
    static consteval auto is_of_static_extents() -> bool;
    static consteval auto type_extents()         -> Extents<A::dimension()>;

    consteval auto is_identity() const -> bool;
    auto is_identity_chain() const -> bool;

    auto extents() const -> const Extents<A::dimension()>&;

    auto extents(const int64_t &i) const -> const int64_t&;

    auto size() const -> int64_t;

    auto map(const Extents<A::dimension()> &indices) const -> Extents<A::dimension()>;

    constexpr auto p_elements() const -> ElementPointer;

    auto begin() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicIdentityView>,
            IndexTupleIterator<BasicIdentityView>
        >;

    auto cbegin() const -> ReadOnlyIndexTupleIterator<BasicIdentityView>;

    auto end() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicIdentityView>,
            IndexTupleIterator<BasicIdentityView>
        >;

    auto cend() const -> ReadOnlyIndexTupleIterator<BasicIdentityView>;

private:

    APointer m_p_array = nullptr;
};

template<typename A, bool IsReadOnly>
BasicIdentityView<A, IsReadOnly>::BasicIdentityView (AReference array)
: m_p_array ( &array )
{}

template<typename A, bool IsReadOnly>
template<typename... I> requires ((sizeof...(I) == A::dimension()) && (std::is_integral_v<I> && ...))
auto BasicIdentityView<A, IsReadOnly>::operator[](I... i) const -> ElementReference
{
    return operator[]({int64_t(i)...});
}

template<typename A, bool IsReadOnly>
auto BasicIdentityView<A, IsReadOnly>::operator[](const Extents<A::dimension()> &indices) const -> ElementReference
{
    return (*m_p_array)[map(indices)];
}

template<typename A, bool IsReadOnly>
consteval auto BasicIdentityView<A, IsReadOnly>::dimension() -> int64_t
{
    return A::dimension();
}

template<typename A, bool IsReadOnly>
consteval auto BasicIdentityView<A, IsReadOnly>::is_owning_type() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly>
consteval auto BasicIdentityView<A, IsReadOnly>::is_of_static_extents() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly>
consteval auto BasicIdentityView<A, IsReadOnly>::type_extents() -> Extents<A::dimension()>
{
    return make_extents_filled<A::dimension()>(dynamic_extent);
}

template<typename A, bool IsReadOnly>
consteval auto BasicIdentityView<A, IsReadOnly>::is_identity() const -> bool
{
    return true;
}

template<typename A, bool IsReadOnly>
auto BasicIdentityView<A, IsReadOnly>::is_identity_chain() const -> bool
{
    if constexpr (A::is_owning_type())
    {
        return is_identity();
    }
    else
    {
        return is_identity() && m_p_array->is_identity_chain();
    }
}

template<typename A, bool IsReadOnly>
auto BasicIdentityView<A, IsReadOnly>::extents() const -> const Extents<A::dimension()>&
{
    return m_p_array->extents();
}

template<typename A, bool IsReadOnly>
auto BasicIdentityView<A, IsReadOnly>::extents(const int64_t &i) const -> const int64_t&
{
    return m_p_array->extents()[i];
}

template<typename A, bool IsReadOnly>
auto BasicIdentityView<A, IsReadOnly>::size() const -> int64_t
{
    return m_p_array->size();
}

template<typename A, bool IsReadOnly>
auto BasicIdentityView<A, IsReadOnly>::map(const Extents<A::dimension()> &view_indices) const -> Extents<A::dimension()>
{
    return view_indices;
}

template<typename A, bool IsReadOnly>
constexpr auto BasicIdentityView<A, IsReadOnly>::p_elements() const -> ElementPointer
{
    return m_p_array->p_elements();
}

template<typename A, bool IsReadOnly>
auto BasicIdentityView<A, IsReadOnly>::begin() const ->
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicIdentityView>,
        IndexTupleIterator<BasicIdentityView>
    >
{
    return
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicIdentityView>,
            IndexTupleIterator<BasicIdentityView>
        >::begin_of(this);
}

template<typename A, bool IsReadOnly>
auto BasicIdentityView<A, IsReadOnly>::cbegin() const -> ReadOnlyIndexTupleIterator<BasicIdentityView>
{
    return ReadOnlyIndexTupleIterator<BasicIdentityView>::cbegin_of(this);
}

template<typename A, bool IsReadOnly>
auto BasicIdentityView<A, IsReadOnly>::end() const ->
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicIdentityView>,
        IndexTupleIterator<BasicIdentityView>
    >
{
    return
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicIdentityView>,
            IndexTupleIterator<BasicIdentityView>
        >::end_of(this);
}

template<typename A, bool IsReadOnly>
auto BasicIdentityView<A, IsReadOnly>::cend() const -> ReadOnlyIndexTupleIterator<BasicIdentityView>
{
    return ReadOnlyIndexTupleIterator<BasicIdentityView>::cend_of(this);
}

template<typename A>
auto make_identity_view (
    A& array,
    const Extents<A::dimension()> &extents = make_extents_filled<A::dimension()>(dynamic_extent)
)
{
    return IdentityView<A>(array);
}

template<typename A>
auto make_read_only_identity_view (
    A& array,
    const Extents<A::dimension()> &extents = make_extents_filled<A::dimension()>(dynamic_extent)
)
{
    return ReadOnlyIdentityView<A>(array);
}


#endif // VIEWS_HPP