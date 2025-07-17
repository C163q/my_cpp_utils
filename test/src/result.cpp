#include"../../src/rs/result.hpp"
#include<cassert>
#include<exception>
#include <optional>
#include <stdexcept>
#include <string_view>
#include<tuple>
#include <vector>

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
    {
        auto x = C163q::Err<unsigned, std::invalid_argument>("gets invalid argument");
        assert(x.is_err_and([](const std::invalid_argument& e) {
                    return std::string_view("gets invalid argument") == e.what();
                    }) == true);

        C163q::Result<long, const char*> y("Error");
        assert(y.is_err_and([](const char* p) { return p[0] == 'e'; }) == false);

        auto z = C163q::Ok<std::exception, unsigned>(1);
        assert(z.is_err_and([](const std::exception&) { return true; }) == false);
    }
    {
        auto x = C163q::Ok<const char*>(std::vector{ 1, 2, 3, 4 });
        auto x_cmp = std::vector{ 1, 2, 3, 4 };
        assert(x.ok().value() == x_cmp);
        assert(std::get<0>(x).size() == 0); // moved
        
        auto y = C163q::Err<unsigned>("Err");
        assert(y.ok().has_value() == false);
    }
}

