#ifndef FORMATTER_HPP
#define FORMATTER_HPP

#include <format>

#include "forward_declarations.hpp"

//******************************************************************************
// std::formatter specializations
//******************************************************************************

// Specialization for BasicSliceViewNode

template <typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
class std::formatter<BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace>>
{

public:

    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const BasicSliceViewNode<A, IsReadOnly, D, ViewIndexSubspace> &v, FormatContext &ctx) const
    {
        auto out = ctx.out();
        auto it = v.cbegin();

        if (it == v.cend())
        {
            out = std::format_to(out, "[]");
            return out;
        }

        while (it != v.cend())
        {
            if (it == v.cbegin())
            {
                out = std::format_to(out, "{}", std::string(D, '['));
            }
            else if (it != v.cend())
            {
                int64_t n_trailing_zeroes = 0;

                for (int64_t i = D - 1; i >= 0; i--)
                {
                    if (it.cursor()[i] == 0)
                    {
                        n_trailing_zeroes++;
                    }
                    else
                    {
                        break;
                    }
                }

                out = std::format_to(out, "{}", std::string(n_trailing_zeroes, ']'));
                out = std::format_to(out, "{}", std::string(", "));
                out = std::format_to(out, "{}", std::string(n_trailing_zeroes, '['));
            }

            out = std::format_to(out, "{}", *it);

            it++;

            if (it == v.cend())
            {
                out = std::format_to(out, "{}", std::string(D, ']'));
            }
        }

        return out;
    }

};

// Specialization for BasicBroadcastViewNode

template <typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
class std::formatter<BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>>
{

public:

    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace> &v, FormatContext &ctx) const
    {
        auto out = ctx.out();
        auto view = ReadOnlySliceViewNode<BasicBroadcastViewNode<A, IsReadOnly, D, AIndexSubspace>>(v);
        out = std::format_to(out, "{}", view);
        return out;
    }
};

// Specialization for Array

template <typename T, int64_t D, Extents<D> E, typename L>
class std::formatter<Array<T, D, E, L>>
{

public:

    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const Array<T, D, E, L> &a, FormatContext &ctx) const
    {
        auto out = ctx.out();
        auto view = ReadOnlySliceViewNode<Array<T, D, E, L>>(a);
        out = std::format_to(out, "{}", view);
        return out;
    }
};

#endif // FORMATTER_HPP
