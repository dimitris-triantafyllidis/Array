#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <cstdint>
#include <string>
#include <source_location>
#include <format>
#include <ranges>
#include <algorithm>

#include "forward_declarations.hpp"

//******************************************************************************
// Helper function for throwing with source location info
//******************************************************************************

template<typename E>
[[noreturn]] constexpr void throw_with_context (
    const std::string& message,
    const std::source_location& location = std::source_location::current()
)
requires std::constructible_from<E, std::string>
{
    std::string full_message =
        std::format (
            "{} @ {}:{} — {}",
            location.function_name(),
            location.file_name(),
            location.line(),
            message
        );
    throw E(full_message);
}


//******************************************************************************
// Helper class for array extents and indices
// Like std::array but does not cause NTTP headaches
//******************************************************************************

template<int64_t N>
struct Extents
{
    int64_t data[N];

    constexpr auto operator[](int64_t i) -> int64_t& { return data[i]; }
    constexpr auto operator[](int64_t i) const -> const int64_t& { return data[i]; }

    constexpr auto size() const -> int64_t { return N; }

    constexpr auto begin() -> int64_t* { return data; }
    constexpr auto end() -> int64_t* { return data + N; }

    constexpr auto begin() const -> const int64_t* { return data; }
    constexpr auto end() const -> const int64_t* { return data + N; }

    constexpr auto cbegin() const -> const int64_t* { return data; }
    constexpr auto cend() const -> const int64_t* { return data + N; }
};

template<int64_t N>
constexpr auto operator==(const Extents<N>& lhs, const Extents<N>& rhs) -> bool
{
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}


//******************************************************************************
// Various helper functions
//******************************************************************************


template<std::integral T>
constexpr auto next_multiple(T n, T f) -> T
{
    if (!(n >= 0 && f > 0))
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }
    return n % f ? (n / f) * f + f : n;
}

template<int64_t D>
constexpr auto all_of_indices_in_bounds(const Extents<D> &indices, const Extents<D> &extents) -> bool
{
    for (int64_t i = 0; i < D; i++)
    {
        if (indices[i] < 0 || indices[i] >= extents[i])
        {
            return false;
        }
    }

    return true;
}

template<int64_t D>
constexpr auto check_bounds(const Extents<D> &indices, const Extents<D> &extents) -> void
{
    if (all_of_indices_in_bounds(indices, extents))
    {
        return;
    }

    throw_with_context<std::out_of_range> (
        std::format("Array indices out of range: indices = {}, extents = {}", indices, extents)
    );
}

template<int64_t D>
constexpr auto extents_intersection(const Extents<D> &e1, const Extents<D> &e2) -> Extents<D>
{
    Extents<D> result;

    for (int64_t i = 0; i < D; i++)
    {
        result[i] = std::min(e1[i], e2[i]);
    }

    return result;
}

template<int64_t D>
constexpr auto all_of_extents_static(const Extents<D> &extents) -> bool
{
    return std::ranges::all_of (
        extents,
        [] (const int64_t &e) -> auto {
            return e != dynamic_extent;
        }
    );
}

template<int64_t D>
constexpr auto all_of_extents_dynamic(const Extents<D> &extents) -> bool
{
    return std::ranges::all_of (
        extents,
        [] (const int64_t &e) -> auto {
            return e == dynamic_extent;
        }
    );
}

template<int64_t D>
constexpr auto all_of_extents_zero(const Extents<D> &extents) -> bool
{
    return std::ranges::all_of (
        extents,
        [] (const int64_t &e) -> auto {
            return e == 0;
        }
    );
}

template<int64_t D>
constexpr auto all_of_extents_positive(const Extents<D> &extents) -> bool
{
    return std::ranges::all_of (
        extents,
        [] (const int64_t &e) -> auto {
            return e > 0;
        }
    );
}

template<int64_t D>
constexpr auto all_of_extents_powers_of_2(const Extents<D> &extents) -> bool
{
    return std::ranges::all_of (
        extents,
        [] (const int64_t &e) -> auto {
            return e > 0 && std::popcount(uint64_t(e)) == 1;
        }
    );
}

template<int64_t D>
constexpr auto all_of_extents_equal(const Extents<D> &extents) -> bool
{
    const int64_t first = extents[0];

    return std::ranges::all_of (
        extents,
        [first] (const int64_t &e) -> auto {
            return e == first;
        }
    );
}

template<int64_t D>
constexpr auto compute_extents_countr_zero(const Extents<D> &extents) -> Extents<D>
{
    Extents<D> result{};

    for (int64_t i = 0; i < D; ++i)
    {
        result[i] = std::countr_zero(uint64_t(extents[i]));
    }

    return result;
}

template<int64_t D>
constexpr auto extents_valid(const Extents<D> &extents) -> bool
{
    return
        ( D > 0 ) &&
        ( all_of_extents_zero    ( extents ) || all_of_extents_positive ( extents ) ) &&
        ( all_of_extents_dynamic ( extents ) || all_of_extents_static   ( extents ) );
}

template<int64_t N>
constexpr auto make_extents_filled(const int64_t &v) -> Extents<N>
{
    Extents<N> a;
    std::fill(a.begin(), a.end(), v);
    return a;
}

template<int64_t N>
constexpr auto make_extents_iota(int64_t min) -> Extents<N>
{
    Extents<N> a;

    for (int64_t i = 0; i < N; i++)
    {
        a[i] = i + min;
    }

    return a;
}

template<int64_t DSpace, int64_t DSubspace>
constexpr auto axis_selection_valid(const Extents<DSubspace> &axes) -> bool
{
    static_assert(DSpace > 0);
    static_assert(DSubspace > 0 && DSubspace <= DSpace);

    if (axes[0] >= DSpace)
    {
        return false;
    }

    for (int64_t i = 1; i < DSubspace; i++)
    {
        if (axes[i] <= axes[i - 1] || axes[i] >= DSpace)
        {
            return false;
        }
    }

    return true;
}

template<int64_t N, typename Tuple>
constexpr auto tuple_take(const Tuple &tuple) -> auto
{
    auto helper =
        [&]<int64_t... I>(std::integer_sequence<int64_t, I...>) -> auto
        {
            return std::make_tuple(std::get<I>(tuple)...);
        };

    return helper(std::make_integer_sequence<int64_t, N>());
}

template<typename T, typename Tuple>
constexpr auto make_array_from_tuple(const Tuple &tuple) -> auto
{
    auto helper =
        [&]<int64_t... I>(std::integer_sequence<int64_t, I...>) -> auto
        {
            Extents<sizeof...(I)> array = {static_cast<T>(std::get<I>(tuple))...};
            return array;
        };

    return helper(std::make_integer_sequence<int64_t, std::tuple_size_v<Tuple>>());
}


#endif // HELPERS_HPP
