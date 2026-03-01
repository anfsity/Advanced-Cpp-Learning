#include <array>
#include <list>
#include <print>
#include <vector>

// https://zhuanlan.zhihu.com/p/384826036

// integral_constant : 把值提升成为类型
template <typename T, T v> struct integral_constant {
  static constexpr T value = v;
  using value_type = T;
  using type = integral_constant;
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};

// 我们的模板元，有三种模板，非类型模板实参，类型模板实参，还有模板模板实参。
// 对于 meta function 的写法，一般是利用偏特化和全特化。
// 对于函数来说，一般是用函数重载协议，函数和模板可以利用显示模板实例化。
// 不过我们通常使用隐式实例化。
// 这是目前所有的知识点。
// 对于模板函数，使用继承来使得语义明确。

template <bool B> using bool_constant = integral_constant<bool, B>;

// 两个类型，可以使用标签分发
using true_type = bool_constant<true>;
using false_type = bool_constant<false>;

template <typename T> struct is_reference : false_type {};

template <typename T> struct is_reference<T &> : true_type {};

template <typename T> struct is_reference<T &&> : true_type {};

template <typename T> struct is_int_or_reference : is_reference<T> {};
template <> struct is_int_or_reference<int> : true_type {};

template <typename T>
inline constexpr bool is_reference_v = is_reference<T>::type::value;

/////////////////////////////////////////////////////////////

template <typename T> struct type_identity {
  using type = T;
};

template <typename T> struct remove_reference : type_identity<T> {};

template <typename T> struct remove_reference<T &> : type_identity<T> {};

template <typename T> struct remove_reference<T &&> : type_identity<T> {};

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

// 移动语义 :P)
template <typename T> constexpr remove_reference_t<T> &&move(T &&t) noexcept {
  return static_cast<remove_reference_t<T> &&>(t);
}

////////////////////////////////////////////////////////////////

template <typename T, typename U> struct is_same : false_type {};

template <typename T> struct is_same<T, T> : true_type {};

template <typename T, typename U>
static constexpr bool is_same_v = is_same<T, U>::value;

////////////////////////////////////////////////////////////////

template <bool B, typename T, typename F>
struct conditional : type_identity<T> {};

template <typename T, typename F>
struct conditional<false, T, F> : type_identity<F> {};

template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

////////////////////////////////////////////////////////////////

// 实现了短路求值，也可以使用递归，在 c++17 中，可以使用折叠表达式
template <typename T, typename U, typename... Rest>
struct is_one_of
    : conditional_t<is_same_v<T, U>, true_type, is_one_of<T, Rest...>> {};

template <typename T, typename U>
struct is_one_of<T, U> : conditional_t<is_same_v<T, U>, true_type, false_type> {
};

template <typename T>
using is_integral = is_one_of<T, bool, char, short, int, long, long long>;

template <typename T>
static constexpr bool is_integral_v = is_integral<T>::value;

////////////////////////////////////////////////////////////////

template <typename Inst, template <typename...> typename Tmpl>
struct is_instantiation_of : false_type {};

template <template <typename...> typename Tmpl, typename... Args>
struct is_instantiation_of<Tmpl<Args...>, Tmpl> : true_type {};

////////////////////////////////////////////////////////////////

template <typename T> struct rank : integral_constant<size_t, 0> {};

template <typename T>
struct rank<T[]> : integral_constant<size_t, rank<T>::value + 1> {};

template <typename T, size_t N>
struct rank<T[N]> : integral_constant<size_t, rank<T>::value + 1> {};

template <typename T> static constexpr size_t rank_v = rank<T>::value;

////////////////////////////////////////////////////////////////

// 主模板, not an array
template <typename T, unsigned N = 0>
struct extent : integral_constant<size_t, 0> {};

// base case, unbounded array dim 0
template <typename T> struct extent<T[], 0> : integral_constant<size_t, 0> {};

// 递归 unbounded array
template <typename T, unsigned N> struct extent<T[], N> : extent<T, N - 1> {};

// base case, bounded array dim 0
template <typename T, size_t I>
struct extent<T[I], 0> : integral_constant<size_t, I> {};

// recursive, bounded array
template <typename T, size_t I, unsigned N>
struct extent<T[I], N> : extent<T, N - 1> {};

/////////////////////////////////////////////////////////////////

int main() {
  std::print("{}\n", is_same_v<int, float>);
  std::print("{}\n", is_same_v<int, int>);

  std::print("{}\n", is_instantiation_of<std::vector<int>, std::list>::value);
  std::print("{}\n", is_instantiation_of<std::vector<std::pair<int, int>>,
                                         std::vector>::value);

  int x = 1;
  std::print("{}\n", is_integral_v<decltype(x)>);
  int *x_ptr = &x;
  std::print("{}\n", is_integral_v<decltype(x_ptr)>);

  int a[7][8];
  // clang-format off
  std::print("{}\n", rank_v<decltype(a)>);               // 2
  std::print("{}\n", extent<int>::value);                           // 0
  std::print("{}\n", extent<decltype(a)>::value);        // 7
  std::print("{}\n", extent<decltype(a), 0>::value);     // 7
  std::print("{}\n", extent<decltype(a), 1>::value);     // 8
  std::print("{}\n", extent<decltype(a), 2>::value);     // 0
  std::print("{}\n", extent<decltype(a), 3>::value);     // 0
  // clang-format on

  std::array<int, 2> arr1;
  std::array<std::array<std::array<int, 2>, 2>, 3> arr2;
  std::print("{}\n", rank_v<decltype(arr1)>);
  std::print("{}\n", rank_v<decltype(arr2)>);
}