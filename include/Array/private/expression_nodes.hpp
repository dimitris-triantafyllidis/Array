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
        Op {} ( std::declval<typename Bare<L>::Element>(), std::declval<R>() )
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
        Op {} ( std::declval<typename Bare<R>::Element>(), std::declval<L>() )
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

    using ElementAccess =
        std::conditional_t <
            ArrayType<A> || ViewType<A>,
            ElementReference,
            typename A::Element
        >;

    explicit BasicIndentityViewNode (
        AReference array
    );

    template<typename... I> requires ((sizeof...(I) == A::dimension()) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementAccess;

    auto operator[](const Extents<A::dimension()> &indices) const -> ElementAccess;

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
            ReadOnlyIndexTupleIterator<BasicIndentityViewNode>,
            IndexTupleIterator<BasicIndentityViewNode>
        >;

    auto cbegin() const -> ReadOnlyIndexTupleIterator<BasicIndentityViewNode>;

    auto end() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicIndentityViewNode>,
            IndexTupleIterator<BasicIndentityViewNode>
        >;

    auto cend() const -> ReadOnlyIndexTupleIterator<BasicIndentityViewNode>;

private:

    APointer m_p_array = nullptr;
};

template<typename A, bool IsReadOnly>
BasicIndentityViewNode<A, IsReadOnly>::BasicIndentityViewNode (AReference array)
: m_p_array ( &array )
{}

template<typename A, bool IsReadOnly>
template<typename... I> requires ((sizeof...(I) == A::dimension()) && (std::is_integral_v<I> && ...))
auto BasicIndentityViewNode<A, IsReadOnly>::operator[](I... i) const -> ElementAccess
{
    return operator[]({int64_t(i)...});
}

template<typename A, bool IsReadOnly>
auto BasicIndentityViewNode<A, IsReadOnly>::operator[](const Extents<A::dimension()> &indices) const -> ElementAccess
{
    return (*m_p_array)[map(indices)];
}

template<typename A, bool IsReadOnly>
consteval auto BasicIndentityViewNode<A, IsReadOnly>::dimension() -> int64_t
{
    return A::dimension();
}

template<typename A, bool IsReadOnly>
consteval auto BasicIndentityViewNode<A, IsReadOnly>::is_owning_type() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly>
consteval auto BasicIndentityViewNode<A, IsReadOnly>::is_of_static_extents() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly>
consteval auto BasicIndentityViewNode<A, IsReadOnly>::type_extents() -> Extents<A::dimension()>
{
    return make_extents_filled<A::dimension()>(dynamic_extent);
}

template<typename A, bool IsReadOnly>
consteval auto BasicIndentityViewNode<A, IsReadOnly>::is_identity() const -> bool
{
    return true;
}

template<typename A, bool IsReadOnly>
auto BasicIndentityViewNode<A, IsReadOnly>::is_identity_chain() const -> bool
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
auto BasicIndentityViewNode<A, IsReadOnly>::extents() const -> const Extents<A::dimension()>&
{
    return m_p_array->extents();
}

template<typename A, bool IsReadOnly>
auto BasicIndentityViewNode<A, IsReadOnly>::extents(const int64_t &i) const -> const int64_t&
{
    return m_p_array->extents()[i];
}

template<typename A, bool IsReadOnly>
auto BasicIndentityViewNode<A, IsReadOnly>::size() const -> int64_t
{
    return m_p_array->size();
}

template<typename A, bool IsReadOnly>
auto BasicIndentityViewNode<A, IsReadOnly>::map(const Extents<A::dimension()> &view_indices) const -> Extents<A::dimension()>
{
    return view_indices;
}

template<typename A, bool IsReadOnly>
constexpr auto BasicIndentityViewNode<A, IsReadOnly>::p_elements() const -> ElementPointer
{
    return m_p_array->p_elements();
}

template<typename A, bool IsReadOnly>
auto BasicIndentityViewNode<A, IsReadOnly>::begin() const ->
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

template<typename A, bool IsReadOnly>
auto BasicIndentityViewNode<A, IsReadOnly>::cbegin() const -> ReadOnlyIndexTupleIterator<BasicIndentityViewNode>
{
    return ReadOnlyIndexTupleIterator<BasicIndentityViewNode>::cbegin_of(this);
}

template<typename A, bool IsReadOnly>
auto BasicIndentityViewNode<A, IsReadOnly>::end() const ->
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

template<typename A, bool IsReadOnly>
auto BasicIndentityViewNode<A, IsReadOnly>::cend() const -> ReadOnlyIndexTupleIterator<BasicIndentityViewNode>
{
    return ReadOnlyIndexTupleIterator<BasicIndentityViewNode>::cend_of(this);
}

template<typename A>
auto make_identity_view (
    A& array,
    const Extents<A::dimension()> &extents = make_extents_filled<A::dimension()>(dynamic_extent)
)
{
    return IndentityViewNode<A>(array);
}

