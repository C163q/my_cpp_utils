#include"../../src/rs/result.hpp"
#include<cassert>
#include<exception>
#include<tuple>

int main() {
    {
        auto x = C163q::Ok<const char*>(-3);
        assert(x.is_ok() == true);

        auto y = C163q::Err<int, const char*>("Some error message");
        assert(y.is_ok() == false);
    }
    {
        C163q::Result<unsigned, const char*> x(2);
        assert(x.is_ok_and([](auto v) { return v > 1; }) == true);

        auto y = C163q::Ok<const char*>(0u);
        assert(y.is_ok_and([](const unsigned& v) { return v > 1; }) == false);

        C163q::Result<unsigned, const char*> z("hey");
        assert(z.is_ok_and([](auto v) { return v > 1; }) == false);
    }
    {
        auto x = C163q::Ok<std::exception, std::tuple<int, float>>(1, 1.0f);
        assert(x.is_err() == false);

        auto y = C163q::Err<std::tuple<int, float>, std::exception>();
        assert(y.is_err() == true);
    }
}

