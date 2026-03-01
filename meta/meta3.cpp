#include <print>
#include <type_traits>

/////////////////////////////////////////////////////////////////////

template <bool, typename T = void> struct enable_if : std::type_identity<T> {};

template <typename T> struct enable_if<false, T> {};

template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

// template <typename T> enable_if_t<std::is_integral_v<T>> foo(T) {
//   std::print("is intergal\n");
// }

// template <typename T> enable_if_t<std::is_floating_point_v<T>> foo(T) {
//   std::print("is float\n");
// }

/////////////////////////////////////////////////////////////////////

template <typename... Args> struct always_true : std::true_type {};

template <typename... Args>
static constexpr bool always_true_v = always_true<Args...>::value;

// template <typename T> struct S {
//   template <typename U>
//   static enable_if_t<always_true_v<U> && std::is_same_v<T, int>> foo(U) {
//     std::print("int\n");
//   }

//   template <typename U>
//   static enable_if_t<always_true_v<U> && !std::is_same_v<T, int>> foo(U) {
//     std::print("not int\n");
//   }
// };

/////////////////////////////////////////////////////////////////////

template <typename...> using void_t = void;

template <typename, typename T = void>
struct has_type_member : std::false_type {};

template <typename T>
struct has_type_member<T, void_t<typename T::type>> : std::true_type {};

/////////////////////////////////////////////////////////////////////

namespace detail {
template <typename T> std::type_identity<T> try_add_lvalue_reference(...) {}
template <typename T> std::type_identity<T &> try_add_lvalue_reference(int) {}

template <typename T> std::type_identity<T> try_add_rvalue_reference(...) {}
template <typename T> std::type_identity<T &&> try_add_rvalue_reference(int) {}
} // namespace detail

template <typename T>
struct add_lvalue_reference : decltype(detail::try_add_lvalue_reference<T>(0)) {
};

template <typename T>
struct add_rvalue_reference : decltype(detail::try_add_rvalue_reference<T>(0)) {
};

template <typename T>
using add_lvalue_reference_t = add_lvalue_reference<T>::type;

template <typename T>
using add_rvalue_reference_t = add_rvalue_reference<T>::type;

template <typename T> enable_if_t<std::is_integral_v<T>, int> foo(T) {}
template <typename T> enable_if_t<std::is_floating_point_v<T>, float> foo(T) {}

template <typename T> add_rvalue_reference_t<T> declval() noexcept;

// template <typename T> struct S1 {
//   decltype(foo<T>(declval<T>())) value_;
// };

/////////////////////////////////////////////////////////////////////

template <typename... Args> struct tuple {
  tuple(Args...) {}
};

template <typename T, typename... Args>
struct tuple<T, Args...> : tuple<Args...> {
  tuple(T v, Args... params) : value_(v), tuple<Args...>(params...) {}
  T value_;
};

int main() {
  // foo(1);
  // foo(1.0f);

  // std::print("{}\n", std::is_same_v<int, decltype(S1<int>().value_)>);
  // std::print("{}\n", std::is_same_v<int, decltype(S1<float>().value_)>);

  // S<float>::foo(1.0f);
  // S<int>::foo(1.0f);
  return 0;
}