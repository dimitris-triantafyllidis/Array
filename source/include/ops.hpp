#ifndef OPS_HPP
#define OPS_HPP

#include "include/forward_declarations.hpp"
#include "include/helpers.hpp"

//******************************************************************************
// Unary and binary operations
//******************************************************************************

template <typename Op, typename L, typename R>
requires (ArrayType<L> && ArrayType<R>)
auto binary_op_assign(const Op &op, L &lhs, const R &rhs) -> L&
{
    if (lhs.extents() != rhs.extents())
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }

    std::transform (
        lhs.cbegin(), lhs.cend(),
        rhs.cbegin(),
        lhs.begin(),
        op
    );

    return lhs;
}

template <typename Op, typename L, typename R>
requires ((ArrayType<L> || ViewType<R>) && ScalarNumType<R>)
auto binary_op_assign_r_scalar(const Op &op, L &lhs, const R &rhs) -> L&
{

    auto lhs_it = lhs.begin();

    while (lhs_it != lhs.end())
    {
        *lhs_it = op(*lhs_it, rhs);
        lhs_it++;
    }

    return lhs;
}

// ExpressionNode op ExpressionNode

template <typename L, typename R>
requires ( ExpressionNodeType<L> ) && ( ExpressionNodeType<R> )
auto operator+(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryZipMapOpNode (
        std::plus<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ExpressionNodeType<L> ) && ( ExpressionNodeType<R> )
auto operator-(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryZipMapOpNode (
        std::minus<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ExpressionNodeType<L> ) && ( ExpressionNodeType<R> )
auto operator*(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryZipMapOpNode (
        std::multiplies<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ExpressionNodeType<L> ) && ( ExpressionNodeType<R> )
auto operator/(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryZipMapOpNode (
        std::divides<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ExpressionNodeType<L> ) && ( ExpressionNodeType<R> )
auto operator%(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryZipMapOpNode (
        std::modulus<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

// ExpressionNode op Scalar

template <typename L, typename R>
requires ( ExpressionNodeType<L> && ScalarNumType<R> )
auto operator+(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryRScalarOpNode (
        std::plus<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ExpressionNodeType<L> && ScalarNumType<R> )
auto operator-(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryRScalarOpNode (
        std::minus<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ExpressionNodeType<L> && ScalarNumType<R> )
auto operator*(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryRScalarOpNode (
        std::multiplies<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ExpressionNodeType<L> && ScalarNumType<R> )
auto operator/(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryRScalarOpNode (
        std::divides<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ExpressionNodeType<L> && ScalarNumType<R> )
auto operator%(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryRScalarOpNode (
        std::modulus<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

// Scalar op ExpressionNode

template <typename L, typename R>
requires ( ScalarNumType<L> && ExpressionNodeType<R> )
auto operator+(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryLScalarOpNode (
        std::plus<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ScalarNumType<L> && ExpressionNodeType<R> )
auto operator-(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryLScalarOpNode (
        std::minus<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ScalarNumType<L> && ExpressionNodeType<R> )
auto operator*(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryLScalarOpNode (
        std::multiplies<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ScalarNumType<L> && ExpressionNodeType<R> )
auto operator/(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryLScalarOpNode (
        std::divides<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

template <typename L, typename R>
requires ( ScalarNumType<L> && ExpressionNodeType<R> )
auto operator%(L &&lhs, R &&rhs) -> decltype(auto) {
    return BinaryLScalarOpNode (
        std::modulus<> {},
        std::forward<L>(lhs),
        std::forward<R>(rhs)
    );
}

// Array op-assign Array

template <typename L, typename R>
requires ( ArrayType<L> && ArrayType<R> )
auto operator+=(L &lhs, const R &rhs) -> L& { return binary_op_assign ( std::plus<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> && ArrayType<R> )
auto operator-=(L &lhs, const R &rhs) -> L& { return binary_op_assign ( std::minus<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> && ArrayType<R> )
auto operator*=(L &lhs, const R &rhs) -> L& { return binary_op_assign ( std::multiplies<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> && ArrayType<R> )
auto operator/=(L &lhs, const R &rhs) -> L& { return binary_op_assign ( std::divides<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> && ArrayType<R> )
auto operator%=(L &lhs, const R &rhs) -> L& { return binary_op_assign ( std::modulus<> {}, lhs, rhs ); }

// (Array | View ) op-assign Scalar

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ScalarNumType<R> )
auto operator+=(L &lhs, const R &rhs) -> L& { return binary_op_assign_r_scalar ( std::plus<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ScalarNumType<R> )
auto operator-=(L &lhs, const R &rhs) -> L& { return binary_op_assign_r_scalar ( std::minus<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ScalarNumType<R> )
auto operator*=(L &lhs, const R &rhs) -> L& { return binary_op_assign_r_scalar ( std::multiplies<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ScalarNumType<R> )
auto operator/=(L &lhs, const R &rhs) -> L& { return binary_op_assign_r_scalar ( std::divides<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ScalarNumType<R> )
auto operator%=(L &lhs, const R &rhs) -> L& { return binary_op_assign_r_scalar ( std::modulus<> {}, lhs, rhs ); }

// op ExpressionNode

template <typename A>
requires ( ArrayType<A> || ViewType<A> )
auto operator-(A &&a) -> decltype(auto) {
    return UnaryOpNode (
        std::negate<> {},
        std::forward<A>(a)
    );
}

template <typename A>
requires ( ExpressionNodeType<A> )
auto abs(A &&a) -> decltype(auto) {
    return UnaryOpNode (
        [](const auto &x) {
            using std::abs;
            return abs(x);
        },
        std::forward<A>(a)
    );
}

// Transpose a 2D array

template<typename A>
requires ( ArrayType<A> && A::dimension() == 2 )
auto transpose(A &a, int64_t block_size = 1) -> void
{
    if (
        (a.extents(0) != a.extents(1)) ||
        (a.extents(0) % block_size != 0)
    )
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }

    for(int i = 0; i < a.extents(0); i += block_size)
    for(int j = i; j < a.extents(1); j += block_size)
    {
        if(j == i)
            for(int k = 0; k < block_size; k++)
            for(int l = k + 1; l < block_size; l++)
                std::swap(a[i + k, j + l], a[j + l, i + k]);
        else
            for(int k = 0; k < block_size; k++)
            for(int l = 0; l < block_size; l++)
                std::swap(a[i + k, j + l], a[j + l, i + k]);
    }
}

template<typename A>
requires ( ArrayType<A> && A::dimension() == 2 )
auto transposed(const A &a, int64_t block_size = 1) -> A
{
    if (
        (a.extents(0) % block_size != 0) ||
        (a.extents(1) % block_size != 0)
    )
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }

    A result(a.extents(1), a.extents(0));

    for(int64_t i = 0; i < a.extents(0); i += block_size)
    for(int64_t j = 0; j < a.extents(1); j += block_size)
        for(int64_t l = j; l < j + block_size; l++)
        for(int64_t k = i; k < i + block_size; k++)
            result[l, k] = a[k, l];

    return result;
}

template<typename A>
requires ( ArrayType<A> || ViewType<A> )
auto max(const A &a) -> typename A::Element
{
    if (a.size() == 0)
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }

    typename A::Element result = *a.cbegin();

    for (const auto &e : a)
        if(e > result)
            result = e;

    return result;
}

template<typename A>
requires ( ArrayType<A> || ViewType<A> )
auto norm_L2(const A &a)
{
    if (a.size() == 0)
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }

    using Result = decltype(std::abs(std::declval<typename A::Element>()));
    Result result = 0;

    for (const auto &e : a)
        result += std::norm(e);

    return std::sqrt(result);
}

template<typename E>
requires ( ExpressionNodeType<E> && !(ArrayType<E> || ViewType<E>) )
auto norm_L2(const E &e)
{
    using Result = decltype(std::abs(std::declval<typename E::Element>()));
    Result result = 0;

    Array<Result, E::dimension()> a(e.extents());

    return norm_L2(a);
}

#endif // OPS_HPP