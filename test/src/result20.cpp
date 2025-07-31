#include"../../src/rs/result20.hpp"
#include<cassert>
#include<cstddef>
#include<exception>
#include<format>
#include<initializer_list>
#include<iterator>
#include<iostream>
#include<numeric>
#include<optional>
#include<stdexcept>
#include<string>
#include<string_view>
#include<tuple>
#include<vector>

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
    {
        C163q::Result<std::vector<int>, const char*> src(std::vector{1, 2, 3, 4});
        auto dst = src.map_into<int>(0, [](std::vector<int> v) {
                return std::accumulate(std::begin(v), std::end(v), 0);
            });
        assert(dst == 10);
        assert(src.get<0>().size() == 0);

        C163q::Result<const char*, std::string> x(std::in_place_type<std::string>, "bar");
        auto y = x.map_into<std::string>("Error", [](std::string v) {
                v.push_back('z');
                return v;
            });
        assert(y == "Error");
        assert(x.get<1>() == "bar");
    }
    {
        size_t k = 21;
        
        auto x = C163q::Ok<std::exception, std::string_view>("foo");
        auto res = x.map<size_t>([k](auto e) { return k * 2; }, [] (auto v) { return v.size(); });
        assert(res == 3);

        auto y = C163q::Err<std::string_view>("bar");
        res = y.map<size_t>([k](auto e) { return k * 2; }, [] (auto v) { return v.size(); });
        assert(res == 42);
    }
    {
        size_t k = 21;
        
        auto x = C163q::Ok<std::exception, std::string>("foo");
        auto res = x.map_into<size_t>([k](auto e) { return k * 2; }, [] (auto v) { return v.size(); });
        assert(res == 3);
        assert(x.get<0>().size() == 0);

        auto y = C163q::Err<std::string_view>("bar");
        res = y.map<size_t>([k](auto e) { return k * 2; }, [] (auto v) { return v.size(); });
        assert(res == 42);
    }
    {
        auto x = C163q::Ok<const char*>(1);
        auto res_x = x.map_err<void*>([](auto e) { return (void*)e; });
        assert(res_x.get<0>() == 1);

        auto y = C163q::Err<const char*>(12);
        auto res_y = y.map_err<std::string>([](auto e) { return std::format("error code: {}", e); });
        assert(res_y.get<1>() == "error code: 12");
    }
    {
        auto x = C163q::Ok<const char*>(std::vector{ 1 });
        auto res_x = x.map_err_into<void*>([](auto e) { return (void*)e; });
        assert(res_x.get<0>() == std::vector{ 1 });
        assert(x.get<0>().size() == 0);

        auto y = C163q::Err<int, std::string>("out of range");
        auto res_y = y.map_err_into<std::string>([](std::string e) { return std::format("error message: {}", e); });
        assert(res_y.get<1>() == "error message: out of range");
        assert(y.get<1>().size() == 0);
    }
    std::cout << "PASS!" << std::endl;
}

