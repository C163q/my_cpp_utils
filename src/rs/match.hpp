/**
 * @file rs/match.hpp
 * @brief 在C++中实现类似rust中match关键字的效果
 */

#include <utility>
#include<variant>

namespace C163q {

    template<typename ...T>
    struct overload : T... {
        using T::operator()...;
    };

    template <typename ...T>
    overload(T...) -> overload<T...>;

    /**
     * @brief 一个类似于rust语言match表达式的函数.
     *
     * @tparam T    std::variant内可保有的元素的类型
     * @tparam F    可调用对象的类型
     *
     * @param v     一个std::variant类型的数据
     * @param func  多个可调用对象（下简称函数），
     *              在variant内保有的类型与函数参数的类型匹配时被调用
     *
     * 例如，对于一个std::variant<T, U, ...>（注意：内部的类型必须互不相同）
     * 以及多个函数std::function<void(T)>, std::function<void(U)>, ...
     * 若std::variant内保有类型T，则会调用std::function<void(T)>
     *
     * @example
     * ```cpp
     * std::variant<int, double, const char*> v { 1.0 };
     * std::string_view ret = C163q::match(v,
     *         [] (int i) { return "int"; },
     *         [] (double d) { return "double"; },
     *         [] (const char* p) { return "const char*"; }
     *         );
     * assert(ret == "double");
     * ```
     *
     * @warning 传入的函数func应当穷尽所有的可能性！
     * 以下是一个会导致编译错误的示例：
     * @example
     * ```cpp
     * std::variant<int, double, const char*> v { 1.0 };
     * std::string_view ret = C163q::match(v,
     *         [] (int i) { return "int"; },
     *         [] (double d) { return "double"; },
     *         );   // 没有类型const char*，因此会导致编译错误！
     * ```
     *
     * 可以使用泛型lambda表达式通配剩余类型。
     *
     * @example
     * ```cpp
     * std::variant<int, double, const char*> v { 1.0 };
     * std::string_view ret = C163q::match(v,
     *         [] (int i) { return "int"; },
     *         [] (auto d) { return "not int"; }   // 泛型lambda表达式
     *         );
     * assert(ret == "not int");
     * ```
     *
     */
    template <typename ...T, typename ...F>
    constexpr auto match(const std::variant<T...>& v, F&&... func) {
        return std::visit(overload(std::forward<F>(func)...), v);
    }

}





