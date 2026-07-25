#ifndef FORWARD_DECLARATIONS_HPP
#define FORWARD_DECLARATIONS_HPP

#include <cstdint>
#include <type_traits>
#include <limits>
#include <concepts>
#include <complex>


// ValuePtr

class ICloneable;

template<typename T>
concept ValuePtrPointee =
    ( std::rank_v<T> == 0 && !std::is_reference_v<T> ) ||
    ( std::rank_v<T> == 1 &&
      std::is_unbounded_array_v<T> &&
      !std::is_reference_v<std::remove_extent_t<T>> );

template<ValuePtrPointee T>
class ValuePtr;

constexpr int64_t dynamic_extent = std::numeric_limits<int64_t>::max();


// Extents

template<int64_t N>
struct Extents;


// Helper functions

template<std::integral T>
constexpr auto next_multiple(T n, T f) -> T;

template<int64_t D>
constexpr auto all_of_indices_in_bounds(const Extents<D> &indices, const Extents<D> &extents) -> bool;

template<int64_t D>
constexpr auto check_bounds(const Extents<D> &indices, const Extents<D> &extents) -> void;

template<int64_t D>
constexpr auto extents_intersection(const Extents<D> &e1, const Extents<D> &e2) -> Extents<D>;

template<int64_t D>
constexpr auto all_of_extents_static(const Extents<D> &extents) -> bool;

template<int64_t D>
constexpr auto all_of_extents_dynamic(const Extents<D> &extents) -> bool;

template<int64_t D>
constexpr auto all_of_extents_zero(const Extents<D> &extents) -> bool;

template<int64_t D>
constexpr auto all_of_extents_positive(const Extents<D> &extents) -> bool;

template<int64_t D>
constexpr auto all_of_extents_powers_of_2(const Extents<D> &extents) -> bool;

template<int64_t D>
constexpr auto all_of_extents_equal(const Extents<D> &extents) -> bool;

template<int64_t D>
constexpr auto compute_extents_countr_zero(const Extents<D> &extents) -> Extents<D>;

template<int64_t D>
constexpr auto extents_valid(const Extents<D> &extents) -> bool;

template<int64_t N>
constexpr auto make_extents_filled(const int64_t &v) -> Extents<N>;

template<int64_t N>
constexpr auto make_extents_iota(int64_t min) -> Extents<N>;

template<int64_t DSpace, int64_t DSubspace>
constexpr auto axis_selection_valid(const Extents<DSubspace> &axes) -> bool;

template<int64_t N, typename Tuple>
constexpr auto tuple_take(const Tuple &tuple) -> auto;

template<typename T, typename Tuple>
constexpr auto make_array_from_tuple(const Tuple &tuple) -> auto;


// Layout types

template <
    int64_t D,
    Extents<D> AxisPermutation = make_extents_iota<D>(0)
>
class Affine;

template <
    int64_t D,
    Extents<D> AxisPermutation,
    Extents<D> BlockExtents
>
class Blocked;

template <int64_t D>
class Morton;


// Iterator types

template<typename A, bool IsReadOnly>
class BasicIndexTupleIterator;

template<typename A>
using IndexTupleIterator = BasicIndexTupleIterator<A, false>;

template<typename A>
using ReadOnlyIndexTupleIterator = BasicIndexTupleIterator<A, true>;


template<typename A, bool IsReadOnly>
class BasicContiguousElementIterator;

template<typename A>
using ContiguousElementIterator = BasicContiguousElementIterator<A, false>;

template<typename A>
using ReadOnlyContiguousElementIterator = BasicContiguousElementIterator<A, true>;


// View types

template <
    typename A,
    bool IsReadOnly,
    int64_t D,
    Extents<D> ViewIndexSubspace
>
class BasicSliceView;

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


template <
    typename A,
    bool IsReadOnly,
    int64_t D,
    Extents<A::dimension()> AIndexSubspace
>
class BasicBroadcastView;

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


template <
    typename A,
    bool IsReadOnly
>
class BasicIdentityView;

template <
    typename A,
    int64_t D = A::dimension(),
    Extents<A::dimension()> AIndexSubspace = make_extents_iota<A::dimension()>(0)
>
using IdentityView = BasicIdentityView<A, false>;

template <
    typename A,
    int64_t D = A::dimension(),
    Extents<A::dimension()> AIndexSubspace = make_extents_iota<A::dimension()>(0)
>
using ReadOnlyIdentityView = BasicIdentityView<A, true>;


// Array

template <
    typename   T,
    int64_t    D,
    Extents<D> E = make_extents_filled<D>(dynamic_extent),
    typename   L = Affine<D>
>
class Array;


// Expression node types

template<typename T>
using Bare = std::remove_cvref_t<T>;

template <typename Op, typename E>
class UnaryOpNode;

template <typename Op, typename L, typename R>
requires (
    ( Bare<L>::dimension() == Bare<R>::dimension() ) &&
    (
        !Bare<L>::is_of_static_extents() ||
        !Bare<R>::is_of_static_extents() ||
        ( Bare<L>::type_extents() == Bare<R>::type_extents() )
    )
)
class BinaryZipMapOpNode;

