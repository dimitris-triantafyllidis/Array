#define ARRAY_IMPLEMENTATION
#include <Array.hpp>

#include <chrono>
#include <print>

auto compute_expression_manual_pointer_walk_1d() -> void
{

    int64_t n = 200000000;

    Array<float, 1> a(n);
    Array<float, 1> b(n);
    Array<float, 1> c(n);
    Array<float, 1> d(n);
    Array<float, 1> e(n);

    Array<float, 1> r(n);

    auto start = std::chrono::high_resolution_clock::now();

    for ( int64_t i = 0; i < n; i++ )
    {
        r.p_elements()[i] =
            a.p_elements()[i] +
            b.p_elements()[i] *
            c.p_elements()[i] +
            2 *
            d.p_elements()[i] *
            e.p_elements()[i] *
            3;
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto compute_expression_manual_indexing_1d() -> void
{

    int64_t n = 200000000;

    Array<float, 1> a(n);
    Array<float, 1> b(n);
    Array<float, 1> c(n);
    Array<float, 1> d(n);
    Array<float, 1> e(n);

    Array<float, 1> r(n);

    auto start = std::chrono::high_resolution_clock::now();

    for ( int64_t i = 0; i < n; i++ )
    {
        r[i] = a[i] + b[i] * c[i] + 2 * d[i] * e[i] * 3;
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto compute_expression_manual_iterators_1d() -> void
{

    int64_t n = 200000000;

    Array<float, 1> a(n);
    Array<float, 1> b(n);
    Array<float, 1> c(n);
    Array<float, 1> d(n);
    Array<float, 1> e(n);

    Array<float, 1> r(n);

    auto it_a = a.cbegin();
    auto it_b = b.cbegin();
    auto it_c = c.cbegin();
    auto it_d = d.cbegin();
    auto it_e = e.cbegin();

    auto it_r = r.begin();

    auto start = std::chrono::high_resolution_clock::now();

    while (it_r != r.end())
    {
        *it_r = *it_a + *it_b * *it_c + 2 * *it_d * *it_e * 3;
        it_a++;
        it_b++;
        it_c++;
        it_d++;
        it_e++;
        it_r++;
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto compute_expression_1d() -> void
{

    int64_t n = 200000000;

    Array<float, 1> a(n);
    Array<float, 1> b(n);
    Array<float, 1> c(n);
    Array<float, 1> d(n);
    Array<float, 1> e(n);

    Array<float, 1> r(n);

    auto it_r = r.begin();

    auto start = std::chrono::high_resolution_clock::now();

    r = a + b * c + 2 * d * e * 3;

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto compute_expression_manual_pointer_walk_2d() -> void
{

    int64_t n1 = 25000;
    int64_t n2 = 8000;

    Array<float, 2> a(n1, n2);
    Array<float, 2> b(n1, n2);
    Array<float, 2> c(n1, n2);
    Array<float, 2> d(n1, n2);
    Array<float, 2> e(n1, n2);

    Array<float, 2> r(n1, n2);

    auto start = std::chrono::high_resolution_clock::now();

    for ( int64_t i = 0; i < n1 * n2; i++ )
    {
        r.p_elements()[i] =
            a.p_elements()[i] +
            b.p_elements()[i] *
            c.p_elements()[i] +
            2 *
            d.p_elements()[i] *
            e.p_elements()[i] *
            3;
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto compute_expression_manual_indexing_2d() -> void
{

    int64_t n1 = 25000;
    int64_t n2 = 8000;

    Array<float, 2> a(n1, n2);
    Array<float, 2> b(n1, n2);
    Array<float, 2> c(n1, n2);
    Array<float, 2> d(n1, n2);
    Array<float, 2> e(n1, n2);

    Array<float, 2> r(n1, n2);

    auto start = std::chrono::high_resolution_clock::now();

    for ( int64_t i = 0; i < n1; i++ )
    {
        for ( int64_t j = 0; j < n2; j++ )
        {
            r[i, j] = a[i, j] + b[i, j] * c[i, j] + 2 * d[i, j] * e[i, j] * 3;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto compute_expression_manual_iterators_2d() -> void
{

    int64_t n1 = 25000;
    int64_t n2 = 8000;

    Array<float, 2> a(n1, n2);
    Array<float, 2> b(n1, n2);
    Array<float, 2> c(n1, n2);
    Array<float, 2> d(n1, n2);
    Array<float, 2> e(n1, n2);

    Array<float, 2> r(n1, n2);

    auto it_a = a.cbegin();
    auto it_b = b.cbegin();
    auto it_c = c.cbegin();
    auto it_d = d.cbegin();
    auto it_e = e.cbegin();

    auto it_r = r.begin();

    auto start = std::chrono::high_resolution_clock::now();

    while (it_r != r.end())
    {
        *it_r = *it_a + *it_b * *it_c + 2 * *it_d * *it_e * 3;
        it_a++;
        it_b++;
        it_c++;
        it_d++;
        it_e++;
        it_r++;
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto compute_expression_2d() -> void
{

    int64_t n1 = 25000;
    int64_t n2 = 8000;

    Array<float, 2> a(n1, n2);
    Array<float, 2> b(n1, n2);
    Array<float, 2> c(n1, n2);
    Array<float, 2> d(n1, n2);
    Array<float, 2> e(n1, n2);

    Array<float, 2> r(n1, n2);


    auto it_r = r.begin();

    auto start = std::chrono::high_resolution_clock::now();

    r = a + b * c + 2 * d * e * 3;

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto compute_expression_manual_pointer_walk_4d() -> void
{

    int64_t n1 = 250;
    int64_t n2 = 80;
    int64_t n3 = 100;
    int64_t n4 = 100;

    Array<float, 4> a(n1, n2, n3, n4);
    Array<float, 4> b(n1, n2, n3, n4);
    Array<float, 4> c(n1, n2, n3, n4);
    Array<float, 4> d(n1, n2, n3, n4);
    Array<float, 4> e(n1, n2, n3, n4);

    Array<float, 4> r(n1, n2, n3, n4);

    auto start = std::chrono::high_resolution_clock::now();

    for ( int64_t i = 0; i < n1 * n2 * n3 * n4; i++ )
    {
        r.p_elements()[i] =
            a.p_elements()[i] +
            b.p_elements()[i] *
            c.p_elements()[i] +
            2 *
            d.p_elements()[i] *
            e.p_elements()[i] *
            3;
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto compute_expression_manual_indexing_4d() -> void
{

    int64_t n1 = 250;
    int64_t n2 = 80;
    int64_t n3 = 100;
    int64_t n4 = 100;

    Array<float, 4> a(n1, n2, n3, n4);
    Array<float, 4> b(n1, n2, n3, n4);
    Array<float, 4> c(n1, n2, n3, n4);
    Array<float, 4> d(n1, n2, n3, n4);
    Array<float, 4> e(n1, n2, n3, n4);

    Array<float, 4> r(n1, n2, n3, n4);

    auto start = std::chrono::high_resolution_clock::now();

    for ( int64_t i = 0; i < n1; i++ )
    {
        for ( int64_t j = 0; j < n2; j++ )
        {
            for ( int64_t k = 0; k < n3; k++ )
            {
                for ( int64_t l = 0; l < n4; l++ )
                {
                    r[i, j, k, l] = a[i, j, k, l] + b[i, j, k, l] * c[i, j, k, l] + 2 * d[i, j, k, l] * e[i, j, k, l] * 3;
                }
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto compute_expression_manual_iterators_4d() -> void
{

    int64_t n1 = 250;
    int64_t n2 = 80;
    int64_t n3 = 100;
    int64_t n4 = 100;

    Array<float, 4> a(n1, n2, n3, n4);
    Array<float, 4> b(n1, n2, n3, n4);
    Array<float, 4> c(n1, n2, n3, n4);
    Array<float, 4> d(n1, n2, n3, n4);
    Array<float, 4> e(n1, n2, n3, n4);

    Array<float, 4> r(n1, n2, n3, n4);

    auto it_a = a.cbegin();
    auto it_b = b.cbegin();
    auto it_c = c.cbegin();
    auto it_d = d.cbegin();
    auto it_e = e.cbegin();

    auto it_r = r.begin();

    auto start = std::chrono::high_resolution_clock::now();

    while (it_r != r.end())
    {
        *it_r = *it_a + *it_b * *it_c + 2 * *it_d * *it_e * 3;
        it_a++;
        it_b++;
        it_c++;
        it_d++;
        it_e++;
        it_r++;
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto compute_expression_4d() -> void
{

    int64_t n1 = 250;
    int64_t n2 = 80;
    int64_t n3 = 100;
    int64_t n4 = 100;

    Array<float, 4> a(n1, n2, n3, n4);
    Array<float, 4> b(n1, n2, n3, n4);
    Array<float, 4> c(n1, n2, n3, n4);
    Array<float, 4> d(n1, n2, n3, n4);
    Array<float, 4> e(n1, n2, n3, n4);

    Array<float, 4> r(n1, n2, n3, n4);

    auto it_r = r.begin();

    auto start = std::chrono::high_resolution_clock::now();

    r = a + b * c + 2 * d * e * 3;

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (
        end - start
    ).count();

    std::println("Time: {} ms", elapsed);

}

auto main() -> int
{

    compute_expression_manual_pointer_walk_1d();
    compute_expression_manual_indexing_1d();
    compute_expression_manual_iterators_1d();
    compute_expression_1d();

    std::println();

    compute_expression_manual_pointer_walk_2d();
    compute_expression_manual_indexing_2d();
    compute_expression_manual_iterators_2d();
    compute_expression_2d();

    std::println();

    compute_expression_manual_pointer_walk_4d();
    compute_expression_manual_indexing_4d();
    compute_expression_manual_iterators_4d();
    compute_expression_4d();

    return 0;
}

