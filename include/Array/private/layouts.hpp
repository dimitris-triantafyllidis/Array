#ifndef LAYOUTS_HPP
#define LAYOUTS_HPP

#include <cstdint>
#include <numeric>

#include "helpers.hpp"

//******************************************************************************
// Array layout classes
//******************************************************************************

// Blocked layout class

template <
    int64_t D,
    Extents<D> AxisPermutation,
    Extents<D> B1Extents,
    Extents<D> B1AxisPermutation,
    Extents<D> B2Extents,
    Extents<D> B2AxisPermutation,
    Extents<D> B3Extents,
    Extents<D> B3AxisPermutation
>
class Blocked
{

    static_assert(D > 0);
    static_assert(std::ranges::is_permutation(AxisPermutation, make_extents_iota<D>(0)));

    static_assert(all_of_extents_powers_of_2(B1Extents));
    static_assert(std::ranges::is_permutation(B1AxisPermutation, make_extents_iota<D>(0)));

    static_assert(all_of_extents_powers_of_2(B2Extents));
    static_assert(std::ranges::is_permutation(B2AxisPermutation, make_extents_iota<D>(0)));

    static_assert(all_of_extents_powers_of_2(B3Extents));
    static_assert(std::ranges::is_permutation(B3AxisPermutation, make_extents_iota<D>(0)));

    static_assert(std::ranges::equal(B1Extents, B2Extents, std::ranges::greater_equal{}));
    static_assert(std::ranges::equal(B2Extents, B3Extents, std::ranges::greater_equal{}));

public:

    constexpr Blocked() = default;

    constexpr explicit Blocked(const Extents<D> &extents)
    : m_extents(extents)
    {
        for (int64_t i = 0; i < D; i++)
        {
            m_extents_allocated[i] = next_multiple(m_extents[i], s_b1_extents[i]);
        }
    }

    consteval auto dimension() const -> int64_t
    {
        return D;
    }

    constexpr auto size() const  -> int64_t
    {
        return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
    }

    constexpr auto size_stored() const -> int64_t
    {
        return size();
    }

    constexpr auto size_allocated() const -> int64_t
    {
        return std::reduce(m_extents_allocated.begin(), m_extents_allocated.end(), int64_t(1), std::multiplies{});
    }

    constexpr auto extents() const -> const Extents<D>&
    {
        return m_extents;
    }

    constexpr auto offset(const Extents<D> &indices) const -> int64_t
    {

    #ifdef ARRAY_BOUNDS_CHECKING_ON
        check_bounds(indices, m_extents);
    #endif

        constexpr int64_t eff_lvl =
            s_b3_size > 1 ? 4 :
            s_b2_size > 1 ? 3 :
            s_b1_size > 1 ? 2 :
            1;

        constexpr int64_t eff_dim = eff_lvl * D;

        Extents<eff_dim> eff_ext;
        Extents<eff_dim> eff_ax_perm;
        Extents<eff_dim> eff_ind;

        for ( int64_t i = 0 * D; i < 1 * D; i++ )
        {
            eff_ext[i]     = m_extents_allocated[i - 0 * D] / s_b1_extents[i - 0 * D];
            eff_ax_perm[i] = s_axis_permutation[i - 0 * D] + 0 * D;
            eff_ind[i]     = indices[i - 0 * D] / s_b1_extents[i - 0 * D];
        }

        if constexpr (eff_lvl > 1)
        {
            for ( int64_t i = 1 * D; i < 2 * D; i++ )
            {
                eff_ext[i]     = s_b1_extents[i - 1 * D] / s_b2_extents[i - 1 * D];
                eff_ax_perm[i] = s_b1_axis_permutation[i - 1 * D] + 1 * D;
                eff_ind[i]     = ((indices[i - 1 * D] % s_b1_extents[i - 1 * D]) / s_b2_extents[i - 1 * D]);
            }
        }

        if constexpr (eff_lvl > 2)
        {
            for ( int64_t i = 2 * D; i < 3 * D; i++ )
            {
                eff_ext[i]     = s_b2_extents[i - 2 * D] / s_b3_extents[i - 2 * D];
                eff_ax_perm[i] = s_b2_axis_permutation[i - 2 * D] + 2 * D;
                eff_ind[i]     = ((indices[i - 2 * D] % s_b2_extents[i - 2 * D]) / s_b3_extents[i - 2 * D]);
            }
        }

        if constexpr (eff_lvl > 3)
        {
            for ( int64_t i = 3 * D; i < 4 * D; i++ )
            {
                eff_ext[i]     = s_b3_extents[i - 3 * D];
                eff_ax_perm[i] = s_b3_axis_permutation[i - 3 * D] + 3 * D;
                eff_ind[i]     = indices[i - 3 * D] % s_b3_extents[i - 3 * D];
            }
        }

        int64_t offset = eff_ind[eff_ax_perm[eff_dim - 1]];

        if constexpr (eff_dim > 1)
        {
            int64_t stride = eff_ext[eff_ax_perm[eff_dim - 1]];
            offset += eff_ind[eff_ax_perm[eff_dim - 2]] * stride;

            for(int64_t i = eff_dim - 2; i > 0; i--)
            {
                stride *= eff_ext[eff_ax_perm[i]];
                offset += eff_ind[eff_ax_perm[i - 1]] * stride;
            }
        }

        return offset;

    }

    constexpr auto is_contiguous() const -> bool
    {
        return size() == size_allocated();
    }

    consteval auto is_always_contiguous() const -> bool
    {
        return s_b1_size_log2 == s_b2_size_log2 == s_b3_size_log2 == 0;
    }

private:

    Extents<D> m_extents;
    Extents<D> m_extents_allocated;

    static constexpr Extents<D> s_axis_permutation = AxisPermutation;

    static constexpr Extents<D> s_b1_extents          = B1Extents;
    static constexpr Extents<D> s_b1_extents_log2     = compute_extents_countr_zero(B1Extents);
    static constexpr Extents<D> s_b1_axis_permutation = B1AxisPermutation;

    static constexpr Extents<D> s_b2_extents          = B2Extents;
    static constexpr Extents<D> s_b2_extents_log2     = compute_extents_countr_zero(B2Extents);
    static constexpr Extents<D> s_b2_axis_permutation = B2AxisPermutation;

    static constexpr Extents<D> s_b3_extents          = B3Extents;
    static constexpr Extents<D> s_b3_extents_log2     = compute_extents_countr_zero(B3Extents);
    static constexpr Extents<D> s_b3_axis_permutation = B3AxisPermutation;

    static constexpr int64_t s_b1_size = std::reduce(s_b1_extents.begin(), s_b1_extents.end(), int64_t(1), std::multiplies{});
    static constexpr int64_t s_b2_size = std::reduce(s_b2_extents.begin(), s_b2_extents.end(), int64_t(1), std::multiplies{});
    static constexpr int64_t s_b3_size = std::reduce(s_b3_extents.begin(), s_b3_extents.end(), int64_t(1), std::multiplies{});

    static constexpr int64_t s_b1_size_log2 = std::reduce(s_b1_extents_log2.begin(), s_b1_extents_log2.end(), int64_t(0), std::plus{});
    static constexpr int64_t s_b2_size_log2 = std::reduce(s_b2_extents_log2.begin(), s_b2_extents_log2.end(), int64_t(0), std::plus{});
    static constexpr int64_t s_b3_size_log2 = std::reduce(s_b3_extents_log2.begin(), s_b3_extents_log2.end(), int64_t(0), std::plus{});

};

#endif // LAYOUTS_HPP