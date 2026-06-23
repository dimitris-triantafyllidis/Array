#include "Array.hpp"

#include <print>
#include <ranges>

#include <stack>


auto constructors() -> void
{

    // A dynamic 10 x 10 x 10 array of floats with the extents passed to the constructor

    Array<float, 3> a(10, 10, 10);
    a[4, 4, 4] = 4;

    // A dynamic 2 x 3 array from an std::initializer_list

    Array<float, 2> b = {
        {1, 2, 3},
        {4, 5, 6}
    };

    // A static array of 3 floats

    Array<float, 1, {3}> d = {1, 2, 3};

    // A static 4 x 4 array of floats from an std::initializer_list

    Array<float, 2, {4, 4}> c = {
         {1, 0, 0, 0},
         {0, 1, 0, 0},
         {0, 0, 1, 0},
         {0, 0, 0, 1}
     };

}

auto iterators() -> void
{
    Array<float, 2, {dynamic_extent, dynamic_extent}, Blocked<2, {0, 1}, {4, 2}>> a(6, 6);

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

}

auto main() -> int
{
    constructors();
    iterators();
    views();
}