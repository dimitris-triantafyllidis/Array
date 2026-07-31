#ifndef EXPRESSION_NODES_HPP
#define EXPRESSION_NODES_HPP

#include <type_traits>
#include <numeric>

#include "forward_declarations.hpp"
#include "helpers.hpp"
#include "iterators.hpp"

//******************************************************************************
// Expression templates
//******************************************************************************

template<class T, bool IsView = ViewType<T>>
struct LayoutOf;

template<class T>
struct LayoutOf<T, true>
{
    using Type = Affine<T::dimension()>;
};

template<class T>
struct LayoutOf<T, false>
{
    using Type = typename T::Layout;
};

template <typename T>
using ExpressionNodeMemberStorage =
    std::conditional_t <
        std::is_lvalue_reference_v<T>,
        T,
        std::remove_cvref_t<T>
    >;

template <typename Op, typename E>
class UnaryOpNode {

public:

    using Element = decltype (
        Op {} (
            std::declval<typename Bare<E>::Element>()
        )
    );

    static consteval auto dimension() -> int64_t {
        return Bare<E>::dimension();
    }

    static consteval auto is_owning_type() -> bool {
        return false;
    }

    static consteval auto is_of_static_extents() -> bool {
        return Bare<E>::is_of_static_extents();
    }

    static consteval auto type_extents() -> Extents<dimension()> {
        if constexpr (is_of_static_extents())
        {
            return Bare<E>::type_extents();
        }
        else
        {
            return make_extents_filled<dimension()>(dynamic_extent);
        }
    }

    UnaryOpNode(Op &&op, E &&e)
    : m_op ( std::forward<Op> ( op ) ),
      m_e  ( std::forward<E>  ( e  ) )
    {}

    auto operator[](const Extents<dimension()> &indices) const -> Element {
        return m_op(m_e[indices]);
    }

    constexpr auto extents() const -> const Extents<dimension()>& {
        return m_e.extents();
    }

    constexpr auto extents(const int64_t &i) const -> const int64_t& {
        return m_e.extents()[i];
    }

private:

    ExpressionNodeMemberStorage<Op> m_op;
    ExpressionNodeMemberStorage<E>  m_e;

};

template <typename Op, typename E>
UnaryOpNode(Op&&, E&&) -> UnaryOpNode<Op, E>;

template <typename Op, typename L, typename R>
requires (
    ( Bare<L>::dimension() == Bare<R>::dimension() ) &&
    (
        !Bare<L>::is_of_static_extents() ||
        !Bare<R>::is_of_static_extents() ||
        ( Bare<L>::type_extents() == Bare<R>::type_extents() )
    )
)
class BinaryZipMapOpNode {

public:

    using Element = decltype (
        Op {} (
            std::declval<typename Bare<L>::Element>(),
            std::declval<typename Bare<R>::Element>()
        )
    );

    static consteval auto dimension() -> int64_t {
        return Bare<L>::dimension();
    }

    static consteval auto is_owning_type() -> bool {
        return false;
    }

    static consteval auto is_of_static_extents() -> bool {
        return
            Bare<L>::is_of_static_extents() &&
            Bare<R>::is_of_static_extents();
    }

    static consteval auto type_extents() -> Extents<dimension()> {
        if constexpr (is_of_static_extents())
        {
            return Bare<L>::type_extents();
        }
        else
        {
            return make_extents_filled<dimension()>(dynamic_extent);
        }
    }

    BinaryZipMapOpNode(Op &&op, L &&l, R &&r)
    : m_op ( std::forward<Op> ( op ) ),
      m_l  ( std::forward<L>  ( l  ) ),
      m_r  ( std::forward<R>  ( r  ) )
    {}

    auto operator[](const Extents<dimension()> &indices) const -> Element {
        return m_op(m_l[indices], m_r[indices]);
    }

    constexpr auto extents() const -> const Extents<dimension()>& {
        return m_l.extents();
    }

    constexpr auto extents(const int64_t &i) const -> const int64_t& {
        return m_l.extents()[i];
    }

private:

    ExpressionNodeMemberStorage<Op> m_op;
    ExpressionNodeMemberStorage<L>  m_l;
    ExpressionNodeMemberStorage<R>  m_r;

};

template <typename Op, typename L, typename R>
BinaryZipMapOpNode(Op&&, L&&, R&&) -> BinaryZipMapOpNode<Op, L, R>;

template <typename Op, typename L, typename R>
class BinaryRScalarOpNode {

public:

    using Element = decltype (
        Op {} (
            std::declval<typename Bare<L>::Element>(),
            std::declval<Bare<R>>()
        )
    );

