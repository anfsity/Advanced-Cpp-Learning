#include "json_dumps.hpp"
#include <print>
#include <string>

using namespace json;

template <typename T> std::string dumps(const typename T::type &value) {
    return std::format("struct format {}\n", value);
}

template <typename T> struct S {
    using type = T;
};

namespace user {
    struct UserDefine {
        int age = 100;
    };

    std::string dumps(const UserDefine &user) {
        return std::format("{{\"age\": {}}}", user.age);
    }
}

int main() {
    int x = 1;
    std::print("{}\n", dumps(x));

    S<int>::type s = 100;
    std::print("{}\n", dumps<S<int>>(s));

    std::array<std::array<int, 2>, 2> arr = {{{1, 2}, {3, 4}}};
    std::print("{}\n", dumps(arr));

    int a[2][2] = {{1, 2}, {3, 4}};
    std::print("{}\n", dumps(a));

    auto tup = std::tuple(1, 1u, 1ll, "abc", 'd', 1.0f);
    std::print("{}\n", dumps(tup));

    auto tup1 = std::tuple(1);
    std::print("{}\n", dumps(tup1));

    std::vector<user::UserDefine> vec = {{1}, {2}};
    std::print("{}\n", dumps(vec));
}