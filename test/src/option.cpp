
#include"../../include/rs/option.hpp"
#include <tuple>

int main() {
    C163q::Option<int> a(1);

    C163q::Option<std::tuple<int, int>> b(std::make_tuple(1, 1));

    b.unzip();
}