    static consteval auto dimension() -> int64_t {
        return Bare<L>::dimension();
    }

    static consteval auto is_owning_type() -> bool {
        return false;
    }

    static consteval auto is_of_static_extents() -> bool {
        return Bare<L>::is_of_static_extents();
    }

    static consteval auto type_extents() -> Extents<dimension()> {
        if constexpr (is_of_static_extents())
        {
            return Bare<L>::type_extents();
        }
        else
        {
            return make_extents_filled<dimension()>(dynamic_extent);
        }
    }

    BinaryRScalarOpNode(Op &&op, L &&l, R &&r)
    : m_op ( std::forward<Op> ( op ) ),
      m_l  ( std::forward<L>  ( l  ) ),
      m_r  ( std::forward<R>  ( r  ) )
    {}

    auto operator[](const Extents<Bare<L>::dimension()> &indices) const -> Element {
        return m_op(m_l[indices], m_r);
    }

    constexpr auto extents() const -> const Extents<dimension()>& {
        return m_l.extents();
    }

    constexpr auto extents(const int64_t &i) const -> const int64_t& {
        return m_l.extents()[i];
    }

private:

    ExpressionNodeMemberStorage<Op> m_op;
    ExpressionNodeMemberStorage<L>  m_l;
    ExpressionNodeMemberStorage<R>  m_r;

};

template <typename Op, typename L, typename R>
BinaryRScalarOpNode(Op&&, L&&, R&&) -> BinaryRScalarOpNode<Op, L, R>;

template <typename Op, typename L, typename R>
class BinaryLScalarOpNode {

public:

    using Element = decltype (
        Op {} (
            std::declval<Bare<L>>(),
            std::declval<typename Bare<R>::Element>()
        )
    );

    static consteval auto dimension() -> int64_t {
        return Bare<R>::dimension();
    }

    static consteval auto is_owning_type() -> bool {
        return false;
    }

    static consteval auto is_of_static_extents() -> bool {
        return Bare<R>::is_of_static_extents();
    }

    static consteval auto type_extents() -> Extents<dimension()> {
        if constexpr (is_of_static_extents())
        {
            return Bare<R>::type_extents();
        }
        else
        {
            return make_extents_filled<dimension()>(dynamic_extent);
        }
    }

    BinaryLScalarOpNode(Op &&op, L &&l, R &&r)
    : m_op ( std::forward<Op> ( op ) ),
      m_l  ( std::forward<L>  ( l  ) ),
      m_r  ( std::forward<R>  ( r  ) )
    {}

    auto operator[](const Extents<Bare<R>::dimension()> &indices) const -> Element {
        return m_op(m_l, m_r[indices]);
    }

    constexpr auto extents() const -> const Extents<dimension()>& {
        return m_r.extents();
    }

    constexpr auto extents(const int64_t &i) const -> const int64_t& {
        return m_r.extents()[i];
    }

private:

    ExpressionNodeMemberStorage<Op> m_op;
    ExpressionNodeMemberStorage<L>  m_l;
    ExpressionNodeMemberStorage<R>  m_r;

};

template <typename Op, typename L, typename R>
BinaryLScalarOpNode(Op&&, L&&, R&&) -> BinaryLScalarOpNode<Op, L, R>;


//******************************************************************************
// ViewNode classes
//******************************************************************************


/**
 * @brief `BasicIndentityViewNode` class template.
 *
 * @tparam A            The array-like type of the object we want the view to refer to, typically an `Array` or another `View`.
 * @tparam IsReadOnly   Whether the view is read-only.
 */

template <
    typename A,
    bool IsReadOnly
>
class BasicIndentityViewNode
{

public:

    using Element = Bare<A>::Element;

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    using ElementAccess =
        std::conditional_t <
            ArrayType<A> || ViewType<A>,
            ElementReference,
            typename Bare<A>::Element
        >;

    explicit BasicIndentityViewNode ( A&& a )
    : m_a ( std::forward<A>(a) )
    {}

