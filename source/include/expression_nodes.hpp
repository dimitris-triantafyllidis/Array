#ifndef EXPRESSION_NODES_HPP
#define EXPRESSION_NODES_HPP

#include "forward_declarations.hpp"

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

    UnaryOpNode(Op &&op, E &&e)
    : m_op ( std::forward<Op> ( op ) ),
      m_e  ( std::forward<E>  ( e  ) )
    {}

    auto operator[](const Extents<dimension()> &indices) const -> Element {
        return m_op(m_e[indices]);
    }

    constexpr auto extents() const -> Extents<dimension()> {
        return m_e.extents();
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

    constexpr auto extents() const -> Extents<dimension()> {
        return m_l.extents();
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

    constexpr auto extents() const -> Extents<dimension()> {
        return m_l.extents();
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

    constexpr auto extents() const -> Extents<dimension()> {
        return m_r.extents();
    }

private:

    ExpressionNodeMemberStorage<Op> m_op;
    ExpressionNodeMemberStorage<L>  m_l;
    ExpressionNodeMemberStorage<R>  m_r;

};

template <typename Op, typename L, typename R>
BinaryLScalarOpNode(Op&&, L&&, R&&) -> BinaryLScalarOpNode<Op, L, R>;

#endif // EXPRESSION_NODES_HPP
