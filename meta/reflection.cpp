#include <array>
#include <cassert>
#include <meta>
#include <print>
#include <string>
#include <type_traits>
#include <vector>

// 反射！！！
// Reflection !!!

constexpr auto no_check = std::meta::access_context::unchecked();

consteval auto fields_of(std::meta::info type) {
  return std::define_static_array(
      std::meta::nonstatic_data_members_of(type, no_check));
}

template <std::meta::info R> std::string printFullName() {
  if constexpr (R == std::meta::info()) {
    return "";
  }

  constexpr auto parent = std::meta::parent_of(R);

  std::string res;

  if constexpr (parent != std::meta::info() && parent != ^^::) {
    res += printFullName<parent>();
    res += "::";
  }

  if constexpr (std::meta::has_identifier(R)) {
    res += std::format("{}", std::meta::identifier_of(R));
  } else {
    res += std::format("{}", std::meta::display_string_of(R));
  }

  return res;
}

//////////////////////////////////////////////////////////////////

struct Point {
  int x;
  int y;
};

namespace detail {

struct Math {
  int operator+(Math) { return 1; }
};

} // namespace detail

template <typename T> struct foo : std::true_type {};

void test1() {
  Point p{1, 2};
  constexpr auto rx =
      std::meta::nonstatic_data_members_of(^^Point, no_check)[0];
  constexpr auto ry =
      std::meta::nonstatic_data_members_of(^^Point, no_check)[1];

  p.[:rx:] = 3;
  p.[:ry:] = 4;

  std::print("{}, {}\n", p.x, p.y);

  std::print("Point: {}, {}\n", std::meta::identifier_of(rx),
             std::meta::identifier_of(ry));             // Point: x, y
  std::print("{}\n", std::meta::display_string_of(rx)); // x
  std::print("{}::{}\n", std::meta::display_string_of(std::meta::parent_of(rx)),
             std::meta::display_string_of(rx)); // Point::x
  std::print("{}::{}\n", std::meta::identifier_of(std::meta::parent_of(rx)),
             std::meta::identifier_of(rx)); // Point::x

  std::vector<int> arr = {1, 2, 3, 4};
  constexpr std::meta::info rarr = ^^std::vector;
  constexpr auto loc = std::meta::source_location_of(rarr);
  std::print("file name: {}\nline: {}\ncolumn: {}\nfunction name: {}\n",
             loc.file_name(), loc.line(), loc.column(), loc.function_name());

  constexpr auto op = ^^detail::Math::operator+;
  std::print("{}\n", std::meta::display_string_of(op));
  // std::print("{}\n", std::meta::identifier_of(op));
  // error: call to consteval function 'std::meta::identifier_of' is not a
  // constant expression

  constexpr auto r = ^^foo<int>;
  std::print("{}\n", std::meta::display_string_of(r));
  std::print("{}\n", printFullName<r>());
  std::print("{}\n", printFullName<op>());
}

//////////////////////////////////////////////////////////////////

void test2() {

  constexpr auto rarr = ^^std::vector<int>;

  static_assert(^^int == ^^int);
  static_assert(^^int != ^^const int);
  static_assert(^^int != ^^const int &);

  using alias = int;
  static_assert(dealias(^^alias) == ^^int);

  namespace AliasNS = ::std;
  static_assert(^^::std != ^^AliasNS);
  static_assert(^^:: == parent_of(^^::std));
}

////////////////////////////////////////////////////////////////////

enum class Color { Red, Green, Blue };

template <typename E>
  requires std::is_enum_v<E>
constexpr void enum_to_string(E value) {
  // P2996 返回 std::vector<std::meta::info>，但由于当前的 constexpr
  // 限制，动态分配在堆上的 vector 无法直接被隐式捕获进 constexpr 变量给
  // template for 展开，所以使用 lambda 先将其转为定长的 std::array

  // constexpr auto enums = []() {
  //   constexpr auto N = std::meta::enumerators_of(^^E).size();
  //   std::array<std::meta::info, N> arr{};
  //   auto vec = std::meta::enumerators_of(^^E);
  //   for (size_t i = 0; i < N; ++i)
  //     arr[i] = vec[i];
  //   return arr;
  // }();

  constexpr auto enums =
      std::define_static_array(std::meta::enumerators_of(^^E));

  // std::println("{}", enums.size());
  // 当我写下这行代码时，我才发现我对 cpp 的 compile time 和 run time 并不了解
  // std::meta::info 是一个编译器内部 AST 的 handle，不具备实际内存
  // 这说明，enums 本身也是一个 consteval-only type
  // 既然如此，当我们在 println 中写下 emums.size 时，这个运算就被提升到 run
  // time 了 所以才会报错
  // 知道了这样的原理，我们可以用一个编译期存在的变量来储存他：
  // constexpr auto enums_size = enums.size()
  // std::println("{}", enums_size);

  // template for 会在编译期将代码展开，类似于宏展开，但它是类型安全的
  template for (constexpr auto e : enums) {
    std::println("{}", std::meta::identifier_of(e));
    // 这里与 enums.size() 不同的是，identifier_of 虽然也是一个 consteval-only
    // 的函数，但是其返回值是一个 string_view
    // 字符串常量是可以在 run time 期间存活的，相当于一个编译期常量展开
    // 在被 println 调用之前，identifier of 就已经被转换为 "Red"
    // 这样的字符串常量了
  }
}

struct User {
  std::string name;
  int id;
  int score;
};

template <typename T> void print_json(const T &obj) {
  std::print("{{ ");

  // 反射直接在编译期拿到 User 的所有成员
  constexpr auto members = fields_of(^^T);

  bool first = true;
  template for (constexpr auto mem : members) {
    if (!first)
      std::print(", ");
    first = false;

    // identifier_of 拿到名字 (比如 "name", "id")
    // obj.[:mem:] 也就是把句柄“插”回对象，等效于运行期的 obj.name 等
    std::print("\"{}\": {}", std::meta::identifier_of(mem), obj.[:mem:]);
  }
  std::println(" }}");
}

////////////////////////////////////////////////////////////////////


struct MyTuple; // 用于被 define_aggregate 注入的“空壳”类型

consteval void define_tuple() {
  // 定义成员规格
  std::array<std::meta::info, 3> members = {
      std::meta::data_member_spec(^^int),
      std::meta::data_member_spec(^^double),
      std::meta::data_member_spec(^^std::string)
  };
  // 利用 define_aggregate 将成员注入到空壳中，使其变成为真正的 structural struct
  std::meta::define_aggregate(^^MyTuple, members);
}
// 执行编译期注入 (P2996 提案支持的代码块注入)
consteval { define_tuple(); }

void test3() {
  MyTuple t;
  
  // MyTuple 已经被反射生成完毕，现在可以照常获取它的成员
  constexpr auto members = std::define_static_array(
      std::meta::nonstatic_data_members_of(^^MyTuple, no_check));
      
  t.[: members[0] :] = 42;
  t.[: members[2] :] = "hello";

  std::println("{}, {}", t.[: members[0] :], t.[: members[2] :]);
}

////////////////////////////////////////////////////////////////////


int main() {
  constexpr std::meta::info rint = ^^int; // Ｏ(≧▽≦)Ｏ
  using int2 = [:rint:];
  int2 a = 10;
  assert(a == 10);

  using L = int;
  constexpr std::meta::info o_O = (^^L);
  using O_o = [:o_O:]; // 哈哈

  test1();
  test2();
  enum_to_string(Color{});
  print_json(User{"Alice", 1, 100});
  // annoation();
  test3();
  return 0;
}