    template<typename... I> requires ((sizeof...(I) == Bare<A>::dimension()) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementAccess
    {
        return operator[]({int64_t(i)...});
    }

    auto operator[](const Extents<Bare<A>::dimension()> &indices) const -> ElementAccess
    {
        return m_a[map(indices)];
    }

    static consteval auto dimension() -> int64_t
    {
        return Bare<A>::dimension();
    }

    static consteval auto is_owning_type() -> bool
    {
        return false;
    }

    static consteval auto is_of_static_extents() -> bool
    {
        return false;
    }

    static consteval auto type_extents() -> Extents<Bare<A>::dimension()>
    {
        return make_extents_filled<Bare<A>::dimension()>(dynamic_extent);
    }

    consteval auto is_identity() const -> bool
    {
        return true;
    }

    auto is_identity_chain() const -> bool
    {
        if constexpr (A::is_owning_type())
        {
            return is_identity();
        }
        else
        {
            return is_identity() && m_a.is_identity_chain();
        }
    }

    auto extents() const -> const Extents<Bare<A>::dimension()>&
    {
        return m_a.extents();
    }

    auto extents(const int64_t &i) const -> const int64_t&
    {
        return m_a.extents()[i];
    }

    auto size() const -> int64_t
    {
        return m_a.size();
    }

    auto map(const Extents<Bare<A>::dimension()> &view_indices) const -> Extents<Bare<A>::dimension()>
    {
        return view_indices;
    }

    constexpr auto p_elements() const -> ElementPointer
    {
        return m_a.p_elements();
    }

    auto begin() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicIndentityViewNode>,
            IndexTupleIterator<BasicIndentityViewNode>
        >
    {
        return
            std::conditional_t <
                IsReadOnly,
                ReadOnlyIndexTupleIterator<BasicIndentityViewNode>,
                IndexTupleIterator<BasicIndentityViewNode>
            >::begin_of(this);
    }

    auto cbegin() const -> ReadOnlyIndexTupleIterator<BasicIndentityViewNode>
    {
        return ReadOnlyIndexTupleIterator<BasicIndentityViewNode>::cbegin_of(this);
    }

    auto end() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicIndentityViewNode>,
            IndexTupleIterator<BasicIndentityViewNode>
        >
    {
        return
            std::conditional_t <
                IsReadOnly,
                ReadOnlyIndexTupleIterator<BasicIndentityViewNode>,
                IndexTupleIterator<BasicIndentityViewNode>
            >::end_of(this);
    }

    auto cend() const -> ReadOnlyIndexTupleIterator<BasicIndentityViewNode>
    {
        return ReadOnlyIndexTupleIterator<BasicIndentityViewNode>::cend_of(this);
    }

private:

    ExpressionNodeMemberStorage<A> m_a;
};

template<typename A>
auto make_identity_view (
    A&& a,
    const Extents<Bare<A>::dimension()> &extents = make_extents_filled<Bare<A>::dimension()>(dynamic_extent)
)
{
    return IndentityViewNode<A>(std::forward<A>(a));
}

template<typename A>
auto make_read_only_identity_view (
    A&& a,
    const Extents<Bare<A>::dimension()> &extents = make_extents_filled<Bare<A>::dimension()>(dynamic_extent)
)
{
    return ReadOnlyIndentityViewNode<A>(std::forward<A>(a));
}

/**
 * @brief `BasicSliceViewNode` class template.
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
class BasicSliceViewNode
{

    static_assert(D > 0);
    static_assert(D <= Bare<A>::dimension());
    static_assert(axis_selection_valid<Bare<A>::dimension(), D>(ViewIndexSubspace));
    static_assert(ViewIndexSubspace[D - 1] < Bare<A>::dimension());

public:

    using Element = Bare<A>::Element;

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    using ElementAccess =
        std::conditional_t <
            ArrayType<A> || ViewType<A>,
            ElementReference,
            typename Bare<A>::Element
        >;

    BasicSliceViewNode (
        A&& a,
        const Extents<Bare<A>::dimension()> &origin,
        const Extents<D>                    &extents,
        const Extents<D>                    &strides
    )
    : m_a       ( std::forward<A>(a) ),
      m_origin  ( origin  ),
      m_extents ( extents ),
      m_strides ( strides )
    {
        if (all_of_extents_dynamic(extents))
        {
            for (int64_t i = 0; i < extents.size(); i++)
            {
                m_extents[i] = a.extents(ViewIndexSubspace[i]);
            }
        }
    }

    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementAccess
    {
        return operator[]({int64_t(i)...});
    }

    auto operator[](const Extents<D> &indices) const -> ElementAccess
    {
        return m_a[map(indices)];
    }

    static consteval auto dimension() -> int64_t
    {
        return D;
    }

    static consteval auto is_owning_type() -> bool
    {
        return false;
    }

    static consteval auto is_of_static_extents() -> bool
    {
        return false;
    }

    static consteval auto type_extents() -> Extents<D>
    {
        return make_extents_filled<D>(dynamic_extent);
    }

    auto is_identity() const -> bool
    {
        if constexpr (dimension() == Bare<A>::dimension())
        {
            return (
                ( origin()  == make_extents_filled<dimension()>(0) ) &&
                ( extents() == m_a.extents()                ) &&
                ( strides() == make_extents_filled<dimension()>(1) )
            );
        }

        return false;
    }

    auto is_identity_chain() const -> bool
    {
        if constexpr (A::is_owning_type())
        {
            return is_identity();
        }
        else
        {
            return is_identity() && m_a.is_identity_chain();
        }
    }

    auto origin() const -> const Extents<Bare<A>::dimension()>&
    {
        return m_origin;
    }

    auto origin(const Extents<Bare<A>::dimension()> &origin) -> BasicSliceViewNode&
    {
        m_origin = origin;
        return *this;
    }

    auto origin(const int64_t &i) const -> const int64_t&
    {
        return m_origin[i];
    }

    auto origin(const int64_t &i, const int64_t &v) -> BasicSliceViewNode&
    {
        m_origin[i] = v;
        return *this;
    }

    auto extents() const -> const Extents<D>&
    {
        return m_extents;
    }

    auto extents(const Extents<D> &extents) -> BasicSliceViewNode&
    {
        m_extents = extents;
        return *this;
    }

    auto extents(const int64_t &i) const -> const int64_t&
    {
        return m_extents[i];
    }

    auto extents(const int64_t &i, const int64_t &v) -> BasicSliceViewNode&
    {
        m_extents[i] = v;
        return *this;
    }

    auto strides() const -> const Extents<D>&
    {
        return m_strides;
    }

    auto strides(const Extents<D> &strides) -> BasicSliceViewNode&
    {
        m_strides = strides;
        return *this;
    }

    auto strides(const int64_t &i) const -> const int64_t&
    {
        return m_strides[i];
    }

    auto strides(const int64_t &i, const int64_t &v) -> BasicSliceViewNode&
    {
        m_strides[i] = v;
        return *this;
    }

    auto size() const -> int64_t
    {
        return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
    }

    auto map(const Extents<D> &view_indices) const -> Extents<Bare<A>::dimension()>
    {
        Extents<Bare<A>::dimension()> array_indices = m_origin;

        for (int64_t k = 0; k < D; k++)
        {
            array_indices[ViewIndexSubspace[k]] += view_indices[k] * m_strides[k];
        }

        return array_indices;
    }

    constexpr auto p_elements() const -> ElementPointer
    {
        return m_a.p_elements();
    }

    auto begin() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicSliceViewNode>,
            IndexTupleIterator<BasicSliceViewNode>
        >
    {
        return
            std::conditional_t <
                IsReadOnly,
                ReadOnlyIndexTupleIterator<BasicSliceViewNode>,
                IndexTupleIterator<BasicSliceViewNode>
            >::begin_of(this);
    }

    auto cbegin() const -> ReadOnlyIndexTupleIterator<BasicSliceViewNode>
    {
        return ReadOnlyIndexTupleIterator<BasicSliceViewNode>::cbegin_of(this);
    }

    auto end() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicSliceViewNode>,
            IndexTupleIterator<BasicSliceViewNode>
        >
    {
        return
            std::conditional_t <
                IsReadOnly,
                ReadOnlyIndexTupleIterator<BasicSliceViewNode>,
                IndexTupleIterator<BasicSliceViewNode>
            >::end_of(this);
    }

    auto cend() const -> ReadOnlyIndexTupleIterator<BasicSliceViewNode>
    {
        return ReadOnlyIndexTupleIterator<BasicSliceViewNode>::cend_of(this);
    }

private:

    ExpressionNodeMemberStorage<A> m_a;

    Extents<Bare<A>::dimension()> m_origin  = {};
    Extents<D>              m_extents = {};
    Extents<D>              m_strides = {};

};

template<int64_t D, Extents<D> ViewIndexSubspace, typename A>
auto make_slice_view (
    A&& a,
    const Extents<Bare<A>::dimension()> &origin  = make_extents_filled<Bare<A>::dimension()>(0),
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent),
    const Extents<D> &strides = make_extents_filled<D>(1)
)
{
    return SliceViewNode<A, D, ViewIndexSubspace>(std::forward<A>(a), origin, extents, strides);
}

template<int64_t D, Extents<D> ViewIndexSubspace, typename A>
auto make_read_only_slice_view (
    A&& a,
    const Extents<Bare<A>::dimension()> &origin  = make_extents_filled<Bare<A>::dimension()>(0),
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent),
    const Extents<D> &strides = make_extents_filled<D>(1)
)
{
    return ReadOnlySliceViewNode<A, D, ViewIndexSubspace>(std::forward<A>(a), origin, extents, strides);
}

/**
 * @brief `BasicBroadcastViewNode` class template.
 *
 * @tparam A                  The array-like type of the object we want the view to refer to, typically an `Array` or another `View`.
 * @tparam IsReadOnly         Whether the view is read-only.
 * @tparam D                  Dimension of the view. Must be equal to or bigger than the dimension of `A`.
 * @tparam AIndexSubspace     An `Extents<Bare<A>::dimension()>` specifying the view axis indices along which the viewed object extends.
 *                            Must be a strictly increasing sequence.
 */

template <
    typename A,
    bool IsReadOnly,
    int64_t D,
    Extents<Bare<A>::dimension()> AIndexSubspace
>
class BasicBroadcastViewNode
{