template<typename A>
auto make_read_only_identity_view (
    A& array,
    const Extents<A::dimension()> &extents = make_extents_filled<A::dimension()>(dynamic_extent)
)
{
    return ReadOnlyIndentityViewNode<A>(array);
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

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    using ElementAccess =
        std::conditional_t <
            ArrayType<A> || ViewType<A>,
            ElementReference,
            typename A::Element
        >;

    explicit BasicSliceViewNode (
        AReference array,
        const Extents<A::dimension()> &origin  = make_extents_filled<A::dimension()>(0),
        const Extents<D>              &extents = make_extents_filled<D>(dynamic_extent),
        const Extents<D>              &strides = make_extents_filled<D>(1)
    );

    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementAccess;

    auto operator[](const Extents<D> &indices) const -> ElementAccess;

    static consteval auto dimension()            -> int64_t;
    static consteval auto is_owning_type()       -> bool;
    static consteval auto is_of_static_extents() -> bool;
    static consteval auto type_extents()         -> Extents<D>;

    auto is_identity() const -> bool;
    auto is_identity_chain() const -> bool;

    auto origin() const -> const Extents<A::dimension()>&;
    auto origin(const Extents<A::dimension()> &origin) -> BasicSliceViewNode&;

    auto origin(const int64_t &i) const -> const int64_t&;
    auto origin(const int64_t &i, const int64_t &v) -> BasicSliceViewNode&;

    auto extents() const -> const Extents<D>&;
    auto extents(const Extents<D> &extents) -> BasicSliceViewNode&;

    auto extents(const int64_t &i) const -> const int64_t&;
    auto extents(const int64_t &i, const int64_t &v) -> BasicSliceViewNode&;

    auto strides() const -> const Extents<D>&;
    auto strides(const Extents<D> &strides) -> BasicSliceViewNode&;

    auto strides(const int64_t &i) const -> const int64_t&;
    auto strides(const int64_t &i, const int64_t &v) -> BasicSliceViewNode&;

    auto size() const -> int64_t;

    auto map(const Extents<D> &indices) const -> Extents<A::dimension()>;

    constexpr auto p_elements() const -> ElementPointer;

    auto begin() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicSliceViewNode>,
            IndexTupleIterator<BasicSliceViewNode>
        >;

    auto cbegin() const -> ReadOnlyIndexTupleIterator<BasicSliceViewNode>;

    auto end() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicSliceViewNode>,
            IndexTupleIterator<BasicSliceViewNode>
        >;

    auto cend() const -> ReadOnlyIndexTupleIterator<BasicSliceViewNode>;

private:

    APointer m_p_array = nullptr;

    Extents<A::dimension()> m_origin  = {};
    Extents<D>              m_extents = {};
    Extents<D>              m_strides = {};

};

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::BasicSliceViewNode (
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
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::operator[](I... i) const -> ElementAccess
{
    return operator[]({int64_t(i)...});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::operator[](const Extents<D> &indices) const -> ElementAccess
{
    return (*m_p_array)[map(indices)];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::dimension() -> int64_t
{
    return D;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::is_owning_type() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::is_of_static_extents() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::type_extents() -> Extents<D>
{
    return make_extents_filled<D>(dynamic_extent);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::is_identity() const -> bool
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
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::is_identity_chain() const -> bool
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
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::origin() const -> const Extents<A::dimension()>&
{
    return m_origin;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::origin(const Extents<A::dimension()> &origin) -> BasicSliceViewNode&
{
    m_origin = origin;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::origin(const int64_t &i) const -> const int64_t&
{
    return m_origin[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::origin(const int64_t &i, const int64_t &v) -> BasicSliceViewNode&
{
    m_origin[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::extents() const -> const Extents<D>&
{
    return m_extents;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::extents(const Extents<D> &extents) -> BasicSliceViewNode&
{
    m_extents = extents;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::extents(const int64_t &i) const -> const int64_t&
{
    return m_extents[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::extents(const int64_t &i, const int64_t &v) -> BasicSliceViewNode&
{
    m_extents[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::strides() const -> const Extents<D>&
{
    return m_strides;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::strides(const Extents<D> &strides) -> BasicSliceViewNode&
{
    m_strides = strides;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::strides(const int64_t &i) const -> const int64_t&
{
    return m_strides[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::strides(const int64_t &i, const int64_t &v) -> BasicSliceViewNode&
{
    m_strides[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::size() const -> int64_t
{
    return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::map(const Extents<D> &view_indices) const -> Extents<A::dimension()>
{
    Extents<A::dimension()> array_indices = m_origin;

    for (int64_t k = 0; k < D; k++)
    {
        array_indices[ViewIndexSubspace[k]] += view_indices[k] * m_strides[k];
    }

    return array_indices;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
constexpr auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::p_elements() const -> ElementPointer
{
    return m_p_array->p_elements();
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::begin() const ->
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

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::cbegin() const -> ReadOnlyIndexTupleIterator<BasicSliceViewNode>
{
    return ReadOnlyIndexTupleIterator<BasicSliceViewNode>::cbegin_of(this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::end() const ->
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

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>::cend() const -> ReadOnlyIndexTupleIterator<BasicSliceViewNode>
{
    return ReadOnlyIndexTupleIterator<BasicSliceViewNode>::cend_of(this);
}

template<int64_t D, Extents<D> ViewIndexSubspace, typename A>
auto make_slice_view (
    A& array,
    const Extents<A::dimension()> &origin  = make_extents_filled<A::dimension()>(0),
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent),
    const Extents<D> &strides = make_extents_filled<D>(1)
)
{
    return SliceViewNode<A, D, ViewIndexSubspace>(array, origin, extents, strides);
}

template<int64_t D, Extents<D> ViewIndexSubspace, typename A>
auto make_read_only_slice_view (
    A& array,
    const Extents<A::dimension()> &origin  = make_extents_filled<A::dimension()>(0),
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent),
    const Extents<D> &strides = make_extents_filled<D>(1)
)
{
    return ReadOnlySliceViewNode<A, D, ViewIndexSubspace>(array, origin, extents, strides);
}

/**
 * @brief `BasicBroadcastViewNode` class template.
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
class BasicBroadcastViewNode
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

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    using ElementAccess =
        std::conditional_t <
            ArrayType<A> || ViewType<A>,
            ElementReference,
            typename A::Element
        >;

    explicit BasicBroadcastViewNode (
        AReference array,
        const Extents<D> &extents
    );

    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementAccess;

    auto operator[](const Extents<D> &indices) const -> ElementAccess;

    static consteval auto dimension()            -> int64_t;
    static consteval auto is_owning_type()       -> bool;
    static consteval auto is_of_static_extents() -> bool;
    static consteval auto type_extents()         -> Extents<D>;

    auto is_identity() const -> bool;
    auto is_identity_chain() const -> bool;

    auto extents() const -> const Extents<D>&;
    auto extents(const Extents<D> &extents) -> BasicBroadcastViewNode&;

    auto extents(const int64_t &i) const -> const int64_t&;
    auto extents(const int64_t &i, const int64_t &v) -> BasicBroadcastViewNode&;

    auto size() const -> int64_t;

    auto map(const Extents<D> &indices) const -> Extents<A::dimension()>;

    constexpr auto p_elements() const -> ElementPointer;

    auto begin() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>,
            IndexTupleIterator<BasicBroadcastViewNode>
        >;

    auto cbegin() const -> ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>;

    auto end() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>,
            IndexTupleIterator<BasicBroadcastViewNode>
        >;

    auto cend() const -> ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>;

private:

    APointer m_p_array = nullptr;

    Extents<D> m_extents = {};
};

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::BasicBroadcastViewNode (AReference array, const Extents<D> &extents)
: m_p_array ( &array ), m_extents(extents)
{}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...))
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::operator[](I... i) const -> ElementAccess
{
    return operator[]({int64_t(i)...});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::operator[](const Extents<D> &indices) const -> ElementAccess
{
    return (*m_p_array)[map(indices)];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::dimension() -> int64_t
{
    return D;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::is_owning_type() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::is_of_static_extents() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::type_extents() -> Extents<D>
{
    return make_extents_filled<D>(dynamic_extent);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::is_identity() const -> bool
{
    if constexpr (dimension() == A::dimension())
    {
        return extents() == m_p_array->extents();
    }

    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::is_identity_chain() const -> bool
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
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::extents() const -> const Extents<D>&
{
    return m_extents;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::extents(const Extents<D> &extents) -> BasicBroadcastViewNode&
{
    m_extents = extents;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::extents(const int64_t &i) const -> const int64_t&
{
    return m_extents[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::extents(const int64_t &i, const int64_t &v) -> BasicBroadcastViewNode&
{
    m_extents[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::size() const -> int64_t
{
    return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::map(const Extents<D> &view_indices) const -> Extents<A::dimension()>
{
    Extents<A::dimension()> array_indices = {};

    for (int64_t i = 0; i < A::dimension(); i++)
    {
        array_indices[i] = view_indices[AIndexSubspace[i]] % m_p_array->extents(i);
    }

    return array_indices;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
constexpr auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::p_elements() const -> ElementPointer
{
    return m_p_array->p_elements();
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::begin() const ->
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

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::cbegin() const -> ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>
{
    return ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>::cbegin_of(this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::end() const ->
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

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>::cend() const -> ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>
{
    return ReadOnlyIndexTupleIterator<BasicBroadcastViewNode>::cend_of(this);
}

template<int64_t D, Extents<D> AIndexSubspace, typename A>
auto make_broadcast_view (
    A& array,
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent)
)
{
    return BroadcastViewNode<A, D, AIndexSubspace>(array, extents);
}

template<int64_t D, Extents<D> AIndexSubspace, typename A>
auto make_read_only_broadcast_view (
    A& array,
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent)
)
{
    return ReadOnlyBroadcastViewNode<A, D, AIndexSubspace>(array, extents);
}

#endif // EXPRESSION_NODES_HPP
