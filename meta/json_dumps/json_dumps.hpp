#include <concepts>
#include <deque>
#include <forward_list>
#include <list>
#include <memory>
#include <print>
#include <ranges>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace json {

/////////////////////////////////////////////////////////////////

template <typename T, typename... Ts>
concept is_one_of = (std::is_same_v<T, Ts> || ...);

template <typename T>
concept is_pair_like = requires(T t) {
  t.first;
  t.second;
};

template <typename T>
concept is_smart_pointer = requires(const T &ptr) {
  { *ptr };
  { static_cast<bool>(ptr) };
} && !std::is_pointer_v<std::decay_t<T>>;

template <typename T>
concept is_weak_pointer = requires(const T &ptr) {
  { ptr.lock() };
} && !std::is_pointer_v<std::decay_t<T>> && !is_smart_pointer<T>;

// ---------------------- Forward Declarations ----------------------

std::string dumps(const auto &value)
  requires is_one_of<std::decay_t<decltype(value)>, int, long, long long,
                     unsigned, unsigned long, unsigned long long, float, double,
                     long double>;

std::string dumps(const auto &value)
  requires is_one_of<std::decay_t<decltype(value)>, std::string, char>;

static inline std::string dumps(const char *s);

std::string dumps(const auto &value)
  requires is_one_of<std::decay_t<decltype(value)>, void, std::nullptr_t>;

static inline std::string dumps(const bool &b);

template <typename T>
  requires std::ranges::input_range<std::decay_t<T>> &&
           (!std::same_as<std::decay_t<T>, std::string>) &&
           (!is_pair_like<std::ranges::range_value_t<std::decay_t<T>>>)
std::string dumps(const T &container);

template <typename T>
  requires std::ranges::input_range<std::decay_t<T>> &&
           is_pair_like<std::ranges::range_value_t<std::decay_t<T>>>
std::string dumps(const T &map_like);

template <typename T, typename U>
std::string dumps(const std::pair<T, U> &pair);

template <typename T>
  requires std::is_array_v<T>
std::string dumps(const T &arr);

template <typename... Args>
  requires(sizeof...(Args) == 0)
std::string dumps(const std::tuple<Args...> &);

template <typename... Args> 
std::string dumps(const std::tuple<Args...> &tup);

template <typename T> std::string dumps(const T *ptr);

template <typename T>
  requires is_smart_pointer<T>
std::string dumps(const T &ptr);

template <typename T>
  requires is_weak_pointer<T>
std::string dumps(const T &ptr);

// ---------------------- Implementations ----------------------

// normal types
std::string dumps(const auto &value)
  requires is_one_of<std::decay_t<decltype(value)>, int, long, long long,
                     unsigned, unsigned long, unsigned long long, float, double,
                     long double>
{
  return std::format("{}", value);
}

// string char
std::string dumps(const auto &value)
  requires is_one_of<std::decay_t<decltype(value)>, std::string, char>
{
  // clang-format off
    return std::format("\"{}\"",value);
  // clang-format on
}

// char *
static inline std::string dumps(const char *s) { return dumps(std::string(s)); }

// void, nullptr
std::string dumps(const auto &value)
  requires is_one_of<std::decay_t<decltype(value)>, void, std::nullptr_t>
{
  return "null";
}

// bool
static inline std::string dumps(const bool &b) { return b ? "true" : "false"; }

/////////////////////////////////////////////////////////////////

// 这里我们提供两种实现
// 一个是基本的模板元思想，一个使用了 std::ranges

// 和 is_one_of 一样，我们写一个 is_one_of_tmpl
// is_one_of_impl need is_same_template_v
// is_same_template<A, B> return true or false

// template <template <typename...> class T1, template <typename...> class T2>
// struct is_same_template : std::false_type {};

// template <template <typename...> class T>
// struct is_same_template<T, T> : std::true_type {};

// template <template <typename...> class T1, template <typename...> class T2>
// static constexpr bool is_same_template_v = is_same_template<T1, T2>::value;

// template <template <typename...> class T1, template <typename...> class...
// Ts> concept is_one_of_impl = (is_same_template_v<T1, Ts> || ...);

// template <template <typename...> class Tmpl, typename... Args>
//     requires is_one_of_impl<Tmpl, std::vector, std::list, std::deque,
//                             std::forward_list, std::set, std::multiset,
//                             std::unordered_multiset, std::unordered_set>