    static_assert(D > 0);
    static_assert(D >= Bare<A>::dimension());
    static_assert(axis_selection_valid<D, Bare<A>::dimension()>(AIndexSubspace));
    static_assert(AIndexSubspace[Bare<A>::dimension() - 1] < D);

public:

    using Element = Bare<A>::Element;

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    using ElementAccess =
        std::conditional_t <
            ArrayType<A> || ViewType<A>,
            ElementReference,
            typename Bare<A>::Element
        >;

    explicit BasicBroadcastViewNode (
        A&& a,
        const Extents<D> &extents
    )
    : m_a ( std::forward<A>(a) ), m_extents(extents)
    {}

    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementAccess
    {
        return operator[]({int64_t(i)...});
    }

    auto operator[](const Extents<D> &indices) const -> ElementAccess
    {
        return m_a[map(indices)];
    }

    static consteval auto dimension() -> int64_t
    {
        return D;
    }

    static consteval auto is_owning_type() -> bool
    {
        return false;
    }

    static consteval auto is_of_static_extents() -> bool
    {
        return false;
    }

    static consteval auto type_extents() -> Extents<D>
    {
        return make_extents_filled<D>(dynamic_extent);
    }

    auto is_identity() const -> bool
    {
        if constexpr (dimension() == Bare<A>::dimension())
        {
            return extents() == m_a.extents();
        }

        return false;
    }

