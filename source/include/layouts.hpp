#ifndef LAYOUTS_HPP
#define LAYOUTS_HPP

#include <cstdint>
#include <numeric>

#include "helpers.hpp"

//******************************************************************************
// Array layout classes
//******************************************************************************

// Affine layout class

template<int64_t D, Extents<D> AxisPermutation>
class Affine
{

    static_assert(D > 0);
    static_assert(std::ranges::is_permutation(AxisPermutation, make_extents_iota<D>(0)));

public:

    constexpr Affine() = default;

    constexpr explicit Affine(const Extents<D> &extents);

    consteval auto dimension() const -> int64_t;

    constexpr auto size() const  -> int64_t;
    constexpr auto size_stored() const -> int64_t;
    constexpr auto size_allocated() const -> int64_t;

    constexpr auto extents() const -> const Extents<D>&;

    constexpr auto offset(const Extents<D> &indices) const -> int64_t;

    constexpr auto is_contiguous() const -> bool;
    constexpr auto is_always_contiguous() const -> bool;

private:

    Extents<D> m_extents;

};

template <int64_t D, Extents<D> AxisPermutation>
constexpr Affine<D, AxisPermutation>::Affine(const Extents<D> &extents)
: m_extents(extents)
{}

template <int64_t D, Extents<D> AxisPermutation>
consteval auto Affine<D, AxisPermutation>::dimension() const -> int64_t
{
    return D;
}

template <int64_t D, Extents<D> AxisPermutation>
constexpr auto Affine<D, AxisPermutation>::size() const -> int64_t
{
    return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
}

template <int64_t D, Extents<D> AxisPermutation>
constexpr auto Affine<D, AxisPermutation>::size_stored() const -> int64_t
{
    return size();
}

template <int64_t D, Extents<D> AxisPermutation>
constexpr auto Affine<D, AxisPermutation>::size_allocated() const -> int64_t
{
    return size();
}

template <int64_t D, Extents<D> AxisPermutation>
constexpr auto Affine<D, AxisPermutation>::extents() const -> const Extents<D>&
{
    return m_extents;
}

template <int64_t D, Extents<D> AxisPermutation>
constexpr auto Affine<D, AxisPermutation>::offset(const Extents<D> &indices) const -> int64_t
{

#ifdef ARRAY_BOUNDS_CHECKING_ON
    check_bounds(indices, m_extents);
#endif

    int64_t offset = indices[AxisPermutation[D - 1]];

    if constexpr (D > 1)
    {
        int64_t stride = m_extents[AxisPermutation[D - 1]];
        offset += indices[AxisPermutation[D - 2]] * stride;

        for (int64_t i = D - 2; i > 0; i--)
        {
            stride *= m_extents[AxisPermutation[i]];
            offset += indices[AxisPermutation[i - 1]] * stride;
        }
    }

    return offset;
}

template <int64_t D, Extents<D> AxisPermutation>
constexpr auto Affine<D, AxisPermutation>::is_contiguous() const -> bool
{
    return true;
}

template <int64_t D, Extents<D> AxisPermutation>
constexpr auto Affine<D, AxisPermutation>::is_always_contiguous() const -> bool
{
    return true;
}

// Blocked layout class

template <
    int64_t D,
    Extents<D> AxisPermutation = make_extents_iota<D>(0),
    Extents<D> BlockExtents = make_extents_filled<D>(1)
>
class Blocked
{

    static_assert(D > 0);
    static_assert(all_of_extents_powers_of_2(BlockExtents));
    static_assert(std::ranges::is_permutation(AxisPermutation, make_extents_iota<D>(0)));

public:

    constexpr Blocked() = default;

    constexpr explicit Blocked(const Extents<D> &extents);

    consteval auto dimension() const -> int64_t;

    constexpr auto size() const  -> int64_t;
    constexpr auto size_stored() const -> int64_t;
    constexpr auto size_allocated() const -> int64_t;

    constexpr auto extents() const -> const Extents<D>&;

    constexpr auto offset(const Extents<D> &indices) const -> int64_t;

    constexpr auto is_contiguous() const -> bool;
    constexpr auto is_always_contiguous() const -> bool;

private:

    Extents<D> m_extents;
    Extents<D> m_extents_allocated;
    static constexpr Extents<D> s_block_extents = BlockExtents;
    static constexpr Extents<D> s_block_extents_log2 = compute_extents_countr_zero(BlockExtents);
    static constexpr int64_t s_block_size = std::reduce(s_block_extents.begin(),s_block_extents.end(), int64_t(1), std::multiplies{});

};

template <int64_t D, Extents<D> AxisPermutation, Extents<D> BlockExtents>
constexpr Blocked<D, AxisPermutation, BlockExtents>::Blocked(const Extents<D> &extents)
: m_extents(extents)
{
    for (int64_t i = 0; i < D; i++)
    {
        m_extents_allocated[i] = next_multiple(m_extents[i], s_block_extents[i]);
    }
}

template <int64_t D, Extents<D> AxisPermutation, Extents<D> BlockExtents>
consteval auto Blocked<D, AxisPermutation, BlockExtents>::dimension() const -> int64_t
{
    return D;
}

template <int64_t D, Extents<D> AxisPermutation, Extents<D> BlockExtents>
constexpr auto Blocked<D, AxisPermutation, BlockExtents>::size() const -> int64_t
{
    return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
}

template <int64_t D, Extents<D> AxisPermutation, Extents<D> BlockExtents>
constexpr auto Blocked<D, AxisPermutation, BlockExtents>::size_stored() const -> int64_t
{
    return size_allocated();
}

