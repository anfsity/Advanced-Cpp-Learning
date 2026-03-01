#include <print>
#include <type_traits>

/////////////////////////////////////////////////////////////////////

// https://zhuanlan.zhihu.com/p/395165250
template <bool, typename T = void> struct enable_if : std::type_identity<T> {};

template <typename T> struct enable_if<false, T> {};

template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

// template <typename T> enable_if_t<std::is_integral_v<T>> foo(T) {
//   std::print("is integral\n");
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

// T& = const T&
template <typename T>
using copy_assign_t = decltype(declval<T&>() = declval<T const&>());

template <typename T, typename = void>
struct is_copy_assignable : std::false_type {};

template <typename T>
struct is_copy_assignable<T, void_t<copy_assign_t<T>>> : std::true_type {};

/////////////////////////////////////////////////////////////////////

template <typename... Args> struct tuple {
  tuple(Args...) {}
};

template <typename T, typename... Args>
struct tuple<T, Args...> : tuple<Args...> {
  tuple(T v, Args... params) : tuple<Args...>(params...), value_(v) {}
  T value_;
};

// tuple<1, 1.0f, 'a'> -- tuple<int, float, char>
// tuple<int, float, char> : tuple<float, char> int value_ = 1;
// tuple<float, char> : tuple<char> float value_ = 1.0f;
// tuple<char> : tuple<> char value_ = 'a';

template <unsigned N, typename Tpl>
struct tuple_element;

template <unsigned N, typename T, typename... Args>
struct tuple_element<N, tuple<T, Args...>> : tuple_element<N - 1, tuple<Args...>> {};

template <typename T, typename... Args>
struct tuple_element<0, tuple<T, Args...>> : std::type_identity<T> {
  using tuple_type = tuple<T, Args...>;
};

template <unsigned N, typename Tpl>
using tuple_element_t = tuple_element<N, Tpl>::type;

template <unsigned N, typename... Args>
tuple_element_t<N, tuple<Args...>>& get(tuple<Args...> &t) {
  using tuple_type = typename tuple_element<N, tuple<Args...>>::tuple_type;
  return static_cast<tuple_type&>(t).value_;
};

// get<1>(tuple<int, float, char>)
// Args...: int, float, char; N = 1; 
// tuple_element<1, tuple<int, float, char>>
// tuple_element<1, tuple<int, float, char>> : tuple_element<0, tuple<float, char>>
// tuple_element<0, tuple<float, char>> : type_identity<float> tuple_type = tuple<float, char>
// tuple<float, char>::T = float;
// tuple<int, float, char> &t -> tuple<float, char> &t
// tuple<float, char>::value_ = float
// tuple_element<0, tuple<float, char>>::type = type_identity<float> = float

int main() {
  // foo(1);
  // foo(1.0f);

  // std::print("{}\n", std::is_same_v<int, decltype(S1<int>().value_)>);
  // std::print("{}\n", std::is_same_v<int, decltype(S1<float>().value_)>);

  // S<float>::foo(1.0f);
  // S<int>::foo(1.0f);

  int i = 1;
  auto t = tuple(0, i, '2', 3.0f, 4ll, std::string("five"));
  i = 0;
  std::print("{}\n", get<1>(t));
  get<1>(t) = 0;
  std::print("{}\n", get<1>(t));
  return 0;
}