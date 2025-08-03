#include"../../src/rs/result20.hpp"
#include"../../src/rs/panic.hpp"
#include"../../src/core/config.hpp"
#include<cassert>
#include<chrono>
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
#include<type_traits>
#include<utility>
#include<vector>

int main() {
#ifdef MY_CXX23
    C163q::enable_traceback = true;
#endif // MY_CXX23
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
            // 使用as_const保证值不会被移动！
            auto val = res.as_const().map<double>([](auto val) { return val * 1.5; });
            if (val.is_ok()) {
                ret.push_back(val.get<0>());
            } else {
                ret.push_back(0);
            }
            static_assert(noexcept(res.as_const().map<double>([](auto val) noexcept { return double(val); })));
        }
        std::vector<double> check{ 1 * 1.5, 2 * 1.5, 5 * 1.5, 0, 10 * 1.5, 20 * 1.5 };
        assert(ret == check);
    }
    {
        C163q::Result<std::vector<int>, const char*> src(std::vector{1, 2, 3, 4});
        auto dst = src.map<std::vector<int>>([](auto v) {
                for (auto&& e : v) {
                    e *= 2;
                }
                return v;
            });
        std::vector<int> check{ 2, 4, 6, 8 };
        assert(dst.get<0>() == check);
        assert(src.get<0>().size() == 0);   // 原始值被移动，因此大小为0
    }
    {
        const auto x = C163q::Ok<std::exception, std::string>("foo");
        assert(x.map(42, [](auto&& str) { return str.size(); }) == 3);  // x是const的，复制值

        const auto y = C163q::Err<std::string, const char*>("bar");
        assert(y.map(42, [](auto&& str) { return str.size(); }) == 42); // y是const的，复制值
    }
    {
        C163q::Result<std::vector<int>, const char*> src(std::vector{1, 2, 3, 4});
        auto dst = src.map<int>(0, [](std::vector<int> v) {
                return std::accumulate(std::begin(v), std::end(v), 0);
            });
        assert(dst == 10);
        assert(src.get<0>().size() == 0);   // 由于原始值被移动，因此大小为0

        C163q::Result<const char*, std::string> x(std::in_place_type<std::string>, "bar");
        auto y = x.map<std::string>("Error", [](std::string v) {
                v.push_back('z');
                return v;
            });
        assert(y == "Error");
        assert(x.get<1>() == "bar");
    }
    {
        size_t k = 21;
        
        const auto x = C163q::Ok<std::exception, std::string_view>("foo");
        auto res = x.map<size_t>([k](auto e) { return k * 2; }, [] (auto v) { return v.size(); });  // x是const的，复制值
        assert(res == 3);

        const auto y = C163q::Err<std::string_view>("bar");
        res = y.map<size_t>([k](auto e) { return k * 2; }, [] (auto v) { return v.size(); });   // y是const的，复制值
        assert(res == 42);
    }
    {
        size_t k = 21;
        
        auto x = C163q::Ok<std::exception, std::string>("foo");
        auto res = x.map<size_t>([k](auto e) { return k * 2; }, [] (auto v) { return v.size(); });  // x是非const的，移动值
        assert(res == 3);
        assert(x.get<0>().size() == 0);

        auto y = C163q::Err<std::string_view>("bar");
        res = y.map<size_t>([k](auto e) { return k * 2; }, [] (auto v) { return v.size(); });   // y是非const的，移动值
        assert(res == 42);
    }
    {
        const auto x = C163q::Ok<const char*>(1);
        auto res_x = x.map_err<void*>([](auto e) { return (void*)e; }); // x是const的，复制值
        assert(res_x.get<0>() == 1);

        auto y = C163q::Err<const char*>(12);
        auto res_y = y.as_const().map_err<std::string>([](auto e) { return std::format("error code: {}", e); });
        // 使用as_const()转换为常引用，复制值
        assert(res_y.get<1>() == "error code: 12");
    }
    {
        auto x = C163q::Ok<const char*>(std::vector{ 1 });
        auto res_x = x.map_err<void*>([](auto e) { return (void*)e; }); // x是非const的，移动值
        assert(res_x.get<0>() == std::vector{ 1 });
        assert(x.get<0>().size() == 0);

        auto y = C163q::Err<int, std::string>("out of range");
        auto res_y = y.map_err<std::string>([](std::string e) { return std::format("error message: {}", e); });
        // y是非const的，移动值
        assert(res_y.get<1>() == "error message: out of range");
        assert(y.get<1>().size() == 0);
    }
    {
        auto x = C163q::Ok<const char*>(1);
        int val = x.expect("Error");
        assert(val == 1);

        // auto y = C163q::Err<int>("code");
        // val = y.expect("Error");    // panics with `Error: code`
    }
    {
        auto x = C163q::Ok<const char*>(1);
        int val = x.unwrap();
        assert(val == 1);

        // auto y = C163q::Err<int>("code");
        // val = y.unwrap();   // panics with `code`
    }
    {
        std::string s1 = "123456";
        std::string s2 = "foo";
        auto f = [](const std::string& s) {
            int(*f)(const std::string&, size_t*, int) = std::stoi;
            return f(s, nullptr, 10);
        };

        auto val1 = C163q::result_helper<std::exception>::invoke(f, s1).unwrap_or_default();
        assert(val1 == 123456);

        auto val2 = C163q::result_helper<std::exception>::invoke(f, s2).unwrap_or_default();
        assert(val2 == 0);
    }
    {
        auto x = C163q::Err<const char*>("likely panic");
        const char* val = x.expect_err("Error");
        assert(std::string_view("likely panic") == val);

        // auto y = C163q::Ok<const char*>(std::chrono::day(7));
        // val = y.expect_err("Error");    // panics with `Error: 07`
    }
    {
        auto x = C163q::Err<const char*>("likely panic");
        std::string_view val = x.unwrap_err();
        assert(val == "likely panic");

        // auto y = C163q::Ok<const char*>(42);
        // val = y.unwrap_err();   // panics with `42`
    }
    {
        auto x = C163q::Ok<const char*>(2);
        auto y = C163q::Err<const char*>("late error");
        auto z = x.and_then(y);
        static_assert(std::is_same_v<C163q::Result<const char*, const char*>, decltype(z)>);
        assert(std::string_view("late error") == z.unwrap_err());

        auto a = C163q::Err<unsigned>("early error");
        auto b = C163q::Ok<const char*>("foo");
        assert(std::string_view("early error") == a.and_then(b).unwrap_err());

        auto c = C163q::Err<unsigned>("not a 2");
        auto d = C163q::Err<const char*>("late error");
        assert(std::string_view("not a 2") == c.and_then(d).unwrap_err());

        auto e = C163q::Ok<const char*>(2);
        auto f = C163q::Ok<const char*>("different result type");
        auto g = e.and_then(f);
        static_assert(std::is_same_v<C163q::Result<const char*, const char*>, decltype(g)>);
        assert(std::string_view("different result type") == g.unwrap());
    }
    {
        auto f = [](const std::string& s) {
            int(*f)(const std::string&, size_t*, int) = std::stoi;
            return C163q::result_helper<std::exception>::invoke(f, s, nullptr, 10)
                .map_err<std::string_view>([](auto) { return "overflowed"; });
        };

        using C163q::Ok;
        using C163q::Err;
        assert(Ok<std::string_view>("2").and_then<int>(f).unwrap() == 2);
        assert(Ok<std::string_view>("2147483648").and_then<int>(f).unwrap_err() == "overflowed");
        auto x = C163q::Err<std::string, std::string_view>("not a number").and_then<int>(f).unwrap_err();
        assert(x == "not a number");
    }
    {
        auto x = C163q::Ok<const char*>(2);
        auto y = C163q::Err<int, std::string_view>("late error");
        auto z = x.or_else(y);
        static_assert(std::is_same_v<C163q::Result<int, std::string_view>, decltype(z)>);
        assert(2 == z.unwrap());

        auto a = C163q::Err<int>("early error");
        auto b = C163q::Ok<std::string_view>(2);
        assert(2 == a.or_else(b).unwrap());

        auto c = C163q::Err<const char*>("not a 2");
        auto d = C163q::Err<const char*>("late error");
        assert(std::string_view("late error") == c.or_else(d).unwrap_err());

        auto e = C163q::Ok<const char*>(2);
        auto f = C163q::Ok<const char*>(100);
        assert(2 == e.or_else(f).unwrap());
    }
    {
        auto sq = [](unsigned x) { return C163q::Ok<unsigned>(x * x); };
        auto err = [](unsigned x) { return C163q::Err<unsigned>(x); };

        assert(2 == C163q::Ok<unsigned>(2u).or_else<unsigned>(sq).or_else<unsigned>(sq).unwrap());
        assert(2 == C163q::Ok<unsigned>(2u).or_else<unsigned>(err).or_else<unsigned>(sq).unwrap());
        assert(9 == C163q::Err<unsigned>(3u).or_else<unsigned>(sq).or_else<unsigned>(err).unwrap());
        assert(3 == C163q::Err<unsigned>(3u).or_else<unsigned>(err).or_else<unsigned>(err).unwrap_err());
    }
    {
        auto val = 2;
        auto x = C163q::Result<unsigned, const char*>(9u);
        assert(x.unwrap_or(val) == 9);

        auto y = C163q::Result<unsigned, const char*>("error");
        assert(y.unwrap_or(val) == 2);
    }
    {
        auto count = [](const std::string_view& str) { return unsigned(str.size()); };

        assert(C163q::Ok<const char*>(2u).unwrap_or(count) == 2);
        assert(C163q::Err<unsigned>("foo").unwrap_or(count) == 3);
    }
    {
        auto val = C163q::Result<std::string, std::vector<int>>("123");
        auto res = val.clone();
        (void) (val >= res.as_const().as_mut());
    }
    {
        using namespace C163q;
        auto good_result = Ok<int>(10);  // Result<int, int>
        auto bad_result = Err<int>(10);  // Result<int, int>
        assert(good_result.is_ok() && !good_result.is_err());
        assert(bad_result.is_err() && !bad_result.is_ok());

        good_result = good_result.map<int>([](auto i) { return i + 1; });   // 消耗本身并产生一个新的Result
        bad_result = bad_result.map_err<int>([](auto i) { return i - 1; });
        assert(good_result == Ok<int>(11));
        assert(bad_result == Err<int>(9));

        auto another_good = good_result.and_then<bool>([](auto i) { return Ok<int>(i == 11); }); // Result<bool, int>
        assert(another_good.as_const().unwrap() == true);   // 使用as_const()阻止移动自身值
        auto another_bad = bad_result.or_else<int>([](auto i) { return Ok<int>(i + 20); });
        assert(another_bad.as_ref().unwrap() == 29);    // 使用as_ref()产生一个指向保存值引用的Result
                                                        // 注意变量的生命周期！
        auto final_awesome_result = good_result.unwrap();
        assert(final_awesome_result);
    }
    {
        C163q::Result<void, std::exception> x;
        x.unwrap();
        x.get<0>();
    }
    std::cout << "PASS!" << std::endl;
}

