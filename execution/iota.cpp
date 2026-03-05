#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <execution>
#include <iterator>
#include <numeric>
#include <print>
#include <ranges>
#include <vector>

// https://en.cppreference.com/w/cpp/algorithm/ranges/iota.html

//* struct iota_fn {
//*   template <std::input_or_output_iterator O, std::sentinel_for<O> S,
//*             std::weakly_incrementable T>
//*     requires std::indirectly_writable<O, const T &>
//*   constexpr iota_result<O, T> operator()(O first, S last, T value) const {
//*     while (first != last) {
//*       *first = as_const(value);
//*       ++first;
//*       ++value;
//*     }
//*     return {std::move(first), std::move(value)};
//*   }

//*   template <std::weakly_incrementable T, std::ranges::output_range<const T
//&> R>
//*   constexpr iota_result<std::ranges::borrowed_iterator_t<R>, T>
//*   operator()(R &&r, T value) const {
//*     return (*this)(std::ranges::begin(r), std::ranges::end(r),
//*                    std::move(value));
//*   }
//* };

// inline constexpr iota_fn iota;

// 我们来看一下 `ranges::iota` 是怎么实现的
// 以及思考一下并行版本的 iota 应该怎么做

// 首先，除了 std::move，其他的 concepts 我一概不认得...
// std::input_or_output_iterator O
// O 必须是一个能读或者能写的迭代器。

// std::sentinel_for<O> S
// S 必须是一个能和 O 比较的哨兵

// std::weakly_incrementable T
// T 必须是一个可自增的类型（比如 ++T），但不要求 -- 或者拷贝

// std::indirectly_writable<O, const T &> 要求 T 必须能够写入到 *O 里面

// std::ranges::output_range<const T &> R
// R 必须是一个 range，并且能够把 const T& 塞进这个 range 里面

// borrowed_iterator_t 比较有意思
// 如果传进去一个左值，会返回原本的左值类型
// 但如果传进去一个右值，就会返回 std::ranges::dangling (cpp reference
// 这个词条好像挂了)

// template <typename T>
// constexpr const T& as_const(T &t) noexcept {
//   return t;
// }

// template <typename T>
// void as_const(const T&&) = delete;
// as_const 的实现大致如上

// 如果看懂了这些乱七八糟的 concept 后，整个函数还是十分直观的

template <typename O, typename T> struct iota_result {
  O out;
  T value;
};

struct parallel_iota_fn {
  template <std::random_access_iterator O, std::sized_sentinel_for<O> S,
            typename T>
    requires std::indirectly_writable<O, const T &> &&
             requires(T t, std::iter_difference_t<O> d) {
               { t + d } -> std::convertible_to<T>;
             }
  constexpr iota_result<O, T> operator()(O first, S last, T value) const {
    if (first == last) {
      return {std::move(first), std::move(value)};
    }

    auto dist = last - first;
    auto indices = std::views::iota(std::iter_difference_t<O>{0}, dist);

    std::for_each(std::execution::par_unseq, indices.begin(), indices.end(),
                  [first, value](auto i) { first[i] = value + i; });

    return {first + dist, static_cast<T>(value + dist)};
  }

  template <std::ranges::random_access_range R, typename T>
    requires std::ranges::sized_range<R> &&
             std::indirectly_writable<std::ranges::iterator_t<R>, const T &> &&
             requires(T t, std::ranges::range_difference_t<R> d) {
               { t + d } -> std::convertible_to<T>;
             }
  constexpr iota_result<std::ranges::borrowed_iterator_t<R>, T>
  operator()(R &&r, T value) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r),
                   std::move(value));
  }
};

inline constexpr parallel_iota_fn p_iota{};

// 本来用这个 p_iota 和 ranges::iota 做了一下 benchmark
// 但是这个是 memory bound 类型，使用的是往 vector 里面填充 int
// 在我的设备上完全体现不出来并行的优势
// 而且很神奇的是，最开始的运行并行版本是串行的 1.5x ，然后越跑越慢（？）
// 最终的结果反而是并行的慢于串行
// 和哈基米的交流后，得出的结论是，并行的总线仲裁，缓存一致性，线程调度和启动这些额外开销
// 导致了并行版本不如串行
// 而且 ios 标准里面也没有给出并行 iota 的实现。
// 然后下面是一个 cpu bound 的例子，优化在 8x-9x 之间。

constexpr long N_CPU = 50'000'000;

void test_cpu_bound() {
  std::vector<double> vec(N_CPU);
  p_iota(vec, 0.0);

  // --- 串行版本 ---
  auto start_std = std::chrono::high_resolution_clock::now();

  std::for_each(vec.begin(), vec.end(), [](double& v) {
      // 故意加入极高复杂度的 CPU 密集型数学运算
      v = std::sin(v) * std::cos(v) + std::sqrt(v);
  });

  auto end_std = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration_std = end_std - start_std;

  std::print("Serial (CPU Bound) time: {}s\n", duration_std.count());

  // 再次初始化
  p_iota(vec, 0.0);

  // --- 并行版本 ---
  auto start_par = std::chrono::high_resolution_clock::now();

  std::for_each(std::execution::par_unseq, vec.begin(), vec.end(), [](double& v) {
      // 一模一样的数学运算
      v = std::sin(v) * std::cos(v) + std::sqrt(v);
  });

  auto end_par = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration_par = end_par - start_par;

  std::print("Parallel (CPU Bound) time: {}s\n", duration_par.count());

  std::print("=> CPU Bound Speedup: {:.2f}x\n\n", duration_std.count() / duration_par.count());
}

int main() {
  std::print("Testing CPU Bound (Compute Intensive) tasks...\nData size: {} elements\n\n", N_CPU);
  
  for(int i = 0; i < 5; ++i) {
    test_cpu_bound();
  }

  return 0;
}
