#define ARRAY_BOUNDS_CHECKING_ON
#define ARRAY_IMPLEMENTATION
#include "Array.hpp"

#include <print>
#include <ranges>
#include <stack>
#include <chrono>

auto constructors() -> void
{

    // A dynamic 10 x 10 x 10 array of floats with the extents passed to the constructor

    Array<float, 3> a(10, 10, 10);
    a[4, 4, 4] = 4;

    // A dynamic 2 x 3 array from an std::initializer_list

    Array<float, 2> b = {
        {0, 1, 2},
        {3, 4, 5}
    };

    // A static array of 3 floats

    constexpr Array<float, 1, {3}> c = {0, 1, 2};
    static_assert(c[0] == 0);

    // A static 4 x 4 array of floats from an std::initializer_list

    constexpr Array<float, 2, {4, 4}> d = {
         {1, 0, 0, 0},
         {0, 1, 0, 0},
         {0, 0, 1, 0},
         {0, 0, 0, 1}
    };
    static_assert(d[0, 0] == 1);

    // A static 2 x 2 x 2 array of floats from an std::initializer_list

    constexpr Array<float, 3, {2, 2, 2}> e = {
        {
            {0, 1},
            {2, 3}
        },
        {
            {4, 5},
            {6, 7}
        }
    };

    std::println("{}", e[1, 1, 1]);

    static_assert(e[0, 0, 0] == 0);

}

auto iterators() -> void
{
    Array<float, 2, {dynamic_extent, dynamic_extent}, Morton<2>> a(4, 4);

    float f = 0;

    for (auto &e : a)
    {
        e = f++;
    }

    for (auto it = a.cbegin(); it != a.cend(); it++)
    {
        std::println("b{} = {}", it.cursor(), *it);
    }

    for (int64_t i = 0; i < a.size(); i++)
    {
        std::println("{}", a.p_elements()[i]);
    }

}

auto views() -> void
{
    Array<float, 2> b = {
        {1, 2, 3},
        {4, 5, 6}
    };

    auto bv = make_broadcast_view<2, {0, 1}>(b, {4, 6});
    auto bvv = make_read_only_slice_view<2, {0, 1}>(bv, {4, 6});

    for (auto it = bvv.begin(); it != bvv.end(); it++)
    {
        std::println("b{} = {}", it.cursor(), *it);
    }

    std::println("{}", b);

}

auto numerics() -> void
{
    Array<int, 1, {2}> a = {2, 3};

    auto b = a + a * 3;
}

auto main() -> int
{
    constructors();
    iterators();
    views();
    numerics();

    return 0;
}