    auto is_identity_chain() const -> bool
    {
        if constexpr (A::is_owning_type())
        {
            return is_identity();
        }
        else
        {
            return is_identity() && m_a.is_identity_chain();
        }
    }

    auto extents() const -> const Extents<D>&
    {
        return m_extents;
    }

    auto extents(const Extents<D> &extents) -> BasicBroadcastViewNode&
    {
        m_extents = extents;
        return *this;
    }

    auto extents(const int64_t &i) const -> const int64_t&
    {
        return m_extents[i];
    }

    auto extents(const int64_t &i, const int64_t &v) -> BasicBroadcastViewNode&
    {
        m_extents[i] = v;
        return *this;
    }

    auto size() const -> int64_t
    {
        return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
    }

    auto map(const Extents<D> &view_indices) const -> Extents<Bare<A>::dimension()>
    {
        Extents<Bare<A>::dimension()> array_indices = {};

        for (int64_t i = 0; i < Bare<A>::dimension(); i++)
        {
            array_indices[i] = view_indices[AIndexSubspace[i]] % m_a.extents(i);
        }

        return array_indices;
    }

    constexpr auto p_elements() const -> ElementPointer
    {
        return m_a.p_elements();
    }

    auto begin() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>,
            IndexTupleIterator<BasicBroadcastViewNode>
        >
    {
        return
            std::conditional_t <
                IsReadOnly,
                ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>,
                IndexTupleIterator<BasicBroadcastViewNode>
            >::begin_of(this);
    }

    auto cbegin() const -> ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>
    {
        return ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>::cbegin_of(this);
    }

    auto end() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>,
            IndexTupleIterator<BasicBroadcastViewNode>
        >
    {
        return
            std::conditional_t <
                IsReadOnly,
                ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>,
                IndexTupleIterator<BasicBroadcastViewNode>
            >::end_of(this);
    }

    auto cend() const -> ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>
    {
        return ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>::cend_of(this);
    }

private:

    ExpressionNodeMemberStorage<A> m_a;

    Extents<D> m_extents = {};
};

template<int64_t D, Extents<D> AIndexSubspace, typename A>
auto make_broadcast_view (
    A&& a,
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent)
)
{
    return BroadcastViewNode<A, D, AIndexSubspace>(std::forward<A>(a), extents);
}

template<int64_t D, Extents<D> AIndexSubspace, typename A>
auto make_read_only_broadcast_view (
    A&& a,
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent)
)
{
    return ReadOnlyBroadcastViewNode<A, D, AIndexSubspace>(std::forward<A>(a), extents);
}

#endif // EXPRESSION_NODES_HPP