template <int64_t D, Extents<D> AxisPermutation, Extents<D> BlockExtents>
constexpr auto Blocked<D, AxisPermutation, BlockExtents>::size_allocated() const -> int64_t
{
    return std::reduce(m_extents_allocated.begin(), m_extents_allocated.end(), int64_t(1), std::multiplies{});
}

template <int64_t D, Extents<D> AxisPermutation, Extents<D> BlockExtents>
constexpr auto Blocked<D, AxisPermutation, BlockExtents>::extents() const -> const Extents<D>&
{
    return m_extents;
}

template <int64_t D, Extents<D> AxisPermutation, Extents<D> BlockExtents>
constexpr auto Blocked<D, AxisPermutation, BlockExtents>::offset(const Extents<D> &indices) const -> int64_t
{

#ifdef ARRAY_BOUNDS_CHECKING_ON
    check_bounds(indices, m_extents);
#endif

    int64_t block_start_offset = indices[AxisPermutation[D - 1]] >> s_block_extents_log2[AxisPermutation[D - 1]];

    if constexpr (D > 1)
    {
        int64_t stride = m_extents[AxisPermutation[D - 1]] >> s_block_extents_log2[AxisPermutation[D - 1]];
        block_start_offset += (indices[AxisPermutation[D - 2]] >> s_block_extents_log2[AxisPermutation[D - 2]]) * stride;

        for(int64_t i = D - 2; i > 0; i--)
        {
            stride *= m_extents[AxisPermutation[i]] >> s_block_extents_log2[AxisPermutation[i]];
            block_start_offset += (indices[AxisPermutation[i - 1]] >> s_block_extents_log2[AxisPermutation[i - 1]]) * stride;
        }
    }

    block_start_offset *= s_block_size;

    int64_t offset = block_start_offset + (indices[AxisPermutation[D - 1]] % s_block_extents[AxisPermutation[D - 1]]);

    if constexpr (D > 1)
    {
        int64_t stride = s_block_extents[AxisPermutation[D - 1]];
        offset += (indices[AxisPermutation[D - 2]] % s_block_extents[AxisPermutation[D - 2]]) * stride;

        for(int64_t i = D - 2; i > 0; i--)
        {
            stride *= s_block_extents[AxisPermutation[i]];
            offset += (indices[AxisPermutation[i - 1]] % s_block_extents[AxisPermutation[i - 1]]) * stride;
        }
    }

    return offset;
}

template <int64_t D, Extents<D> AxisPermutation, Extents<D> BlockExtents>
constexpr auto Blocked<D, AxisPermutation, BlockExtents>::is_contiguous() const -> bool
{
    for (int64_t i = 1; i < D; i++)
    {
        if (m_extents[AxisPermutation[i]] == m_extents_allocated[AxisPermutation[i]])
        {
            return false;
        }
    }

    return true;
}

template <int64_t D, Extents<D> AxisPermutation, Extents<D> BlockExtents>
constexpr auto Blocked<D, AxisPermutation, BlockExtents>::is_always_contiguous() const -> bool
{
    return false;
}

// Morton layout class

template <int64_t D>
class Morton
{

    static_assert(D > 0);

public:

    constexpr Morton() = default;

    constexpr explicit Morton(const Extents<D> &extents);

    consteval auto dimension() const -> int64_t;

    constexpr auto size() const  -> int64_t;
    constexpr auto size_stored() const -> int64_t;
    constexpr auto size_allocated() const -> int64_t;

    constexpr auto extents() const -> const Extents<D>&;

    constexpr auto offset(const Extents<D> &indices) const -> int64_t;

    constexpr auto is_contiguous() const -> bool;
    constexpr auto is_always_contiguous() const -> bool;

private:

    Extents<D> m_extents;
};

template <int64_t D>
constexpr Morton<D>::Morton(const Extents<D> &extents)
: m_extents(extents)
{
    if (
        !all_of_extents_powers_of_2(extents) ||
        !all_of_extents_equal(extents)
    )
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }
}

template <int64_t D>
consteval auto Morton<D>::dimension() const -> int64_t
{
    return D;
}

template <int64_t D>
constexpr auto Morton<D>::size() const -> int64_t
{
    return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
}

template <int64_t D>
constexpr auto Morton<D>::size_stored() const -> int64_t
{
    return size();
}

template <int64_t D>
constexpr auto Morton<D>::size_allocated() const -> int64_t
{
    return size();
}

template <int64_t D>
constexpr auto Morton<D>::extents() const -> const Extents<D>&
{
    return m_extents;
}

template <int64_t D>
constexpr auto Morton<D>::offset(const Extents<D> &indices) const -> int64_t
{

#ifdef ARRAY_BOUNDS_CHECKING_ON
    check_bounds(indices, m_extents);
#endif

    int64_t offset = 0;

    for (int64_t extents_bit = 0; extents_bit <= std::countr_zero(static_cast<uint64_t>(m_extents[0])); extents_bit++)
    {
        for (int64_t extent = 0; extent < m_extents.size(); extent++)
        {
            offset |= ((indices[extent] >> extents_bit) & int64_t(1)) << (extents_bit * m_extents.size() + m_extents.size() - (extent + 1));
        }
    }

    return offset;
}

template <int64_t D>
constexpr auto Morton<D>::is_contiguous() const -> bool
{
    return true;
}

template <int64_t D>
constexpr auto Morton<D>::is_always_contiguous() const -> bool
{
    return true;
}

#endif // LAYOUTS_HPP