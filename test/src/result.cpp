#include"../../src/rs/result.hpp"
#include<cassert>
#include<exception>
#include<initializer_list>
#include<optional>
#include<print>
#include<stdexcept>
#include<string_view>
#include<tuple>
#include<vector>

class noise_type {
public:
    noise_type() { std::println("default construct!"); }
    noise_type(const noise_type&) { std::println("copy construct!"); }
    noise_type(noise_type&&) { std::println("move construct!"); }
    noise_type& operator=(const noise_type&) { std::println("copy assign!"); return *this; }
    noise_type& operator=(noise_type&&) { std::println("move assign!"); return *this; }
};

int main() {
    {
        auto x = C163q::Ok<const char*>(-3);
        assert(x.is_ok() == true);

        auto y = C163q::Err<int, const char*>("Some error message");
        assert(y.is_ok() == false);
    }
    {
        C163q::Result<unsigned, const char*> x(2u);
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
    {
        auto x = C163q::Ok<const char*>(1);
        assert(x.err().has_value() == false);

        auto y = C163q::Err<unsigned>("Err");
        assert(y.err().value()[0] == 'E');
    }
    {
        auto Ok = [](int val) { return C163q::Ok<const char*>(val); };
        std::vector<C163q::Result<int, const char*>> vec {
            Ok(1), Ok(2), Ok(5), C163q::Err<int>("Err"), Ok(10), Ok(20)
        };
        std::vector<double> ret;
        for (auto&& res : vec) {
            auto val = res.map<double>([](auto val) { return val * 1.5; });
            if (val.is_ok()) {
                ret.push_back(val.get<0>());
            } else {
                ret.push_back(0);
            }
            static_assert(noexcept(res.map<double>([](auto val) noexcept { return double(val); })));
        }
        std::vector<double> check{ 1 * 1.5, 2 * 1.5, 5 * 1.5, 0, 10 * 1.5, 20 * 1.5 };
        assert(ret == check);
    }
    {
        C163q::Result<std::vector<int>, const char*> src(std::vector{1, 2, 3, 4});
        auto dst = src.map_into<std::vector<int>>([](auto&& v) {
                for (auto&& e : v) {
                    e *= 2;
                }
                return v;
            });
        std::vector<int> check{ 2, 4, 6, 8 };
        assert(dst.get<0>() == check);
        assert(src.get<0>().size() == 0);
    }
    {
        auto x = C163q::Ok<std::exception, std::string>("foo");
        assert(x.map(42, [](auto&& str) { return str.size(); }) == 3);

        auto y = C163q::Err<std::string, const char*>("bar");
        assert(y.map(42, [](auto&& str) { return str.size(); }) == 42);
    }
    std::println("PASS!");
}

