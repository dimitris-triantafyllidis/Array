#ifndef ARRAY_HPP
#define ARRAY_HPP

#include <concepts>
#include <cstdint>
#include <exception>
#include <utility>
#include <functional>
#include <algorithm>
#include <vector>
#include <queue>
#include <string>
#include <array>
#include <format>
#include <source_location>
#include <numeric>
#include <complex>
#include <thread>
#include <mutex>
#include <future>
#include <print>
#include <cmath>

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
// SingleProducerAsyncTaskQueue
//******************************************************************************

/**
 * A single-producer asynchronous task queue.
 *
 * Tasks are submitted only from the owning thread.
 * Worker threads execute tasks but must not submit new tasks.
 *
 * Destruction waits until all queued tasks have completed.
 */

class SingleProducerAsyncTaskQueue
{

public:

    SingleProducerAsyncTaskQueue();
    ~SingleProducerAsyncTaskQueue();

    template<typename F, typename... Args>
    auto enqueue_task(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    auto queue_size() -> int64_t;

private:

    std::thread                       m_thread;
    std::mutex                        m_mutex;
    std::condition_variable           m_condition_variable;
    std::queue<std::function<void()>> m_task_queue;
    bool                              m_stop = false;

    auto thread_loop() -> void;

};

template<typename F, typename... Args>
auto SingleProducerAsyncTaskQueue::enqueue_task(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>
{
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>> (
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    auto result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_task_queue.push([task]() { (*task)(); });
    }

    m_condition_variable.notify_one();
    return result;
}

#ifdef ARRAY_IMPLEMENTATION

SingleProducerAsyncTaskQueue::SingleProducerAsyncTaskQueue()
{
    m_thread = std::thread([this] { thread_loop(); });
}

SingleProducerAsyncTaskQueue::~SingleProducerAsyncTaskQueue()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_condition_variable.notify_one();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

auto SingleProducerAsyncTaskQueue::queue_size() -> int64_t
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_task_queue.size();
}

auto SingleProducerAsyncTaskQueue::thread_loop() -> void
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition_variable.wait(lock, [this] { return m_stop || !m_task_queue.empty(); });

            if (m_stop && m_task_queue.empty())
            {
                break;
            }

            task = std::move(m_task_queue.front());
            m_task_queue.pop();
        }
        task();
    }
}

#endif

//******************************************************************************
// SingleProducerAsyncTaskQueuePool
//******************************************************************************

class SingleProducerAsyncTaskQueuePool
{

public:

    explicit SingleProducerAsyncTaskQueuePool(int64_t task_queue_count = 1);

    template<typename F, typename... Args>
    auto enqueue_task(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    auto queue_size(int64_t queue_index) -> int64_t;

private:

    std::vector<SingleProducerAsyncTaskQueue> m_task_queues;

};

template<typename F, typename... Args>
auto SingleProducerAsyncTaskQueuePool::enqueue_task(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
{
    int64_t min_queue_size = std::numeric_limits<int64_t>::max();
    int64_t min_queue_size_queue_index = std::numeric_limits<int64_t>::max();

    for(int64_t i = 0; i < m_task_queues.size(); i++)
    {
        if(m_task_queues[i].queue_size() < min_queue_size)
        {
            min_queue_size = m_task_queues[i].queue_size();
            min_queue_size_queue_index = i;
        }
    }

    return m_task_queues[min_queue_size_queue_index].enqueue_task (
        std::forward<F>(f),
        std::forward<Args>(args)...
    );
}

#ifdef ARRAY_IMPLEMENTATION

SingleProducerAsyncTaskQueuePool::SingleProducerAsyncTaskQueuePool(int64_t task_queue_count) : m_task_queues(task_queue_count)
{
    if ( task_queue_count <= 0 )
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }
}

auto SingleProducerAsyncTaskQueuePool::queue_size(int64_t queue_index) -> int64_t
{
    return m_task_queues[queue_index].queue_size();
}

#endif

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

constexpr int64_t dynamic_extent = std::numeric_limits<int64_t>::max();

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

//******************************************************************************
// ICloneable Interface
//******************************************************************************

#define DECLARE_CLONE(TYPE) \
    auto clone() const -> TYPE* override { return new TYPE(*this); }

class ICloneable
{

public:

    [[nodiscard]] virtual auto clone() const -> ICloneable* = 0;

    virtual ~ICloneable() = default;

};

//******************************************************************************
// ValuePtr
//******************************************************************************

// Concept for valid ValuePtr pointee types

template<typename T>
concept ValuePtrPointee =
    ( std::rank_v<T> == 0 && !std::is_reference_v<T> ) ||
    ( std::rank_v<T> == 1 &&
      std::is_unbounded_array_v<T> &&
      !std::is_reference_v<std::remove_extent_t<T>> );

// ValuePtr class template

// Base template

template<ValuePtrPointee T>
class ValuePtr
{

public:

    ValuePtr() = default;

    ValuePtr(const ValuePtr<T> &other);
    ValuePtr(ValuePtr<T> &&other) noexcept;

    auto operator=(const ValuePtr<T> &other) -> ValuePtr&;
    auto operator=(ValuePtr<T> &&other) noexcept -> ValuePtr&;

    ~ValuePtr();

    explicit ValuePtr(T *p);

    operator bool() const;

    auto get() const noexcept -> T*;
    auto release() -> T*;

    auto swap(ValuePtr<T> &other) noexcept -> void;
    auto reset(T *p = nullptr) -> void;

    auto operator*() const -> T&;
    auto operator->() const -> T*;

    template<typename... Args>
    static auto make(Args&&... args) -> ValuePtr<T>;

private:

    T *m_p = nullptr;

};

// Specialization for arrays

template<ValuePtrPointee T>
class ValuePtr<T[]>
{

public:

    ValuePtr() = default;

    ValuePtr(const ValuePtr<T[]> &other);
    ValuePtr(ValuePtr<T[]> &&other) noexcept;

    auto operator=(const ValuePtr<T[]> &other) -> ValuePtr&;
    auto operator=(ValuePtr<T[]> &&other) noexcept -> ValuePtr&;

    ~ValuePtr();

    explicit ValuePtr(T *p, int64_t n);

    operator bool() const;

    auto get() const noexcept -> T*;
    auto release() -> std::pair<T*, int64_t>;

    auto swap(ValuePtr<T[]> &other) noexcept -> void;
    auto reset(T *p = nullptr, int64_t n = 0) -> void;

    auto size() const -> int64_t;

    auto operator[](int64_t index) const -> T&;

    static auto make(int64_t n) -> ValuePtr<T[]>;

private:

    T *m_p = nullptr;
    int64_t m_n = 0;

};

// Base template

template<ValuePtrPointee T>
ValuePtr<T>::ValuePtr(const ValuePtr<T> &other)
{

    ValuePtr<T> copy;

    if constexpr (std::derived_from<T, ICloneable>)
    {
        copy.reset (
            other ? other->clone() : nullptr
        );
    }
    else
    {
        copy.reset (
            other ? new T(*other) : nullptr
        );
    }

    swap(copy);
}

template<ValuePtrPointee T>
ValuePtr<T>::ValuePtr(ValuePtr<T> &&other) noexcept
{
    swap(other);
}

template<ValuePtrPointee T>
auto ValuePtr<T>::operator=(const ValuePtr<T> &other) -> ValuePtr<T>&
{
    ValuePtr<T> copy(other);
    swap(copy);
    return *this;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::operator=(ValuePtr<T> &&other) noexcept -> ValuePtr<T>&
{
    swap(other);
    return *this;
}

template<ValuePtrPointee T>
ValuePtr<T>::~ValuePtr()
{
    reset();
}

template<ValuePtrPointee T>
ValuePtr<T>::ValuePtr(T *p)
: m_p(p)
{}

template<ValuePtrPointee T>
ValuePtr<T>::operator bool() const
{
    return m_p != nullptr;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::get() const noexcept -> T*
{
    return m_p;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::release() -> T*
{
    T *p = m_p;
    m_p = nullptr;
    return p;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::swap(ValuePtr<T> &other) noexcept -> void
{
    std::swap(m_p, other.m_p);
}

template<ValuePtrPointee T>
auto ValuePtr<T>::reset(T *p) -> void
{
    delete m_p;
    m_p = p;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::operator*() const -> T&
{
    return *m_p;
}

template<ValuePtrPointee T>
auto ValuePtr<T>::operator->() const -> T*
{
    return m_p;
}

template<ValuePtrPointee T>
template<typename... Args>
auto ValuePtr<T>::make(Args&&... args) -> ValuePtr<T>
{
    return ValuePtr<T>(new T(std::forward<Args>(args)...));
}

// Specialization for arrays

template<ValuePtrPointee T>
ValuePtr<T[]>::ValuePtr(const ValuePtr<T[]> &other)
{
    ValuePtr<T[]> copy;

    copy.reset (
        other ? new T[other.size()] : nullptr,
        other.size()
    );

    std::copy_n(other.get(), other.size(), copy.get());

    swap(copy);
}

template<ValuePtrPointee T>
ValuePtr<T[]>::ValuePtr(ValuePtr<T[]> &&other) noexcept
{
    swap(other);
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::operator=(const ValuePtr<T[]> &other) -> ValuePtr<T[]>&
{
    ValuePtr<T[]> copy(other);
    swap(copy);
    return *this;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::operator=(ValuePtr<T[]> &&other) noexcept -> ValuePtr<T[]>&
{
    swap(other);
    return *this;
}

template<ValuePtrPointee T>
ValuePtr<T[]>::~ValuePtr()
{
    reset();
}

template<ValuePtrPointee T>
ValuePtr<T[]>::ValuePtr(T *p, int64_t n)
: m_p(p), m_n(n)
{}

template<ValuePtrPointee T>
ValuePtr<T[]>::operator bool() const
{
    return m_p != nullptr;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::get() const noexcept -> T*
{
    return m_p;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::release() -> std::pair<T*, int64_t>
{
    auto p = std::make_pair(m_p, m_n);

    m_p = nullptr;
    m_n = 0;

    return p;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::swap(ValuePtr<T[]> &other) noexcept -> void
{
    std::swap(m_p, other.m_p);
    std::swap(m_n, other.m_n);
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::reset(T *p, int64_t n) -> void
{
    delete[] m_p;
    m_n = 0;

    m_p = p;
    m_n = n;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::size() const -> int64_t
{
    return m_n;
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::operator[](int64_t index) const -> T&
{
    return m_p[index];
}

template<ValuePtrPointee T>
auto ValuePtr<T[]>::make(int64_t n) -> ValuePtr<T[]>
{
    return ValuePtr<T[]>(new T[n], n);
}

//******************************************************************************
// Array layout classes
//******************************************************************************

// Affine layout class

template<int64_t D, Extents<D> AxisPermutation = make_extents_iota<D>(0)>
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

//******************************************************************************
// Iterator classes
//******************************************************************************

// IndexTupleIterator class

template<typename A, bool IsReadOnly>
class BasicIndexTupleIterator
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

    explicit BasicIndexTupleIterator() = default;

    explicit BasicIndexTupleIterator (
        APointer p_a,
        const Extents<A::dimension()> &cursor = make_extents_filled<A::dimension()>(0)
    );

    auto operator*() const -> ElementReference;

    auto operator->() const -> ElementPointer;

    auto operator++() -> BasicIndexTupleIterator&;
    auto operator++(int) -> BasicIndexTupleIterator;

    auto operator--() -> BasicIndexTupleIterator&;
    auto operator--(int) -> BasicIndexTupleIterator;

    auto cursor() const -> const Extents<A::dimension()>&;
    auto cursor(const Extents<A::dimension()> &cursor) -> BasicIndexTupleIterator&;

    auto is_at_end() const -> const bool&;
    auto is_at_end(const bool &flag) -> BasicIndexTupleIterator&;

    auto p_a() const -> APointer;

private:

    APointer                m_p_a       = nullptr;
    Extents<A::dimension()> m_cursor    = make_extents_filled<A::dimension()>(0);
    bool                    m_is_at_end = false;
};

template<typename AL, bool IsReadOnlyL, typename AR, bool IsReadOnlyR>
auto operator== (
    const BasicIndexTupleIterator<AL, IsReadOnlyL> &lhs,
    const BasicIndexTupleIterator<AR, IsReadOnlyR> &rhs
) -> bool
{
    return
        lhs.p_a()       == rhs.p_a()        &&
        lhs.cursor()    == rhs.cursor()     &&
        lhs.is_at_end() == rhs.is_at_end();
}

template<typename A, bool IsReadOnly>
BasicIndexTupleIterator<A, IsReadOnly>::BasicIndexTupleIterator(APointer p_a, const Extents<A::dimension()> &cursor)
: m_p_a(p_a),
  m_cursor(cursor),
  m_is_at_end(p_a->size() == 0)
{}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator*() const -> ElementReference
{
    return (*m_p_a)[m_cursor];
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator->() const -> ElementPointer
{
    return &(*m_p_a)[m_cursor];
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator++() -> BasicIndexTupleIterator&
{
    m_is_at_end = false;

    for (int64_t i = A::dimension() - 1; i >= 0; i--)
    {
        if (m_cursor[i] < m_p_a->extents(i) - 1)
        {
            m_cursor[i]++;
            return *this;
        }
        else
        {
            m_cursor[i] = 0;
        }
    }

    m_is_at_end = true;

    return *this;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator++(int) -> BasicIndexTupleIterator
{
    BasicIndexTupleIterator<A, IsReadOnly> r = *this;
    ++(*this);
    return r;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator--() -> BasicIndexTupleIterator&
{
    m_is_at_end = false;

    for (int64_t i = A::dimension() - 1; i >= 0; i--)
    {
        if (m_cursor[i] >= 1)
        {
            m_cursor[i]--;
            break;
        }
        else
        {
            m_cursor[i] = m_p_a->extents(i) - 1;
        }
    }

    return *this;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::operator--(int) -> BasicIndexTupleIterator
{
    BasicIndexTupleIterator<A, IsReadOnly> r = *this;
    --(*this);
    return r;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::cursor() const -> const Extents<A::dimension()>&
{
    return m_cursor;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::cursor(const Extents<A::dimension()> &cursor) -> BasicIndexTupleIterator&
{
    m_cursor = cursor;
    return *this;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::is_at_end() const -> const bool&
{
    return m_is_at_end;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::is_at_end(const bool &flag) -> BasicIndexTupleIterator&
{
    m_is_at_end = flag;
    return *this;
}

template<typename A, bool IsReadOnly>
auto BasicIndexTupleIterator<A, IsReadOnly>::p_a() const -> APointer
{
    return m_p_a;
}

template<typename A>
using IndexTupleIterator = BasicIndexTupleIterator<A, false>;

template<typename A>
using ReadOnlyIndexTupleIterator = BasicIndexTupleIterator<A, true>;

template<typename A>
BasicIndexTupleIterator(A&) -> BasicIndexTupleIterator<A, false>;

template<typename A>
BasicIndexTupleIterator(const A&) -> BasicIndexTupleIterator<A, true>;

//******************************************************************************
// View classes
//******************************************************************************

/**
 * @brief `BasicSliceView` class template.
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
class BasicSliceView
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

    using Owning =
        std::conditional_t <
            A::is_owning_type(),
            A,
            typename A::Owning
        >;

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    BasicSliceView() = default;

    explicit BasicSliceView (
        AReference array,
        const Extents<A::dimension()> &origin  = make_extents_filled<A::dimension()>(0),
        const Extents<D>              &extents = make_extents_filled<D>(dynamic_extent),
        const Extents<D>              &strides = make_extents_filled<D>(1)
    );

    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementReference;

    auto operator[](const Extents<D> &indices) const -> ElementReference;

    static consteval auto dimension()            -> int64_t;
    static consteval auto is_owning_type()       -> bool;
    static consteval auto is_of_static_extents() -> bool;
    static consteval auto type_extents()         -> Extents<D>;

    auto origin() const -> const Extents<A::dimension()>&;
    auto origin(const Extents<A::dimension()> &origin) -> BasicSliceView&;

    auto origin(const int64_t &i) const -> const int64_t&;
    auto origin(const int64_t &i, const int64_t &v) -> BasicSliceView&;

    auto extents() const -> const Extents<D>&;
    auto extents(const Extents<D> &extents) -> BasicSliceView&;

    auto extents(const int64_t &i) const -> const int64_t&;
    auto extents(const int64_t &i, const int64_t &v) -> BasicSliceView&;

    auto strides() const -> const Extents<D>&;
    auto strides(const Extents<D> &strides) -> BasicSliceView&;

    auto strides(const int64_t &i) const -> const int64_t&;
    auto strides(const int64_t &i, const int64_t &v) -> BasicSliceView&;

    auto size() const -> int64_t;

    auto map(const Extents<D> &indices) const -> Extents<A::dimension()>;

    auto begin() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicSliceView>,
            IndexTupleIterator<BasicSliceView>
        >;

    auto cbegin() const -> ReadOnlyIndexTupleIterator<BasicSliceView>;

    auto end() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicSliceView>,
            IndexTupleIterator<BasicSliceView>
        >;

    auto cend() const -> ReadOnlyIndexTupleIterator<BasicSliceView>;

private:

    APointer m_p_array = nullptr;

    Extents<A::dimension()> m_origin  = {};
    Extents<D>              m_extents = {};
    Extents<D>              m_strides = {};

};

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::BasicSliceView (
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
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::operator[](I... i) const -> ElementReference
{
    return operator[]({int64_t(i)...});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::operator[](const Extents<D> &indices) const -> ElementReference
{
    return (*m_p_array)[map(indices)];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::dimension() -> int64_t
{
    return D;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::is_owning_type() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::is_of_static_extents() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
consteval auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::type_extents() -> Extents<D>
{
    return make_extents_filled<D>(dynamic_extent);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::origin() const -> const Extents<A::dimension()>&
{
    return m_origin;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::origin(const Extents<A::dimension()> &origin) -> BasicSliceView&
{
    m_origin = origin;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::origin(const int64_t &i) const -> const int64_t&
{
    return m_origin[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::origin(const int64_t &i, const int64_t &v) -> BasicSliceView&
{
    m_origin[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::extents() const -> const Extents<D>&
{
    return m_extents;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::extents(const Extents<D> &extents) -> BasicSliceView&
{
    m_extents = extents;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::extents(const int64_t &i) const -> const int64_t&
{
    return m_extents[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::extents(const int64_t &i, const int64_t &v) -> BasicSliceView&
{
    m_extents[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::strides() const -> const Extents<D>&
{
    return m_strides;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::strides(const Extents<D> &strides) -> BasicSliceView&
{
    m_strides = strides;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::strides(const int64_t &i) const -> const int64_t&
{
    return m_strides[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::strides(const int64_t &i, const int64_t &v) -> BasicSliceView&
{
    m_strides[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::size() const -> int64_t
{
    return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::map(const Extents<D> &view_indices) const -> Extents<A::dimension()>
{
    Extents<A::dimension()> array_indices = m_origin;

    for (int64_t k = 0; k < D; k++)
    {
        array_indices[ViewIndexSubspace[k]] += view_indices[k] * m_strides[k];
    }

    return array_indices;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::begin() const ->
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicSliceView>,
        IndexTupleIterator<BasicSliceView>
    >
{
    return
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicSliceView>,
            IndexTupleIterator<BasicSliceView>
        >(this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::cbegin() const -> ReadOnlyIndexTupleIterator<BasicSliceView>
{
    return ReadOnlyIndexTupleIterator<BasicSliceView>(this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::end() const ->
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicSliceView>,
        IndexTupleIterator<BasicSliceView>
    >
{
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicSliceView>,
        IndexTupleIterator<BasicSliceView>
    > it(this);

    it.is_at_end(true);
    return it;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
auto BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>::cend() const -> ReadOnlyIndexTupleIterator<BasicSliceView>
{
    ReadOnlyIndexTupleIterator<BasicSliceView> it(this);
    it.is_at_end(true);
    return it;
}

template <
    typename A,
    int64_t D = A::dimension(),
    Extents<D> ViewIndexSubspace = make_extents_iota<D>(0)
>
using SliceView = BasicSliceView<A, false, D, ViewIndexSubspace>;

template <
    typename A,
    int64_t D = A::dimension(),
    Extents<D> ViewIndexSubspace = make_extents_iota<D>(0)
>
using ReadOnlySliceView = BasicSliceView<A, true, D, ViewIndexSubspace>;

template<int64_t D, Extents<D> ViewIndexSubspace, typename A>
auto make_slice_view (
    A& array,
    const Extents<A::dimension()> &origin  = make_extents_filled<A::dimension()>(0),
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent),
    const Extents<D> &strides = make_extents_filled<D>(1)
)
{
    return SliceView<A, D, ViewIndexSubspace>(array, origin, extents, strides);
}

template<int64_t D, Extents<D> ViewIndexSubspace, typename A>
auto make_read_only_slice_view (
    A& array,
    const Extents<A::dimension()> &origin  = make_extents_filled<A::dimension()>(0),
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent),
    const Extents<D> &strides = make_extents_filled<D>(1)
)
{
    return ReadOnlySliceView<A, D, ViewIndexSubspace>(array, origin, extents, strides);
}

/**
 * @brief `BasicBroadcastView` class template.
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
class BasicBroadcastView
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

    using Owning =
        std::conditional_t <
            A::is_owning_type(),
            A,
            typename A::Owning
        >;

    using ElementReference = std::conditional_t<IsReadOnly, const Element&, Element&>;
    using ElementPointer   = std::conditional_t<IsReadOnly, const Element*, Element*>;

    BasicBroadcastView() = default;

    explicit BasicBroadcastView (
        AReference array,
        const Extents<D> &extents
    );

    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) auto operator[](I... i) const -> ElementReference;

    auto operator[](const Extents<D> &indices) const -> ElementReference;

    static consteval auto dimension()            -> int64_t;
    static consteval auto is_owning_type()       -> bool;
    static consteval auto is_of_static_extents() -> bool;
    static consteval auto type_extents()         -> Extents<D>;

    auto extents() const -> const Extents<D>&;
    auto extents(const Extents<D> &extents) -> BasicBroadcastView&;

    auto extents(const int64_t &i) const -> const int64_t&;
    auto extents(const int64_t &i, const int64_t &v) -> BasicBroadcastView&;

    auto size() const -> int64_t;

    auto map(const Extents<D> &indices) const -> Extents<A::dimension()>;

    auto begin() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicBroadcastView>,
            IndexTupleIterator<BasicBroadcastView>
        >;

    auto cbegin() const -> ReadOnlyIndexTupleIterator<BasicBroadcastView>;

    auto end() const ->
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicBroadcastView>,
            IndexTupleIterator<BasicBroadcastView>
        >;

    auto cend() const -> ReadOnlyIndexTupleIterator<BasicBroadcastView>;

private:

    APointer m_p_array = nullptr;

    Extents<D> m_extents = {};
};

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::BasicBroadcastView (AReference array, const Extents<D> &extents)
: m_p_array ( &array ), m_extents(extents)
{}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...))
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::operator[](I... i) const -> ElementReference
{
    return operator[]({int64_t(i)...});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::operator[](const Extents<D> &indices) const -> ElementReference
{
    return (*m_p_array)[map(indices)];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::dimension() -> int64_t
{
    return D;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::is_owning_type() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::is_of_static_extents() -> bool
{
    return false;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
consteval auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::type_extents() -> Extents<D>
{
    return make_extents_filled<D>(dynamic_extent);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::extents() const -> const Extents<D>&
{
    return m_extents;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::extents(const Extents<D> &extents) -> BasicBroadcastView&
{
    m_extents = extents;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::extents(const int64_t &i) const -> const int64_t&
{
    return m_extents[i];
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::extents(const int64_t &i, const int64_t &v) -> BasicBroadcastView&
{
    m_extents[i] = v;
    return *this;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::size() const -> int64_t
{
    return std::reduce(m_extents.begin(), m_extents.end(), int64_t(1), std::multiplies{});
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::map(const Extents<D> &view_indices) const -> Extents<A::dimension()>
{
    Extents<A::dimension()> array_indices = {};

    for (int64_t i = 0; i < A::dimension(); i++)
    {
        array_indices[i] = view_indices[AIndexSubspace[i]] % m_p_array->extents(i);
    }

    return array_indices;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::begin() const ->
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicBroadcastView>,
        IndexTupleIterator<BasicBroadcastView>
    >
{
    return
        std::conditional_t <
            IsReadOnly,
            ReadOnlyIndexTupleIterator<BasicBroadcastView>,
            IndexTupleIterator<BasicBroadcastView>
        >(*this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::cbegin() const -> ReadOnlyIndexTupleIterator<BasicBroadcastView>
{
    return ReadOnlyIndexTupleIterator<BasicBroadcastView>(*this);
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::end() const ->
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicBroadcastView>,
        IndexTupleIterator<BasicBroadcastView>
    >
{
    std::conditional_t <
        IsReadOnly,
        ReadOnlyIndexTupleIterator<BasicBroadcastView>,
        IndexTupleIterator<BasicBroadcastView>
    > it(*this);

    it.carry(true);
    return it;
}

template<typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
auto BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>::cend() const -> ReadOnlyIndexTupleIterator<BasicBroadcastView>
{
    ReadOnlyIndexTupleIterator<BasicBroadcastView> it(*this);
    it.carry(true);
    return it;
}

template <
    typename A,
    int64_t D = A::dimension(),
    Extents<A::dimension()> AIndexSubspace = make_extents_iota<A::dimension()>(0)
>
using BroadcastView = BasicBroadcastView<A, false, D, AIndexSubspace>;

template <
    typename A,
    int64_t D = A::dimension(),
    Extents<A::dimension()> AIndexSubspace = make_extents_iota<A::dimension()>(0)
>
using ReadOnlyBroadcastView = BasicBroadcastView<A, true, D, AIndexSubspace>;

template<int64_t D, Extents<D> AIndexSubspace, typename A>
auto make_broadcast_view (
    A& array,
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent)
)
{
    return BroadcastView<A, D, AIndexSubspace>(array, extents);
}

template<int64_t D, Extents<D> AIndexSubspace, typename A>
auto make_read_only_broadcast_view (
    A& array,
    const Extents<D> &extents = make_extents_filled<D>(dynamic_extent)
)
{
    return ReadOnlyBroadcastView<A, D, AIndexSubspace>(array, extents);
}

//******************************************************************************
// Array class
//******************************************************************************

/**
 * @brief Primary class template declaration for `Array`.
 *
 * @tparam T  Type of the array's elements.
 * @tparam D  Number of dimensions.
 * @tparam E  The extents of the array, of type `Extents<D>`. Should be either filled with values smaller than `dynamic_extent` for static arrays,
 *            or with the value `dynamic_extent` for dynamic arrays. Filled with the `dynamic_extent` sentinel value by default.
 * @tparam L  A type implementing the actual read and write operations, determining the memory layout.
 *            Defaults to `Affine<D>`.
 */

template <
    typename   T,
    int64_t    D,
    Extents<D> E = make_extents_filled<D>(dynamic_extent),
    typename   L = Affine<D>
>
class Array;

// Helper functions to copy from initializer lists, for D = 1, D = 2, D = 3

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr void array_copy_from_initializer_list(Array<T, 1, E, L> &a, std::initializer_list<T> l)
{
    if (!(l.size() == a.extents(0)))
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }
    int64_t i = 0;
    for (const auto &e : l)
    {
        a[i] = e;
        i++;
    }
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr void array_copy_from_initializer_list(Array<T, 2, E, L> &a, std::initializer_list<std::initializer_list<T>> ll)
{
    if (!(ll.size() == a.extents(0)))
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }
    int64_t i = 0;
    for (const auto &l : ll)
    {
        if (!(l.size() == a.extents(1)))
        {
            throw_with_context<std::domain_error>("Domain error. Check source location.");
        }
        int64_t j = 0;
        for (const auto &e : l)
        {
            a[i, j] = e;
            j++;
        }
        i++;
    }
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr void array_copy_from_initializer_list(Array<T, 3, E, L> &a, std::initializer_list<std::initializer_list<std::initializer_list<T>>> lll)
{
    if (!(lll.size() == a.extents(0)))
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }
    int64_t i = 0;
    for (const auto &ll : lll)
    {
        if (!(ll.size() == a.extents(1)))
        {
            throw_with_context<std::domain_error>("Domain error. Check source location.");
        }
        int64_t j = 0;
        for (const auto &l : ll)
        {
            if (!(l.size() == a.extents(2)))
            {
                throw_with_context<std::domain_error>("Domain error. Check source location.");
            }
            int64_t k = 0;
            for (const auto &e : l)
            {
                a[i, j, k] = e;
                k++;
            }
            j++;
        }
        i++;
    }
}

template <typename T, int64_t D, Extents<D> E, typename L, bool S>
struct ArrayMembers;

template <typename T, int64_t D, Extents<D> E, typename L>
struct ArrayMembers<T, D, E, L, false>
{
    using Layout = L;

    Layout layout;
    ValuePtr<T[]> elements;
};

template <typename T, int64_t D, Extents<D> E, typename L>
struct ArrayMembers<T, D, E, L, true>
{
    using Layout = L;

    static constexpr Layout layout = Layout(E);
    std::array<T, layout.size_allocated()> elements;
};

template<typename T, int64_t D, Extents<D> E, typename L>
class Array
{

    static_assert(D > 0);
    static_assert(extents_valid(E));

public:

    using Element = T;
    using Layout = L;

    using Owning = Array;

    Array();

    explicit Array(const Extents<D> &extents) requires (all_of_extents_dynamic(E));

    template<std::integral... ExtentsPack>
    requires (all_of_extents_dynamic(E))
    Array(ExtentsPack... extents_pack);

    constexpr Array (
        std::initializer_list<T> l
    ) requires (D == 1);

    constexpr Array (
        std::initializer_list <
            std::initializer_list<T>
        > ll
    ) requires (D == 2);

    constexpr Array (
        std::initializer_list <
            std::initializer_list <
                std::initializer_list<T>
            >
        > lll
    ) requires (D == 3);

    template<typename A>
    requires
        requires { { A::is_owning_type() } -> std::convertible_to<bool>; } &&
        ( A::is_owning_type() == false )
    Array (const A &a);

    template<typename T1>
    Array(const Array<T1, D, E, L> &other);

    template<typename T1>
    auto operator=(const Array<T1, D, E, L> &other) -> Array&;

    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) constexpr auto operator[](I... i) const -> const T&;
    template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...)) constexpr auto operator[](I... i) -> T&;

    constexpr auto operator[](const Extents<D> &indices) const -> const T&;
    constexpr auto operator[](const Extents<D> &indices) -> T&;

    static consteval auto dimension() -> int64_t;

    static consteval auto is_owning_type() -> bool;

    static consteval auto is_of_static_extents() -> bool;

    static consteval auto type_extents() -> Extents<D>;

    constexpr auto extents() const -> const Extents<D>&;
    constexpr auto extents(const int64_t &i) const -> const int64_t&;

    constexpr auto layout() const -> const Layout&;

    constexpr auto size() const -> int64_t;
    constexpr auto size_stored() -> int64_t;
    constexpr auto size_allocated() const -> int64_t;

    constexpr auto p_elements() const -> const T*;
    constexpr auto p_elements() -> T*;

    constexpr auto begin() -> IndexTupleIterator<Array>;
    constexpr auto begin() const -> ReadOnlyIndexTupleIterator<Array>;
    constexpr auto cbegin() const -> ReadOnlyIndexTupleIterator<Array>;

    constexpr auto end() -> IndexTupleIterator<Array>;
    constexpr auto end() const -> ReadOnlyIndexTupleIterator<Array>;
    constexpr auto cend() const -> ReadOnlyIndexTupleIterator<Array>;

    template<typename... NewExtents> requires ((sizeof...(NewExtents) == D) && (std::is_integral_v<NewExtents> && ...))
    auto resize(NewExtents... extents) -> void requires (all_of_extents_dynamic(E));

    auto resize(const Extents<D> &extents) -> void requires (all_of_extents_dynamic(E));

private:

    ArrayMembers<T, D, E, L, all_of_extents_static(E)> m;

};

template<typename T, int64_t D, Extents<D> E, typename L>
Array<T, D, E, L>::Array()
{
    if constexpr (all_of_extents_dynamic(E))
    {
        m.layout   = Layout(make_extents_filled<D>(0));
        m.elements = ValuePtr<T[]>(nullptr, 0);
    }
}

template<typename T, int64_t D, Extents<D> E, typename L>
Array<T, D, E, L>::Array(const Extents<D> &extents) requires (all_of_extents_dynamic(E))
: m {
    .layout = Layout(extents)
}
{
    if (!(extents_valid<D>(extents)))
    {
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }

    m.elements = ValuePtr<T[]>::make(m.layout.size_allocated());
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<std::integral... ExtentsPack>
requires (all_of_extents_dynamic(E))
Array<T, D, E, L>::Array(ExtentsPack... extents_pack)
: m {
    .layout = Layout({int64_t(extents_pack)...})
}
{
    static_assert(sizeof...(extents_pack) == D);

    if (!(extents_valid<D>({int64_t(extents_pack)...})))
    {
        m.layout = Layout(make_extents_filled<D>(0));
        throw_with_context<std::domain_error>("Domain error. Check source location.");
    }

    m.elements = ValuePtr<T[]>::make(m.layout.size_allocated());
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr Array<T, D, E, L>::Array (
    std::initializer_list<T> l
) requires (D == 1)
{
    if constexpr (all_of_extents_dynamic(E))
    {
        Extents<D> extents = {
            int64_t(l.size())
        };
        m.layout   = Layout(extents);
        m.elements = ValuePtr<T[]>::make(m.layout.size_allocated());
    }
    array_copy_from_initializer_list(*this, l);
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr Array<T, D, E, L>::Array (
    std::initializer_list <
        std::initializer_list<T>
    > ll
) requires (D == 2)
{
    if constexpr (all_of_extents_dynamic(E))
    {
        Extents<D> extents = {
            int64_t(ll.size()),
            int64_t(ll.begin()->size())
        };
        m.layout   = Layout(extents);
        m.elements = ValuePtr<T[]>::make(m.layout.size_allocated());
    }
    array_copy_from_initializer_list(*this, ll);
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr Array<T, D, E, L>::Array (
    std::initializer_list <
        std::initializer_list <
            std::initializer_list<T>
        >
    > lll
) requires (D == 3)
{
    if constexpr (all_of_extents_dynamic(E))
    {
        Extents<D> extents = {
            int64_t(lll.size()),
            int64_t(lll.begin()->size()),
            int64_t(lll.begin()->begin()->size())
        };
        m.layout   = Layout(extents);
        m.elements = ValuePtr<T[]>::make(m.layout.size_allocated());
    }
    array_copy_from_initializer_list(*this, lll);
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename A>
requires
    requires
    { { A::is_owning_type() } -> std::convertible_to<bool>; } &&
    ( A::is_owning_type() == false )
Array<T, D, E, L>::Array(const A &a) : Array(a.extents())
{
    ReadOnlyIndexTupleIterator view_it(a);
    IndexTupleIterator it(*this);

    for (int64_t i = 0; i < size(); i++)
    {
        *it++ = *view_it++;
    }
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename T1>
Array<T, D, E, L>::Array(const Array<T1, D, E, L> &other)
{
    if constexpr (all_of_extents_dynamic(E))
    {
        m.layout   = other.layout();
        m.elements = ValuePtr<T[]>::make(other.size_allocated());
    }
    std::copy_n(other.p_elements(), other.size(), p_elements());
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename T1>
auto Array<T, D, E, L>::operator=(const Array<T1, D, E, L> &other) -> Array&
{
    if constexpr (std::is_same_v<T, T1>)
    {
        if(this == &other)
        {
            return *this;
        }
    }

    if constexpr (all_of_extents_dynamic(E))
    {
        m.layout = other.layout();
        m.elements = ValuePtr<T[]>::make(other.size_allocated());
    }
    std::copy_n(other.p_elements(), other.size(), p_elements());

    return *this;
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...))
constexpr auto Array<T, D, E, L>::operator[](I... i) const -> const T&
{
    return m.elements[m.layout.offset({int64_t(i)...})];
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename... I> requires ((sizeof...(I) == D) && (std::is_integral_v<I> && ...))
constexpr auto Array<T, D, E, L>::operator[](I... i) -> T&
{
    return m.elements[m.layout.offset({int64_t(i)...})];
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::operator[](const Extents<D> &indices) const -> const T&
{
    return m.elements[m.layout.offset(indices)];
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::operator[](const Extents<D> &indices) -> T&
{
    return m.elements[m.layout.offset(indices)];
}

template<typename T, int64_t D, Extents<D> E, typename L>
consteval auto Array<T, D, E, L>::dimension() -> int64_t
{
    return D;
}

template<typename T, int64_t D, Extents<D> E, typename L>
consteval auto Array<T, D, E, L>::is_owning_type() -> bool
{
    return true;
}

template<typename T, int64_t D, Extents<D> E, typename L>
consteval auto Array<T, D, E, L>::is_of_static_extents() -> bool
{
    return all_of_extents_static(E);
}

template<typename T, int64_t D, Extents<D> E, typename L>
consteval auto Array<T, D, E, L>::type_extents() -> Extents<D>
{
    return E;
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::extents() const -> const Extents<D>&
{
    return m.layout.extents();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::extents(const int64_t &i) const -> const int64_t&
{
    return m.layout.extents()[i];
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::layout() const -> const Layout&
{
    return m.layout;
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::size() const -> int64_t
{
    return m.layout.size();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::size_allocated() const -> int64_t
{
    return m.layout.size_allocated();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::p_elements() const -> const T*
{
    if constexpr (all_of_extents_dynamic(E))
        return m.elements.get();
    else
        return m.elements.data();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::p_elements() -> T*
{
    if constexpr (all_of_extents_dynamic(E))
        return m.elements.get();
    else
        return m.elements.data();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::begin() -> IndexTupleIterator<Array>
{
    return IndexTupleIterator<Array>(this);
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::begin() const -> ReadOnlyIndexTupleIterator<Array>
{
    return ReadOnlyIndexTupleIterator<Array>(this);
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::cbegin() const -> ReadOnlyIndexTupleIterator<Array>
{
    return begin();
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::end() -> IndexTupleIterator<Array>
{
    IndexTupleIterator<Array> it(this);
    it.is_at_end(true);
    return it;
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::end() const -> ReadOnlyIndexTupleIterator<Array>
{
    ReadOnlyIndexTupleIterator<Array> it(this);
    it.is_at_end(true);
    return it;
}

template<typename T, int64_t D, Extents<D> E, typename L>
constexpr auto Array<T, D, E, L>::cend() const -> ReadOnlyIndexTupleIterator<Array>
{
    return end();
}

template<typename T, int64_t D, Extents<D> E, typename L>
template<typename... NewExtents> requires ((sizeof...(NewExtents) == D) && (std::is_integral_v<NewExtents> && ...))
auto Array<T, D, E, L>::resize(NewExtents... extents) -> void requires (all_of_extents_dynamic(E))
{
    resize({int64_t(extents)...});
}

template<typename T, int64_t D, Extents<D> E, typename L>
auto Array<T, D, E, L>::resize(const Extents<D> &extents) -> void requires (all_of_extents_dynamic(E))
{
    auto intersection_view = make_read_only_slice_view<D, make_extents_iota<D>(0)> (
        *this,
        make_extents_filled<D>(0),
        extents_intersection<D>(this->extents(), extents)
    );

    Array resized = Array(extents);

    for (auto it = intersection_view.cbegin(); it != intersection_view.cend(); it++)
    {
        resized[it.cursor()] = *it;
    }

    std::swap(*this, resized);
}

//******************************************************************************
// std::formatter specializations
//******************************************************************************

// Specialization for BasicSliceView

template <typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
class std::formatter<BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>>
{

public:

    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace> &v, FormatContext &ctx) const
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

// Specialization for BasicBroadcastView

template <typename A, bool IsReadOnly, int64_t D, Extents<A::dimension()> AIndexSubspace>
class std::formatter<BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>>
{

public:

    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace> &v, FormatContext &ctx) const
    {
        auto out = ctx.out();
        auto view = ReadOnlySliceView<BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>>(v);
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
        auto view = ReadOnlySliceView<Array<T, D, E, L>>(a);
        out = std::format_to(out, "{}", view);
        return out;
    }
};

//******************************************************************************
// Concepts for scalar numerical types
//******************************************************************************

template<typename T, typename U>
concept SameAsUpToCVRef = std::same_as <
    std::remove_cvref_t<T>,
    std::remove_cvref_t<U>
>;

template<typename T>
concept ScalarRealSignedIntegerNumType =
    SameAsUpToCVRef<T, int8_t> ||
    SameAsUpToCVRef<T, int16_t> ||
    SameAsUpToCVRef<T, int32_t> ||
    SameAsUpToCVRef<T, int64_t>;

template<typename T>
concept ScalarRealUnsignedIntegerNumType =
    SameAsUpToCVRef<T, uint8_t> ||
    SameAsUpToCVRef<T, uint16_t> ||
    SameAsUpToCVRef<T, uint32_t> ||
    SameAsUpToCVRef<T, uint64_t>;

template<typename T>
concept ScalarRealIntegerNumType =
    ScalarRealSignedIntegerNumType<T> || ScalarRealUnsignedIntegerNumType<T>;

template<typename T>
concept ScalarRealFloatingPointNumType =
    SameAsUpToCVRef<T, float> ||
    SameAsUpToCVRef<T, double>;

template<typename T>
concept ScalarComplexFloatingPointNumType =
    requires {
        typename T::value_type;
    } &&
    ScalarRealFloatingPointNumType<typename T::value_type> &&
    SameAsUpToCVRef<T, std::complex<typename T::value_type>>;

template<typename T>
concept ScalarComplexNumType =
    ScalarComplexFloatingPointNumType<T>;

template<typename T>
concept ScalarNumType =
    ScalarRealIntegerNumType<T> ||
    ScalarRealFloatingPointNumType<T> ||
    ScalarComplexNumType<T>;

//******************************************************************************
// Traits and concepts for array and view types
//******************************************************************************


// Arrays

template <typename T>
struct IsArrayType : std::false_type {};

template <typename T, int64_t D, Extents<D> E, typename L>
struct IsArrayType<Array<T, D, E, L>> : std::true_type {};

template <typename T>
concept ArrayType = IsArrayType<std::remove_cvref_t<T>>::value;


// Views

template <typename T>
struct IsSliceViewType : std::false_type {};

template <typename A, bool IsReadOnly, int64_t D, Extents<D> ViewIndexSubspace>
struct IsSliceViewType<BasicSliceView<A, IsReadOnly, D, ViewIndexSubspace>> : std::true_type {};

template <typename T>
concept SliceViewType = IsSliceViewType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsBroadcastViewType : std::false_type {};

template <typename A, bool IsReadOnly, int64_t D, Extents<D> AIndexSubspace>
struct IsBroadcastViewType<BasicBroadcastView<A, IsReadOnly, D, AIndexSubspace>> : std::true_type {};

template <typename T>
concept BroadcastViewType = IsBroadcastViewType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsViewType :
    std::bool_constant <
        IsSliceViewType<T>::value     ||
        IsBroadcastViewType<T>::value
    > {};

template <typename T>
concept ViewType = IsViewType<std::remove_cvref_t<T>>::value;


// Iterators

template <typename T>
struct IsIndexTupleIteratorType : std::false_type {};

template <typename A, bool IsReadOnly>
struct IsIndexTupleIteratorType<BasicIndexTupleIterator<A, IsReadOnly>> : std::true_type {};

template <typename T>
concept IndexTupleIteratorType = IsIndexTupleIteratorType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsIteratorType : std::bool_constant <IsIndexTupleIteratorType<T>::value> {};

template <typename T>
concept IteratorType = IsIteratorType<std::remove_cvref_t<T>>::value;


// Layouts

template <typename T>
struct IsAffineLayoutType : std::false_type {};

template <int64_t D, Extents<D> AxisPermutation>
struct IsAffineLayoutType<Affine<D, AxisPermutation>> : std::true_type {};

template <typename T>
concept AffineLayoutType = IsAffineLayoutType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsBlockedLayoutType : std::false_type {};

template <int64_t D, Extents<D> AxisPermutation, Extents<D> BlockExtents>
struct IsBlockedLayoutType<Blocked<D, AxisPermutation, BlockExtents>> : std::true_type {};

template <typename T>
concept BlockedLayoutType = IsBlockedLayoutType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsMortonLayoutType : std::false_type {};

template <int64_t D>
struct IsMortonLayoutType<Morton<D>> : std::true_type {};

template <typename T>
concept MortonLayoutType = IsMortonLayoutType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsLayoutType :
    std::bool_constant <
        IsAffineLayoutType<T>::value  ||
        IsBlockedLayoutType<T>::value ||
        IsMortonLayoutType<T>::value
    > {};

template <typename T>
concept LayoutType = IsLayoutType<std::remove_cvref_t<T>>::value;

//******************************************************************************
// Compute result types
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

template<typename Op, typename A>
requires (
    ( ArrayType<A> || ViewType<A> )
)
struct UnaryOpResultType
{
    using OpIn  = typename A::Element;
    using OpOut = std::invoke_result_t<Op, OpIn>;

    using Type = Array <
        OpOut,
        A::dimension(),
        A::type_extents(),
        typename LayoutOf<A>::Type
    >;
};

template<typename Op, typename L, typename R>
requires (
    ( ArrayType<L> || ViewType<L> ) &&
    ( ArrayType<R> || ViewType<R> ) &&
    ( L::dimension() == R::dimension() ) &&
    (
        !L::is_of_static_extents() ||
        !R::is_of_static_extents() ||
        ( L::type_extents() == R::type_extents() )
    ) &&
    (
        ViewType<L> ||
        ViewType<R> ||
        std::same_as<typename L::Layout, typename R::Layout>
    )
)
struct BinaryOpResultType
{
    using Element = decltype (
        Op {} (
            std::declval<typename L::Element>(),
            std::declval<typename R::Element>()
        )
    );

    static consteval auto dimension() -> int64_t {
        return L::dimension();
    }

    using Layout =
        std::conditional_t <
            ViewType<L>,
            typename LayoutOf<R>::Type,
            typename LayoutOf<L>::Type
        >;

    static consteval auto is_of_static_extents() -> bool {
        return
            L::is_of_static_extents() &&
            R::is_of_static_extents();
    }

    static consteval auto type_extents() -> Extents<dimension()> {
        if constexpr (is_of_static_extents())
        {
            return L::type_extents();
        }
        else
        {
            return make_extents_filled<dimension()>(dynamic_extent);
        }
    }

    using Type = Array <
        Element,
        dimension(),
        type_extents(),
        Layout
    >;
};

template<typename Op, typename L, typename R>
requires (
    ( ArrayType<L> || ViewType<L> ) && ScalarNumType<R>
)
struct BinaryOpRScalarResultType
{
    using Element = decltype (
        Op {} ( std::declval<typename L::Element>(), std::declval<R>() )
    );

    static consteval auto dimension() -> int64_t {
        return L::dimension();
    }

    static consteval auto is_of_static_extents() -> bool {
        return L::is_of_static_extents();
    }

    static consteval auto type_extents() -> Extents<dimension()> {
        if constexpr (is_of_static_extents())
        {
            return L::type_extents();
        }
        else
        {
            return make_extents_filled<dimension()>(dynamic_extent);
        }
    }

    using Type = Array <
        Element,
        dimension(),
        type_extents(),
        typename LayoutOf<L>::Type
    >;
};

template<typename Op, typename L, typename R>
requires (
    ( ArrayType<R> || ViewType<R> ) && ScalarNumType<L>
)
struct BinaryOpLScalarResultType
{
    using Element = decltype (
        Op {} ( std::declval<typename R::Element>(), std::declval<L>() )
    );

    static consteval auto dimension() -> int64_t {
        return R::dimension();
    }

    static consteval auto is_of_static_extents() -> bool {
        return R::is_of_static_extents();
    }

    static consteval auto type_extents() -> Extents<dimension()> {
        if constexpr (is_of_static_extents())
        {
            return R::type_extents();
        }
        else
        {
            return make_extents_filled<dimension()>(dynamic_extent);
        }
    }

    using Type = Array <
        Element,
        dimension(),
        type_extents(),
        typename LayoutOf<R>::Type
    >;
};

template<typename Op, typename A>
requires ( ArrayType<A> || ViewType<A> )
auto unary_op(const Op &op, const A &a)
    -> UnaryOpResultType<Op, A>::Type
{
    typename UnaryOpResultType<Op, A>::Type result;

    if constexpr ( !result.is_of_static_extents() )
    {
        result.resize(a.extents());
    }

    std::transform (
        a.cbegin(), a.cend(),
        result.begin(),
        op
    );

    return result;
}

template <typename Op, typename L, typename R>
requires ((ArrayType<L> || ViewType<L>) && (ArrayType<R> || ViewType<R>))
auto binary_op(const Op &op, const L &lhs, const R &rhs)
    -> BinaryOpResultType<Op, L, R>::Type
{
    typename BinaryOpResultType<Op, L, R>::Type result;

    if constexpr ( !result.is_of_static_extents() )
    {
        if (lhs.extents() != rhs.extents())
        {
            throw_with_context<std::domain_error>("Domain error. Check source location.");
        }

        result.resize(lhs.extents());
    }

    std::transform (
        lhs.cbegin(), lhs.cend(),
        rhs.cbegin(),
        result.begin(),
        op
    );

    return result;
}

template <typename Op, typename L, typename R>
requires ((ArrayType<L> || ViewType<L> ) && ScalarNumType<R>)
auto binary_op_r_scalar(const Op &op, const L &lhs, const R &rhs)
    -> BinaryOpRScalarResultType<Op, L, R>::Type
{
    typename BinaryOpRScalarResultType<Op, L, R>::Type result;

    if constexpr ( !result.is_of_static_extents() )
    {
        result.resize(lhs.extents());
    }

    auto lhs_it = lhs.cbegin();
    auto result_it = result.begin();

    while ( lhs_it != lhs.cend() )
    {
        *result_it = op(*lhs_it, rhs);
        lhs_it++; result_it++;
    }

    return result;
}

template <typename Op, typename L, typename R>
requires ((ArrayType<R> || ViewType<R> ) && ScalarNumType<L>)
auto binary_op_l_scalar(const Op &op, const L &lhs, const R &rhs)
    -> BinaryOpLScalarResultType<Op, L, R>::Type
{
    typename BinaryOpLScalarResultType<Op, L, R>::Type result;

    if constexpr ( !result.is_of_static_extents() )
    {
        result.resize(rhs.extents());
    }

    auto rhs_it = rhs.cbegin();
    auto result_it = result.begin();

    while ( rhs_it != rhs.cend() )
    {
        *result_it = op(*rhs_it, lhs);
        rhs_it++; result_it++;
    }

    return result;
}

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

// (Array | View ) op (Array | View)

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ArrayType<R> || ViewType<R> )
auto operator+(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op ( std::plus<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ArrayType<R> || ViewType<R> )
auto operator-(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op ( std::minus<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ArrayType<R> || ViewType<R> )
auto operator*(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op ( std::multiplies<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ArrayType<R> || ViewType<R> )
auto operator/(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op ( std::divides<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ArrayType<R> || ViewType<R> )
auto operator%(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op ( std::modulus<> {}, lhs, rhs ); }

// (Array | View ) op Scalar

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ScalarNumType<R> )
auto operator+(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op_r_scalar ( std::plus<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ScalarNumType<R> )
auto operator-(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op_r_scalar ( std::minus<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ScalarNumType<R> )
auto operator*(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op_r_scalar ( std::multiplies<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ScalarNumType<R> )
auto operator/(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op_r_scalar ( std::divides<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<L> || ViewType<L> ) && ( ScalarNumType<R> )
auto operator%(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op_r_scalar ( std::modulus<> {}, lhs, rhs ); }

// Scalar op (Array | View)

template <typename L, typename R>
requires ( ArrayType<R> || ViewType<R> ) && ( ScalarNumType<L> )
auto operator+(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op_l_scalar ( std::plus<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<R> || ViewType<R> ) && ( ScalarNumType<L> )
auto operator-(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op_l_scalar ( std::minus<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<R> || ViewType<R> ) && ( ScalarNumType<L> )
auto operator*(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op_l_scalar ( std::multiplies<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<R> || ViewType<R> ) && ( ScalarNumType<L> )
auto operator/(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op_l_scalar ( std::divides<> {}, lhs, rhs ); }

template <typename L, typename R>
requires ( ArrayType<R> || ViewType<R> ) && ( ScalarNumType<L> )
auto operator%(const L &lhs, const R &rhs) -> decltype(auto) { return binary_op_l_scalar ( std::modulus<> {}, lhs, rhs ); }

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

// Unary ops

template <typename A>
requires ( ArrayType<A> || ViewType<A> )
auto operator-(const A &a) -> decltype(auto) { return unary_op ( std::negate<> {}, a ); }

template <typename A>
requires ( ArrayType<A> || ViewType<A> )
auto abs(const A &a) -> decltype(auto) { return unary_op ( [](auto x) { return std::abs(x); }, a ); }

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
auto transposed(const A &a, int64_t block_size = 16) -> A
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

#endif // ARRAY_HPP