template <typename Op, typename L, typename R>
class BinaryRScalarOpNode;

template <typename Op, typename L, typename R>
class BinaryLScalarOpNode;


// Scalar numerical types

template<typename T, typename U>
concept SameAsUpToCVRef = std::same_as <
    std::remove_cvref_t<T>,
    std::remove_cvref_t<U>
>;

template<typename T>
concept ScalarRealSignedIntegerNumType =
    SameAsUpToCVRef<T, int8_t>  ||
    SameAsUpToCVRef<T, int16_t> ||
    SameAsUpToCVRef<T, int32_t> ||
    SameAsUpToCVRef<T, int64_t>;

template<typename T>
concept ScalarRealUnsignedIntegerNumType =
    SameAsUpToCVRef<T, uint8_t>  ||
    SameAsUpToCVRef<T, uint16_t> ||
    SameAsUpToCVRef<T, uint32_t> ||
    SameAsUpToCVRef<T, uint64_t>;

template<typename T>
concept ScalarRealIntegerNumType =
    ScalarRealSignedIntegerNumType<T> ||
    ScalarRealUnsignedIntegerNumType<T>;

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


// Iterators

template <typename T>
struct IsIndexTupleIteratorType : std::false_type {};

template <typename A, bool IsReadOnly>
struct IsIndexTupleIteratorType<BasicIndexTupleIterator<A, IsReadOnly>> : std::true_type {};

template <typename T>
concept IndexTupleIteratorType = IsIndexTupleIteratorType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsContiguousElementIteratorType : std::false_type {};

template <typename A, bool IsReadOnly>
struct IsContiguousElementIteratorType<BasicContiguousElementIterator<A, IsReadOnly>> : std::true_type {};

template <typename T>
concept ContiguousElementIteratorType = IsContiguousElementIteratorType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsIteratorType :
    std::bool_constant <
        IsIndexTupleIteratorType<T>::value        ||
        IsContiguousElementIteratorType<T>::value
    > {};

template <typename T>
concept IteratorType = IsIteratorType<std::remove_cvref_t<T>>::value;


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
struct IsIdentityViewType : std::false_type {};

template <typename A, bool IsReadOnly>
struct IsIdentityViewType<BasicIdentityView<A, IsReadOnly>> : std::true_type {};

template <typename T>
concept IdentityViewType = IsIdentityViewType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsViewType :
    std::bool_constant <
        IsSliceViewType<T>::value     ||
        IsBroadcastViewType<T>::value ||
        IsIdentityViewType<T>::value
    > {};

template <typename T>
concept ViewType = IsViewType<std::remove_cvref_t<T>>::value;


// Arrays

template <typename T>
struct IsArrayType : std::false_type {};

template <typename T, int64_t D, Extents<D> E, typename L>
struct IsArrayType<Array<T, D, E, L>> : std::true_type {};

template <typename T>
concept ArrayType = IsArrayType<std::remove_cvref_t<T>>::value;


// Expression nodes

template <typename T>
struct IsUnaryOpNodeType : std::false_type {};

template <typename Op, typename E>
struct IsUnaryOpNodeType<UnaryOpNode<Op, E>> : std::true_type {};

template <typename T>
concept UnaryOpNodeType = IsUnaryOpNodeType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsBinaryZipMapOpNodeType : std::false_type {};

template <typename Op, typename L, typename R>
struct IsBinaryZipMapOpNodeType<BinaryZipMapOpNode<Op, L, R>> : std::true_type {};

template <typename T>
concept BinaryZipMapOpNodeType = IsBinaryZipMapOpNodeType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsBinaryRScalarOpNodeType : std::false_type {};

template <typename Op, typename L, typename R>
struct IsBinaryRScalarOpNodeType<BinaryRScalarOpNode<Op, L, R>> : std::true_type {};

template <typename T>
concept BinaryRScalarOpNodeType = IsBinaryRScalarOpNodeType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsBinaryLScalarOpNodeType : std::false_type {};

template <typename Op, typename L, typename R>
struct IsBinaryLScalarOpNodeType<BinaryLScalarOpNode<Op, L, R>> : std::true_type {};

template <typename T>
concept BinaryLScalarOpNodeType = IsBinaryLScalarOpNodeType<std::remove_cvref_t<T>>::value;


template <typename T>
struct IsExpressionNodeType :
    std::bool_constant <
        IsUnaryOpNodeType<T>::value         ||
        IsBinaryZipMapOpNodeType<T>::value        ||
        IsBinaryRScalarOpNodeType<T>::value ||
        IsBinaryLScalarOpNodeType<T>::value ||
        IsArrayType<T>::value               ||
        IsViewType<T>::value
    > {};

template <typename T>
concept ExpressionNodeType = IsExpressionNodeType<std::remove_cvref_t<T>>::value;

#endif // FORWARD_DECLARATIONS_HPP