// std::string dumps(const Tmpl<Args...> &container) {
//     std::string str;
//     for (auto &[i, elem] : container | std::views::enumerate) {
//         str += dumps(elem);
//         if (i != ssize(container)) {
//             str += ",";
//         }
//     }
//     return std::format("[{}]", str);
// }

/////////////////////////////////////////////////////////////////

// 以上是第一种方法，我们采取第二种方法，更为简单。

// vector, list, deque, forward_list, set, multiset, unordered_set,
// unordered_multiset
template <typename T>
  requires std::ranges::input_range<std::decay_t<T>> &&
           (!std::same_as<std::decay_t<T>, std::string>) &&
           (!is_pair_like<std::ranges::range_value_t<std::decay_t<T>>>)
std::string dumps(const T &container) {
  std::string str;
  for (const auto &[i, elem] : container | std::views::enumerate) {
    str += dumps(elem);
    if (i != ssize(container) - 1) {
      str += ", ";
    }
  }
  return std::format("[{}]", str);
}

/////////////////////////////////////////////////////////////////

// map, multimap, unordered_map, unordered_multimap
// 我们仍然可以使用 is_one_of_impl 来实现，不过我喜欢 concept

template <typename T>
  requires std::ranges::input_range<std::decay_t<T>> &&
           is_pair_like<std::ranges::range_value_t<std::decay_t<T>>>
std::string dumps(const T &map_like) {
  std::string str;
  for (const auto &[i, elem] : map_like | std::views::enumerate) {
    str += dumps(elem);
    if (i != ssize(map_like) - 1) {
      str += ", ";
    }
  }
  return std::format("{{{}}}", str);
}

// std::pair
template <typename T, typename U>
std::string dumps(const std::pair<T, U> &pair) {
  return std::format("{{{} : {}}}", dumps(pair.first), dumps(pair.second));
}

/////////////////////////////////////////////////////////////////

// int[][]
template <typename T>
  requires std::is_array_v<T>
std::string dumps(const T &arr) {
  std::string str;
  for (const auto &[i, elem] : arr | std::views::enumerate) {
    str += dumps(elem);
    if (i != std::extent_v<T> - 1) {
      str += ", ";
    }
  }
  return std::format("[{}]", str);
}

/////////////////////////////////////////////////////////////////

// std::tuple
//! 作者的实现有很大的缺陷
//! 在 N = 1 的时候有二义性问题
//! 在 N = 0 的时候有无限递归的问题
// template <size_t N, typename... Args>
//     requires(N == sizeof...(Args) - 1)
// std::string dumps(const std::tuple<Args...> &tup) {
//     return std::format("{}]", std::get<N>(tup));
// }

// template <size_t N, typename... Args>
//     requires(N != 0 && N != sizeof...(Args) - 1)
// std::string dumps(const std::tuple<Args...> &tup) {
//     return std::format("{}, {}", std::get<N>(tup), dumps<N + 1,
//     Args...>(tup));
// }

// template <size_t N = 0, typename... Args>
//     requires(N == 0)
// std::string dumps(const std::tuple<Args...> &tup) {
//     return std::format("[{}, {}", std::get<N>(tup), dumps<N + 1,
//     Args...>(tup));
// }

template <typename... Args>
  requires(sizeof...(Args) == 0)
std::string dumps(const std::tuple<Args...> &) {
  return "[]";
}

template <typename... Args> std::string dumps(const std::tuple<Args...> &tup) {
  return std::apply(
      [](const auto &...args) {
        std::string str;
        size_t index = 0;
        ((str += dumps(args) + (++index == sizeof...(Args) ? "" : ", ")), ...);
        return std::format("[{}]", str);
      },
      tup);
}

/////////////////////////////////////////////////////////////////

template <typename T> std::string dumps(const T *ptr) {
  if (ptr) {
    return dumps(*ptr);
  }
  return "null";
}

template <typename T>
  requires is_smart_pointer<T>
std::string dumps(const T &ptr) {
  if (ptr) {
    return dumps(*ptr);
  }
  return "null";
}

template <typename T>
  requires is_weak_pointer<T>
std::string dumps(const T &ptr) {
  if (auto shared = ptr.lock()) {
    return dumps(*shared);
  }
  return "null";
}

} // namespace json

// 我好像很久之前写过类似的东西...
// 当初是为了在写题时快速，方便打印类型和变量...
// 其实没有太大区别...
// 当初还用宏封装了一下，还挺好玩的
// 回旋镖？
// 参考 https://zhuanlan.zhihu.com/p/395165250 实现
// 文章最后关于 json dumps 代码存在诸多漏洞，谨慎阅读