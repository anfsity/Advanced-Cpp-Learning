#include <cassert>
#include <meta>
#include <print>
#include <string>
#include <type_traits>
#include <vector>

// 反射！！！
// Reflection !!!

constexpr auto no_check = std::meta::access_context::unchecked();

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
  return 0;
}