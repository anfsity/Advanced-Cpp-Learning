#include <iostream>
#include <type_traits>

template <typename T> void my_algo(T &a, T &b) {
  // 错误写法 1：强制使用标准库
  std::swap(a, b);
  // 后果：如果 T 有自定义的高效 swap（比如只交换指针），这里却用了
  // std::swap（也就是三次拷贝），性能极差。

  // 错误写法 2：直接调用
  swap(a, b);
  // 后果：如果 T 是标准类型（如 int），这里编译不过，因为找不到 swap。
}

template <typename T> void my_algo_(T &a, T &b) {
  using std::swap; // 1. 引入标准库兜底
  swap(a,
       b); // 2. 让 ADL (参数依赖查找) 决定是用 std::swap 还是 T 自定义的 swap
}

/////////////////////////////////////////////////////////////////////////

// namespace thirdparty {
//   struct complicated_structure {
//     //....各种指针，引用

//     friend void do_something(complicated_structure &t) noexcept {
//        std::cout << "customized do something" << std::endl;
//     }
//   };

//   struct simple_structure {
//   };
// }

// namespace standard {
//    template<typename T>
//    void do_something(T &t) noexcept {
//       std::cout << "standard do something" << std::endl;
//    }
// }

// int main()
// {
//    thirdparty::simple_structure s;
//    standard::do_something(s);

//    thirdparty::complicated_structure c;
//    standard::do_something(c); // 这里直接在 std 里面查找，thirdparty 相对于
//    std 不可见

//    using namespace standard;
//    do_something(s); // 这里触发了 ADL，导致前后行为不一致
//    do_something(c);

//    return 0;
// }

/////////////////////////////////////////////////////////////////////////

// namespace thirdparty {
//   struct complicated_structure {
//     //....各种指针，引用

//     friend void do_something(complicated_structure &t) noexcept {
//        std::cout << "customized do something" << std::endl;
//     }
//   };

//   struct simple_structure {
//   };
// }

// namespace standard {

//    namespace detail {
//       template<typename T>
//       void do_something(T &t) noexcept {
//          std::cout << "standard do something" << std::endl;
//       }

//       struct do_something_t {
//          template<typename T>
//          void operator()(T &t) noexcept {
//             do_something(t);
//          }
//       };
//    }

//    inline detail::do_something_t do_something{};
// }

// int main()
// {
//    thirdparty::simple_structure s;
//    standard::do_something(s);

//    thirdparty::complicated_structure c;
//    standard::do_something(c); // 引入了 CPO，强制 ADL 执行
//    此时保持了前后一致。

//    using namespace standard;
//    do_something(s);
//    do_something(c);

//    return 0;
// }

/////////////////////////////////////////////////////////////////////////

// 这样看起来已经十分不错了，不是吗

// 考虑如下情况
namespace json_lib {
namespace detail {
struct dump_t {
  // CPO 的 operator()
  template <typename T> void operator()(const T &t) const {
    // 直接通过 ADL 查找名叫 dump 的函数
    dump(t);
  }
};
} // namespace detail
// JSON 库的 CPO
inline constexpr detail::dump_t dump{};
} // namespace json_lib

// --- 第三方库 B：日志记录库 ---
namespace log_lib {
namespace detail {
struct dump_t {
  // CPO 的 operator()
  template <typename T> void operator()(const T &t) const {
    // 直接通过 ADL 查找名叫 dump 的函数
    dump(t); 
  }
};
} // namespace detail
// 日志库的同名 CPO
inline constexpr detail::dump_t dump{};
} // namespace log_lib

namespace my_app {
struct User {
  int id = 42;
  std::string name = "Alice";
};

void dump(const User &) {
  // ...
}

// (重复定义!)
// void dump(const User &u) { ... }
} // namespace my_app

/////////////////////////////////////////////////////////////////////////

// 所以提出了 tag invoke
namespace standard {
namespace detail {
// poison pill
void tag_invoke();
struct tag_invoke_t {
  template <typename Tag, typename... Args>
  constexpr auto operator()(Tag tag, Args &&...args) const
      noexcept(noexcept(tag_invoke(static_cast<Tag &&>(tag),
                                   static_cast<Args &&>(args)...)))
          -> decltype(tag_invoke(static_cast<Tag &&>(tag),
                                 static_cast<Args &&>(args)...)) {
    return tag_invoke(static_cast<Tag &&>(tag), static_cast<Args &&>(args)...);
  }
};
// tag_invoke，为了避免名字冲突
// 提案 P1895 https://open-std.org/JTC1/SC22/WG21/docs/papers/2019/p1895r0.pdf
} // namespace detail

inline constexpr detail::tag_invoke_t tag_invoke{}; // cpo

template <auto &Tag> using tag_t = std::decay_t<decltype(Tag)>;
} // namespace standard

namespace standard {

namespace detail {
struct do_something_t {
  template <typename T> void operator()(T &t) noexcept {
    tag_invoke(do_something_t{}, t); // function
  }
};

// 注意函数定义不再是do_something，而是tag_invoke，tag就是
// detail::do_something_t
template <typename T> void tag_invoke(do_something_t, T &) noexcept {
  std::cout << "standard do something" << std::endl;
}
} // namespace detail

inline detail::do_something_t do_something{};
} // namespace standard

namespace thirdparty {

struct complicated_structure {
  //....各种指针，引用

  // tag_t<do_something>就是standard::detail::do_something_t
  friend void tag_invoke(standard::tag_t<standard::do_something>,
                         complicated_structure &) noexcept {
    std::cout << "customized do something" << std::endl;
  }
};

struct simple_structure {};
} // namespace thirdparty

int main() {
  thirdparty::simple_structure s;
  standard::do_something(s);

  thirdparty::complicated_structure c;
  standard::do_something(c);

  using namespace standard;
  do_something(s);
  do_something(c);

  return 0;
}
