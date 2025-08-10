/*!
 * @file rs/result20.hpp
 * @brief 在C++中实现rust当中的Result类
 *
 * 至少需要C++20
 *
 * @since Jul 17, 2025
 */

#ifndef C163Q_MY_CPP_UTILS_RS_RESULT20_HPP
#define C163Q_MY_CPP_UTILS_RS_RESULT20_HPP

#include"../core/config.hpp"
#ifndef MY_CXX20
    static_assert(false, "Require C++20!");
#else


// TODO: add `copied` method for Result<std::reference_wrapper<T>, E>

#include<algorithm>
#include<compare>
#include<concepts>
#include<cstddef>
#include<format>
#include<functional>
#include<optional>
#include<string_view>
#include<type_traits>
#include<utility>
#include<variant>
#include"option.hpp"
#include"panic.hpp"

namespace C163q {

    template<typename T>
        requires (std::is_object_v<T> && !std::is_array_v<T> &&
                 !std::is_same_v<std::remove_cv_t<T>, std::nullopt_t> &&
                 !std::is_same_v<std::remove_cv_t<T>, std::in_place_t>) ||
                  std::is_same_v<T, void>
    class Option;

    /**
     * @brief Rust当中的Result枚举类型，表示有可能成功（返回值）或者失败（返回异常）的类型。
     *
     * 见：https://doc.rust-lang.org/stable/std/result/enum.Result.html
     * 
     * @tparam T 保有的数据的类型
     * @tparam E 可能返回的错误的类型
     *
     * 若Result保有类型T，则代表其为Ok状态；若Result保有类型E，则代表其为Err状态。
     *
     * @example
     * ```
     * using namespace C163q;
     * auto good_result = Ok<int>(10);  // Result<int, int>
     * auto bad_result = Err<int>(10);  // Result<int, int>
     * assert(good_result.is_ok() && !good_result.is_err());
     * assert(bad_result.is_err() && !bad_result.is_ok());
     *
     * good_result = good_result.map<int>([](auto i) { return i + 1; });   // 消耗本身并产生一个新的Result
     * bad_result = bad_result.map_err<int>([](auto i) { return i - 1; });
     * assert(good_result == Ok<int>(11));
     * assert(bad_result == Err<int>(9));
     *
     * auto another_good = good_result.and_then<bool>([](auto i) { return Ok<int>(i == 11); }); // Result<bool, int>
     * assert(another_good.as_const().unwrap() == true);   // 使用as_const()阻止移动自身值
     * auto another_bad = bad_result.or_else<int>([](auto i) { return Ok<int>(i + 20); });
     * assert(another_bad.as_ref().unwrap() == 29);    // 使用as_ref()产生一个指向保存值引用的Result
     *                                                 // 注意变量的生命周期！
     * auto final_awesome_result = good_result.unwrap();
     * assert(final_awesome_result);
     * ```
     */
    template<typename T, typename E>
        requires (std::is_object_v<T> && std::is_object_v<E>) || std::is_same_v<T, void>
    class Result {
    private:
        using as_cref_t = Result<std::reference_wrapper<const T>, std::reference_wrapper<const E>>;
        using as_ref_t = Result<std::reference_wrapper<T>, std::reference_wrapper<E>>;


        template<typename U>
        struct my_type {
            template<typename V>
            static std::type_identity<typename V::type> test(V::type*) { return {}; }

            template<typename V>
            static std::type_identity<void> test(...) { return {}; }

            using type = decltype(test<U>(nullptr))::type;
        };

    public:
        using value_type = T;
        using error_type = E;

    public:
        /**
         * @brief 默认构造函数
         *
         * 要求类型T必须是可以默认构造的，即requires: is_default_constructible_v<T>。
         */
        constexpr Result() noexcept(std::is_nothrow_constructible_v<T>) : m_data() {}

        /**
         * @brief 使用T构造Ok所保有的类型T
         *
         * 要求T至少是可移动构造的，这会使Result处于Ok状态。
         *
         * @param value 要存储的值
         *
         * @tparam U 构造T时所使用的类型
         */
        template<typename U>
            requires std::constructible_from<T, U>
        constexpr explicit Result(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>)
            : m_data(std::forward<U>(value)) {}

        /**
         * @brief 使用E构造Err所保有的类型
         *
         * 要求E至少是可移动构造的，这会使Result处于Err状态。
         *
         * @param value 要存储的异常
         *
         * @tparam F 构造E时所使用的类型
         */
        template<typename F>
            requires std::constructible_from<E, F>
        constexpr explicit Result(F&& err) noexcept(std::is_nothrow_constructible_v<E, F>)
            : m_data(std::forward<F>(err)) {}

        /**
         * @brief 使用args构造Result类型
         *
         * 要求U可以使用args构造。如果U是T，Result会处于Ok状态；如果U是E，Result会处于Err状态。
         *
         * @tparam U 要构造的对象的类型，应当是类型T或E
         * @tparma Args 参数的类型
         * @param args 用于构造U的参数
         */
        template<typename U, typename ...Args>
            requires ((std::is_same_v<T, U> || std::is_same_v<E, U>) &&
                      std::is_constructible_v<U, Args...> && !std::is_same_v<T, E>)
        constexpr explicit Result(std::in_place_type_t<U>, Args&&... args)
            : m_data(std::in_place_type<U>, std::forward<Args>(args)...) {}


        template<size_t I, typename ...Args>
            requires ((I == 0 || I == 1) &&
                    std::is_constructible_v<std::conditional_t<I == 0, T, E>, Args...>)
        constexpr explicit Result(std::in_place_index_t<I>, Args&&... args)
            : m_data(std::in_place_index<I>, std::forward<Args>(args)...) {}

        constexpr Result(const Result&) = default;
        constexpr Result(Result&&) noexcept(std::is_nothrow_move_constructible_v<T> &&
                std::is_nothrow_move_constructible_v<E>) = default;
        constexpr Result& operator=(const Result&) = default;
        constexpr Result& operator=(Result&&) noexcept(std::is_nothrow_move_constructible_v<T> &&
                std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<E> &&
                std::is_nothrow_move_assignable_v<E>) = default;

        /**
         * @brief 检查Result是否处于Ok状态，即是否保有值的类型是T
         *
         * @return 处于Ok状态时返回true，否则false
         *
         * @example
         * ```cpp
         * auto x = C163q::Ok<const char*>(-3);
         * assert(x.is_ok() == true);
         *
         * auto y = C163q::Err<int, const char*>("Some error message");
         * assert(y.is_ok() == false);
         * ```
         */
        [[nodiscard]] constexpr bool is_ok() const noexcept {
            return !m_data.index();
        }

        /**
         * @brief 当Result处于Ok状态且其值满足谓词f时返回true，否则false
         *
         * @param f 参数为const T&且返回bool的谓词，
         *          Result内保有的数据的常引用会被传入。
         *
         * @tparam F 可调用对象的类型，应当接收const T&并返回bool
         *
         * @return 当Result处于Ok状态且其保有满足谓词f的值时返回true，否则false
         *
         * @example
         * ```cpp
         * C163q::Result<unsigned, const char*> x(2u);
         * assert(x.is_ok_and([](auto v) { return v > 1; }) == true);
         *
         * auto y = C163q::Ok<const char*>(0u);
         * assert(y.is_ok_and([](const unsigned& v) { return v > 1; }) == false);
         *
         * C163q::Result<unsigned, const char*> z("hey");
         * assert(z.is_ok_and([](auto v) { return v > 1; }) == false);
         * ```
         */
        template<typename F>
            requires std::predicate<F, const T&>
        [[nodiscard]] constexpr bool is_ok_and(F&& f)
            const noexcept(std::is_nothrow_invocable_v<F, const T&>) {
            return !m_data.index() && std::invoke(std::forward<F>(f), const_cast<const T&>(std::get<0>(m_data)));
        }

        /**
         * @brief 检查Result是否处于Err状态，即是否保有值的类型是E
         *
         * @return 处于Err状态时返回true，否则false
         *
         * @example
         * ```cpp
         * auto x = C163q::Ok<const char*>(-3);
         * assert(x.is_ok() == true);
         *
         * auto y = C163q::Err<int, const char*>("Some error message");
         * assert(y.is_ok() == false);
         * ```
         */
        [[nodiscard]] constexpr bool is_err() const noexcept {
            return !!m_data.index();
        }

        /**
         * @brief 当Result处于Err状态且其值满足谓词f时返回true，否则false
         *
         * @param f 参数为const E&且返回bool的谓词，
         *          Result内保有的数据的常引用会被传入。
         *
         * @tparam F 可调用对象的类型，应当接收const E&并返回bool
         *
         * @return 当Result处于Err状态且其保有满足谓词f的值时返回true，否则false
         *
         * @example
         * ```cpp
         * auto x = C163q::Err<unsigned, std::invalid_argument>("gets invalid argument");
         * assert(x.is_err_and([](const std::invalid_argument& e) {
         *             return std::string_view("gets invalid argument") == e.what();
         *             }) == true);
         *
         * C163q::Result<long, const char*> y("Error");
         * assert(y.is_err_and([](const char* p) { return p[0] == 'e'; }) == false);
         *
         * auto z = C163q::Ok<std::exception, unsigned>(1);
         * assert(z.is_err_and([](const std::exception&) { return true; }) == false);
         * ```
         */
        template<typename F>
            requires std::predicate<F, const E&>
        [[nodiscard]] constexpr bool is_err_and(F&& f)
            const noexcept(std::is_nothrow_invocable_v<F, const E&>) {
            return !!m_data.index() && std::invoke(std::forward<F>(f), const_cast<const E&>(std::get<1>(m_data)));
        }

        /**
         * @brief 访问Result内所保有元素
         *
         * @tparam U 要访问的元素的类型。若为T，尝试访问Ok时所保有的值
         *           若为E，尝试访问Err时所保有的值
         * @tparam I 要访问的元素的索引。若为0，尝试访问Ok时所保有的值
         *           若为1，尝试访问Err时所保有的值
         *
         * @panic 当访问错误的类型或索引时将导致panic
         */
        template<typename U>
            requires ((std::is_same_v<U, T> || std::is_same_v<U, E>) && !std::is_same_v<T, E>)
        [[nodiscard]] constexpr U& get() {
            try {
                return std::get<U>(m_data);
            } catch(...) {
                panic("Invaild access to Result");
            }
        }

        template<typename U>
            requires ((std::is_same_v<U, T> || std::is_same_v<U, E>) && !std::is_same_v<T, E>)
        [[nodiscard]] constexpr const U& get() const {
            try {
                return std::get<U>(m_data);
            } catch(...) {
                panic("Invaild access to Result");
            }
        }

        template<size_t I>
            requires (I == 0 || I == 1)
        [[nodiscard]] constexpr std::variant_alternative_t<I, std::variant<T, E>>& get() {
            try {
                return std::get<I>(m_data);
            } catch(...) {
                panic("Invaild access to Result");
            }
        }

        template<size_t I>
            requires (I == 0 || I == 1)
        [[nodiscard]] constexpr const std::variant_alternative_t<I, std::variant<T, E>>& get() const {
            try {
                return std::get<I>(m_data);
            } catch(...) {
                panic("Invaild access to Result");
            }
        }

        [[nodiscard]] constexpr const std::variant<T, E>& data() const noexcept {
            return m_data;
        }

        /**
         * @brief 将Result<T, E>转换为Option<T>
         *
         * 将自身转换为Option<T>，自身保有的值将会被移动，若为Err状态，则为空值。
         *
         * @return 一个Option<T>。在Ok状态下，移动构造一个有值的Option<T>，
         *         在Err状态下，返回一个无值的Option<T>
         *
         * @example
         * ```
         * auto x = C163q::Ok<const char*>(std::vector{ 1, 2, 3, 4 });
         * auto x_cmp = std::vector{ 1, 2, 3, 4 };
         * assert(x.ok().unwrap() == x_cmp);
         * assert(std::get<0>(x).size() == 0); // moved
         *
         * auto y = C163q::Err<unsigned>("Err");
         * assert(y.ok().is_some() == false);
         * ```
         */
        [[nodiscard]] constexpr Option<T> ok() noexcept(std::is_nothrow_move_constructible_v<T>) {
            if (is_err()) return std::nullopt;
            return Option<T>(std::in_place, std::move(get<0>()));
        }

        /**
         * @brief 将Result<T, E>转换为Option<E>
         *
         * 将自身转换为Option<E>，自身保有的值将会被移动，若为Ok状态，则为空值。
         *
         * @return 一个Option<E>。在Err状态下，移动构造一个有值的Option<E>，
         *         在Err状态下，返回一个无值的Option<E>
         *
         * @example
         * ```
         * auto x = C163q::Ok<const char*>(1);
         * assert(x.err().is_some() == false);
         *
         * auto y = C163q::Err<unsigned>("Err");
         * assert(y.err().unwrap()[0] == 'E');
         * ```
         */
        [[nodiscard]] constexpr Option<E> err() noexcept(std::is_nothrow_move_constructible_v<E>) {
            if (is_ok()) return std::nullopt;
            return Option<E>(std::in_place, std::move(get<1>()));
        }

        /**
         * @brief 使用可调用对象将Result<T, E>映射为Result<U, E>，在Ok时调用，而Err时什么都不做
         *
         * 在Ok状态下时，const T&会被传入并将返回值作为构造Result<U, E>的U类型的值
         * 在Err状态下，仅仅是复制构造E的值
         *
         * @warning 不同于非const，const下调用后原始值被复制
         *
         * @param func 用于映射的可调用对象，接收一个const T&并返回一个可以构造为U的类型
         *
         * @tparam U 返回的Result中，处于Ok状态时保有的值的类型，func的返回值应该能够转换为该类型
         * @tparam F 可调用对象的类型
         *
         * @example
         * ```
         * auto Ok = [](int val) { return C163q::Ok<const char*>(val); };
         * std::vector<C163q::Result<int, const char*>> vec {
         *     Ok(1), Ok(2), Ok(5), C163q::Err<int>("Err"), Ok(10), Ok(20)
         * };
         * std::vector<double> ret;
         * for (auto&& res : vec) {
         *     // 使用as_const保证值不会被移动！
         *     auto val = res.as_const().map<double>([](auto val) { return val * 1.5; });
         *     if (val.is_ok()) {
         *         ret.push_back(val.get<0>());
         *     } else {
         *         ret.push_back(0);
         *     }
         * }
         * std::vector<double> check{ 1 * 1.5, 2 * 1.5, 5 * 1.5, 0, 10 * 1.5, 20 * 1.5 };
         * assert(ret == check);
         * ```
         */
        template<typename U, typename F>
            requires (requires (F f, const T& t) {
                { std::invoke(f, t) } -> std::convertible_to<U>;
            } && std::is_copy_constructible_v<E>)
        [[nodiscard]] constexpr Result<U, E> map(F&& func) const
            noexcept(std::is_nothrow_constructible_v<U, std::invoke_result_t<F, const T&>> &&
                     std::is_nothrow_copy_constructible_v<E> && std::is_nothrow_invocable_v<F, const T&>) {
            if (is_err()) return Result<U, E>(std::in_place_index<1>, get<1>());
            return Result<U, E>(std::in_place_index<0>, std::invoke(std::forward<F>(func), get<0>()));
        }

        /**
         * @brief 如果处于Err状态，返回默认值，否则用保有值调用可调用对象
         *
         * @param default_value 当处于Err状态时返回的值
         * @param func          当处于Ok状态时所调用的可调用对象，参数类型为const T&，返回值类型为U
         *
         * @warning 不同于非const，const下调用后原始值被复制
         *
         * @tparam U 返回值类型，default_value应该为该类型，func的返回值应当可以转换为该类型
         * @tparam F 可调用对象的类型
         *
         * @return 类型为U，在Err状态下返回default_value，否则使用保有值调用可调用对象，并返回
         *
         * @example
         * ```
         * const auto x = C163q::Ok<std::exception, std::string>("foo");
         * assert(x.map(42, [](auto&& str) { return str.size(); }) == 3);  // x是const的，复制值
         *
         * const auto y = C163q::Err<std::string, const char*>("bar");
         * assert(y.map(42, [](auto&& str) { return str.size(); }) == 42); // y是const的，复制值
         * ```
         */
        template<typename U, typename F>
            requires (requires (F f, const T& t) {
                { std::invoke(f, t) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<U>)
        [[nodiscard]] constexpr U map(U default_value, F&& func) const
            noexcept(std::is_nothrow_constructible_v<U, std::invoke_result_t<F, const T&>> &&
                     std::is_nothrow_move_constructible_v<U> && std::is_nothrow_invocable_v<F, const T&>) {
            if (is_err()) return default_value;
            return std::invoke(std::forward<F>(func), get<0>());
        }

        /**
         * @brief 如果处于Err状态，用错误值调用callback，否则用保有值调用func
         *
         * @warning 不同于非const，const下调用后原始值被复制
         *
         * @param fallback 当处于Err状态时，所调用的可调用对象，参数类型为const E&，返回值类型可以转换为U
         * @param func     当处于Ok状态时，所调用的可调用对象，参数类型为const T&，返回值类型可以转换为U
         *
         * @tparma U 返回值类型，fallback和func的返回值应当可以转换为该类型
         * @tparam D 可调用对象fallback的类型
         * @tparam F 可调用对象func的类型
         *
         * @return 类型为U。在Err状态下，是使用错误值调用callback得到的；
         *         在Ok状态下，是使用保有值调用func得到的。
         *
         * @example
         * ```
         * size_t k = 21;
         * 
         * const auto x = C163q::Ok<std::exception, std::string_view>("foo");
         * auto res = x.map<size_t>([k] (auto e) { return k * 2; }, [] (auto v) { return v.size(); });  // x是const的，复制值
         * assert(res == 3);
         *
         * const auto y = C163q::Err<std::string_view>("bar");
         * res = y.map<size_t>([k] (auto e) { return k * 2; }, [] (auto v) { return v.size(); });   // y是const的，复制值
         * assert(res == 42);
         * ```
         */
        template<typename U, typename D, typename F>
            requires (requires (D f0, F f1, const E& e, const T& t) {
                { std::invoke(f0, e) } -> std::convertible_to<U>;
                { std::invoke(f1, t) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<U>)
        [[nodiscard]] constexpr U map(D&& fallback, F&& func) const
            noexcept(std::is_nothrow_constructible_v<U, std::invoke_result_t<D, const E&>> &&
                     std::is_nothrow_constructible_v<U, std::invoke_result_t<F, const T&>> &&
                     std::is_nothrow_move_constructible_v<U> && std::is_nothrow_invocable_v<D, const E&> &&
                     std::is_nothrow_invocable_v<F, const T&>) {
            if (is_err()) return std::invoke(std::forward<D>(fallback), get<1>());
            return std::invoke(std::forward<F>(func), get<0>());
        }

        /**
         * @brief 使用可调用对象将Result<T, E>映射为Result<T, F>，在Err时调用，而Ok时什么都不做
         *
         * 在Ok状态下，仅仅是复制构造T的值
         * 在Err状态下时，const E&会被传入并将返回值作为构造Result<T, F>的F类型的值
         *
         * @warning 不同于非const，const下调用后原始值被复制
         *
         * @param op 用于映射的可调用对象，接收一个const E&并返回一个可以构造为F的类型
         *
         * @tparam F 返回的Result中，处于Err状态时保有的值的类型，op的返回值应该能够转换为该类型
         * @tparam O 可调用对象的类型
         *
         * @example
         * ```
         * const auto x = C163q::Ok<const char*>(1);
         * auto res_x = x.map_err<void*>([] (auto e) { return (void*)e; }); // x是const的，复制值
         * assert(res_x.get<0>() == 1);
         *
         * auto y = C163q::Err<const char*>(12);
         * auto res_y = y.as_const().map_err<std::string>([] (auto e) { return std::format("error code: {}", e); });
         * // 使用as_const()转换为常引用，复制值
         * assert(res_y.get<1>() == "error code: 12");
         * ```
         */
        template<typename F, typename O>
            requires (requires (O op, const E& e) {
                { std::invoke(op, e) } -> std::convertible_to<F>;
            } && std::is_copy_constructible_v<T>)
        [[nodiscard]] constexpr Result<T, F> map_err(O&& op) const
            noexcept(std::is_nothrow_constructible_v<F, std::invoke_result_t<O, const E&>> &&
                     std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_invocable_v<O, const E&>) {
            if (is_ok()) return Result<T, F>(std::in_place_index<0>, get<0>());
            return Result<T, F>(std::in_place_index<1>, std::invoke(std::forward<O>(op), get<1>()));
        }

        /**
         * @brief 使用可调用对象将Result<T, E>映射为Result<U, E>，在Ok时调用，而Err时什么都不做
         *
         * 在Ok状态下时，T&&会被移动传入并将返回值作为构造Result<U, E>的U类型的值
         * 在Err状态下，仅仅是移动构造E的值
         *
         * @warning 不同于const，非const下调用后原始值被移动
         *
         * @param 用于映射的可调用对象，接收一个T并返回一个可以构造为U的类型
         * 
         * @tparam U 返回的Result中，处于Ok状态时保有的值的类型，func的返回值应该能够转换为该类型
         * @tparam F 可调用对象的类型                                                                                             @example
         * 
         * @example
         * ```
         * C163q::Result<std::vector<int>, const char*> src(std::vector{1, 2, 3, 4});
         * auto dst = src.map<std::vector<int>>([](auto v) {
         *         for (auto&& e : v) {
         *             e *= 2;
         *         }
         *         return v;
         *     });
         * std::vector<int> check{ 2, 4, 6, 8 };
         * assert(dst.get<0>() == check);
         * assert(src.get<0>().size() == 0);   // 原始值被移动，因此大小为0
         * ```
         */
        template<typename U, typename F>
            requires (requires (F f, T t) {
                { std::invoke(f, std::move(t)) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<E> && std::is_move_constructible_v<T>)
        [[nodiscard]] constexpr Result<U, E> map(F&& func)
            noexcept(std::is_nothrow_constructible_v<U, std::invoke_result_t<F, T>> &&
                     std::is_nothrow_move_constructible_v<E> && std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_invocable_v<F, T>) {
            if (is_err()) return Result<U, E>(std::in_place_index<1>, std::move(get<1>()));
            return Result<U, E>(std::in_place_index<0>, std::invoke(
                            std::forward<F>(func), std::move(get<0>())));
        }

        /**
         * @brief 如果处于Err状态，返回默认值，否则移动保有值调用可调用对象
         *
         * @warning 不同于const，非const下调用后原始值被移动
         *
         * @param default_value 当处于Err状态时返回的值
         * @param func          当处于Ok状态时所调用的可调用对象，参数类型为T，返回值类型为U
         *
         * @tparam U 返回值类型，default_value应该为该类型，func的返回值应当可以转换为该类型
         * @tparam F 可调用对象的类型
         *
         * @return 类型为U，在Err状态下返回default_value，否则移动保有值调用可调用对象，并返回
         *
         * @example
         * ```
         * C163q::Result<std::vector<int>, const char*> src(std::vector{1, 2, 3, 4});
         * auto dst = src.map<int>(0, [](std::vector<int> v) {
         *         return std::accumulate(std::begin(v), std::end(v), 0);
         *     });
         * assert(dst == 10);
         * assert(src.get<0>().size() == 0);   // 由于原始值被移动，因此大小为0
         *
         * C163q::Result<const char*, std::string> x(std::in_place_type<std::string>, "bar");
         * auto y = x.map<std::string>("Error", [](std::string v) {
         *         v.push_back('z');
         *         return v;
         *     });
         * assert(y == "Error");
         * assert(x.get<1>() == "bar");
         * ```
         */
        template<typename U, typename F>
            requires (requires (F f, T t) {
                { std::invoke(f, std::move(t)) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<U> && std::is_move_constructible_v<T>)
        [[nodiscard]] constexpr U map(U default_value, F&& func)
            noexcept(std::is_nothrow_constructible_v<U, std::invoke_result_t<F, T>> &&
                     std::is_nothrow_move_constructible_v<U> && std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_invocable_v<F, T>) {
            if (is_err()) return default_value;
            return std::invoke(std::forward<F>(func), std::move(get<0>()));
        }

        /**
         * @brief 如果处于Err状态，移动错误值调用callback，否则移动保有值调用func
         *
         * @warning 不同于const，非const下调用后原始值被移动
         *
         * @param fallback 当处于Err状态时，所调用的可调用对象，参数类型为E，返回值类型可以转换为U
         * @param func     当处于Ok状态时，所调用的可调用对象，参数类型为T，返回值类型可以转换为U
         *
         * @tparma U 返回值类型，fallback和func的返回值应当可以转换为该类型
         * @tparam D 可调用对象fallback的类型
         * @tparam F 可调用对象func的类型
         *
         * @return 类型为U。在Err状态下，是移动错误值调用callback得到的；
         *         在Ok状态下，是移动保有值调用func得到的。
         *
         * @example
         * ```
         * size_t k = 21;
         * 
         * auto x = C163q::Ok<std::exception, std::string>("foo");
         * auto res = x.map<size_t>([k] (auto e) { return k * 2; }, [] (auto v) { return v.size(); });  // x是非const的，移动值
         * assert(res == 3);
         * assert(x.get<0>().size() == 0);
         *
         * auto y = C163q::Err<std::string_view>("bar");
         * res = y.map<size_t>([k] (auto e) { return k * 2; }, [] (auto v) { return v.size(); });   // y是非const的，移动值
         * assert(res == 42);
         * ```
         */
        template<typename U, typename D, typename F>
            requires (requires (D f0, F f1, E e, T t) {
                { std::invoke(f0, std::move(e)) } -> std::convertible_to<U>;
                { std::invoke(f1, std::move(t)) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<U> && std::is_move_constructible_v<T> &&
                 std::is_move_constructible_v<E>)
        [[nodiscard]] constexpr U map(D&& fallback, F&& func)
            noexcept (std::is_nothrow_constructible_v<U, std::invoke_result_t<D, E>> &&
                      std::is_nothrow_constructible_v<U, std::invoke_result_t<F, T>> &&
                      std::is_nothrow_move_constructible_v<U> && std::is_nothrow_move_constructible_v<T> &&
                      std::is_nothrow_move_constructible_v<E> && std::is_nothrow_invocable_v<D, E> &&
                      std::is_nothrow_invocable_v<F, T>) {
            if (is_err()) return std::invoke(std::forward<D>(fallback), std::move(get<1>()));
            return std::invoke(std::forward<F>(func), std::move(get<0>()));
        }

        /**
         * @brief 使用可调用对象将Result<T, E>映射为Result<T, F>，在Err时调用，而Ok时什么都不做
         *
         * 在Ok状态下，仅仅是移动构造T的值
         * 在Err状态下时，E&&会被移动传入并将返回值作为构造Result<T, F>的F类型的值
         *
         * @warning 不同于const，非const下调用后原始值被移动
         *
         * @param op 用于映射的可调用对象，接收一个E并返回一个可以构造为F的类型
         *
         * @tparam F 返回的Result中，处于Err状态时保有的值的类型，op的返回值应该能够转换为该类型
         * @tparam O 可调用对象的类型
         *
         * @example
         * ```
         * auto x = C163q::Ok<const char*>(std::vector{ 1 });
         * auto res_x = x.map_err<void*>([](auto e) { return (void*)e; }); // x是非const的，移动值
         * assert(res_x.get<0>() == std::vector{ 1 });
         * assert(x.get<0>().size() == 0);
         *
         * auto y = C163q::Err<int, std::string>("out of range");
         * auto res_y = y.map_err<std::string>([](std::string e) { return std::format("error message: {}", e); });
         * // y是非const的，移动值
         * assert(res_y.get<1>() == "error message: out of range");
         * assert(y.get<1>().size() == 0);
         * ```
         */
        template<typename F, typename O>
            requires (requires (O op, E e) {
                { std::invoke(op, std::move(e)) } -> std::convertible_to<F>;
            } && std::is_move_constructible_v<T> && std::is_move_constructible_v<E>)
        [[nodiscard]] constexpr Result<T, F> map_err(O&& op)
            noexcept(std::is_nothrow_constructible_v<F, std::invoke_result_t<O, E>> &&
                     std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_constructible_v<E> &&
                     std::is_nothrow_invocable_v<O, E>) {
            if (is_ok()) return Result<T, F>(std::in_place_index<0>, std::move(get<0>()));
            return Result<T, F>(std::in_place_index<1>, std::invoke(
                            std::forward<O>(op), std::move(get<1>())));
        }

        /**
         * @brief 返回存储的Ok值。
         *        非const时移动存储值，const时复制存储值。
         *
         * @panic 当值为Err时panic，panic信息包括传递的消息和Err的值。
         *
         * @param msg panic时的消息。
         *
         * @example
         * ```
         * auto x = C163q::Ok<const char*>(1);
         * int val = x.expect("Error");
         * assert(val == 1);
         *
         * auto y = C163q::Err<int>("code");
         * val = y.expect("Error");    // panics with `Error: code`
         * ```
         */
        [[nodiscard]] constexpr T expect(const std::string_view& msg) {
            if (is_ok()) return std::move(get<0>());
            call_panic_with_TE_uncheck<1>(msg, ": ");
        }

        [[nodiscard]] constexpr T expect(const std::string_view& msg) const {
            if (is_ok()) return get<0>();
            call_panic_with_TE_uncheck<1>(msg, ": ");
        }

        /**
         * @brief 返回存储的Ok值。
         *        非const时移动存储值，const时复制存储值。
         *
         * @panic 当值为Err时panic，panic信息为Err的值。
         *
         * @example
         * ```
         * auto x = C163q::Ok<const char*>(1);
         * int val = x.unwrap();
         * assert(val == 1);
         *
         * auto y = C163q::Err<int>("code");
         * val = y.unwrap();   // panics with `code`
         * ```
         */
        [[nodiscard]] constexpr T unwrap() {
            if (is_ok()) return std::move(get<0>());
            call_panic_with_TE_uncheck<1>("", "");
        }

        [[nodiscard]] constexpr T unwrap() const {
            if (is_ok()) return get<0>();
            call_panic_with_TE_uncheck<1>("", "");
        }

        /**
         * @brief 返回存储的Ok值。
         *        当处于Ok状态时，返回存储值；处于Err状态时，返回类型的默认值。要求T可以默认构造。
         *        处于Ok状态时，非const时移动存储值，const时复制存储值。
         *
         * @example
         * ```
         * std::string s1 = "123456";
         * std::string s2 = "foo";
         * auto f = [](const std::string& s) {
         *     int(*f)(const std::string&, size_t*, int) = std::stoi;
         *     return f(s, nullptr, 10);
         * };
         *
         * auto val1 = C163q::result_helper<std::exception>::invoke(f, s1).unwrap_or_default();
         * assert(val1 == 123456);
         *
         * auto val2 = C163q::result_helper<std::exception>::invoke(f, s2).unwrap_or_default();
         * assert(val2 == 0);
         * ```
         */
        [[nodiscard]] constexpr T unwrap_or_default()
            noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_default_constructible_v<T>) {
            if (is_ok()) return std::move(get<0>());
            return T();
        }

        [[nodiscard]] constexpr T unwrap_or_default() const
            noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_default_constructible_v<T>) {
            if (is_ok()) return get<0>();
            return T();
        }

        /**
         * @brief 返回存储的Err值。
         *        非const时移动存储值，const时复制存储值。
         *
         * @panic 当值为Ok时panic，panic信息包括传递的消息和Ok的值。
         *
         * @param msg panic时的消息。
         *
         * @example
         * ```
         * auto x = C163q::Err<const char*>("likely panic");
         * const char* val = x.expect_err("Error");
         * assert(std::string_view("likely panic") == val);
         *
         * auto y = C163q::Ok<const char*>(std::chrono::day(7));
         * val = y.expect_err("Error");    // panics with `Error: 07`
         * ```
         */
        [[nodiscard]] constexpr E expect_err(const std::string_view& msg) {
            if (is_err()) return std::move(get<1>());
            call_panic_with_TE_uncheck<0>(msg, ": ");
        }

        [[nodiscard]] constexpr E expect_err(const std::string_view& msg) const {
            if (is_err()) return get<1>();
            call_panic_with_TE_uncheck<0>(msg, ": ");
        }

        /**
         * @brief 返回存储的Err值。
         *        非const时移动存储值，const时复制存储值。
         *
         * @panic 当值为Ok时panic，panic信息为Ok的值。
         *
         * @example
         * ```
         * auto x = C163q::Err<const char*>("likely panic");
         * std::string_view val = x.unwrap_err();
         * assert(val == "likely panic");
         *
         * auto y = C163q::Ok<const char*>(42);
         * val = y.unwrap_err();   // panics with `42`
         * ```
         */
        [[nodiscard]] constexpr E unwrap_err() {
            if (is_err()) return std::move(get<1>());
            call_panic_with_TE_uncheck<0>("", "");
        }

        [[nodiscard]] constexpr E unwrap_err() const {
            if (is_err()) return get<1>();
            call_panic_with_TE_uncheck<0>("", "");
        }

        /**
         * @brief 如果自身是Ok，则返回res；否则返回自身的Err值。
         *        非const时移动存储值，const时复制存储值。
         *
         * @tparam U 要返回的Result的Ok值的类型
         *
         * @param res 另一个Result，当自身为Ok时返回该值。
         *
         * @example
         * ```
         * auto x = C163q::Ok<const char*>(2);
         * auto y = C163q::Err<const char*>("late error");
         * auto z = x.and_then(y);
         * static_assert(std::is_same_v<C163q::Result<const char*, const char*>, decltype(z)>);
         * assert(std::string_view("late error") == z.unwrap_err());
         *
         * auto a = C163q::Err<unsigned>("early error");
         * auto b = C163q::Ok<const char*>("foo");
         * assert(std::string_view("early error") == a.and_then(b).unwrap_err());
         *
         * auto c = C163q::Err<unsigned>("not a 2");
         * auto d = C163q::Err<const char*>("late error");
         * assert(std::string_view("not a 2") == c.and_then(d).unwrap_err());
         *
         * auto e = C163q::Ok<const char*>(2);
         * auto f = C163q::Ok<const char*>("different result type");
         * auto g = e.and_then(f);
         * static_assert(std::is_same_v<C163q::Result<const char*, const char*>, decltype(g)>);
         * assert(std::string_view("different result type") == g.unwrap());
         * ```
         */
        template<typename U>
        [[nodiscard]] constexpr Result<U, E> and_then(Result<U, E> res)
        noexcept(std::is_nothrow_move_constructible_v<Result<U, E>> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::in_place_index_t<1>, E&&> &&
                 std::is_nothrow_move_constructible_v<E>) {
            if (is_ok()) return res;
            return Result<U, E>(std::in_place_index<1>, std::move(get<1>()));
        }

        template<typename U>
        [[nodiscard]] constexpr Result<U, E> and_then(Result<U, E> res) const
        noexcept(std::is_nothrow_move_constructible_v<Result<U, E>> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::in_place_index_t<1>, const E&> &&
                 std::is_nothrow_copy_constructible_v<E>) {
            if (is_ok()) return res;
            return Result<U, E>(std::in_place_index<1>, get<1>());
        }

        /**
         * @brief 如果自身为Ok则调用op，否则返回自身的Err值
         *        非const时移动存储值，const时复制存储值。
         *
         * @tparam U 返回的Result的Ok值的类型，可以与T的类型不同
         * @tparam F 可对用对象的类型，参数为E&&（或const E&，const时），返回类型为Result<U, E>
         *
         * @param op 可调用对象，参数类型见{@tparam F}
         *
         * @result 当自身为Ok时，返回值为用Err值调用op的结果。
         *         当自身为Err时，返回值为用自身Err值构造Result的结果。
         *
         * @example
         * ```
         * auto f = [](const std::string& s) {
         *     int(*f)(const std::string&, size_t*, int) = std::stoi;
         *     return C163q::result_helper<std::exception>::invoke(f, s, nullptr, 10)
         *         .map_err<std::string_view>([](auto) { return "overflowed"; });
         * };
         *
         * using C163q::Ok;
         * using C163q::Err;
         * assert(Ok<std::string_view>("2").and_then<int>(f).unwrap() == 2);
         * assert(Ok<std::string_view>("2147483648").and_then<int>(f).unwrap_err() == "overflowed");
         * auto x = C163q::Err<std::string, std::string_view>("not a number").and_then<int>(f).unwrap_err();
         * assert(x == "not a number");
         * ```
         */
        template<typename U, typename F>
            requires (requires (F f, T t) {
                { std::invoke(f, std::move(t)) } -> std::convertible_to<Result<U, E>>;
            })
        [[nodiscard]] constexpr Result<U, E> and_then(F&& op)
        noexcept(std::is_nothrow_invocable_v<F, T&&> && std::is_nothrow_move_constructible_v<T> &&
                 std::is_nothrow_move_constructible_v<E> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::in_place_index_t<1>, E&&> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::invoke_result_t<F, T&&>>) {
            if (is_ok()) return std::invoke(std::forward<F>(op), std::move(get<0>()));
            return Result<U, E>(std::in_place_index<1>, std::move(get<1>()));
        }

        template<typename U, typename F>
            requires (requires (F f, const T& t) {
                { std::invoke(f, t) } -> std::convertible_to<Result<U, E>>;
            })
        [[nodiscard]] constexpr Result<U, E> and_then(F&& op) const
        noexcept(std::is_nothrow_invocable_v<F, const T&> && std::is_nothrow_copy_constructible_v<E> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::in_place_index_t<1>, const E&> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::invoke_result_t<F, const T&>>) {
            if (is_ok()) return std::invoke(std::forward<F>(op), get<0>());
            return Result<U, E>(std::in_place_index<1>, get<1>());
        }

        /**
         * @brief 如果自身是Err，则返回res；否则返回自身的Ok值。
         *        非const时移动存储值，const时复制存储值。
         *
         * @tparam F 要返回的Result的Err值的类型
         *
         * @param res 另一个Result，当自身为Err时返回该值。
         *
         * @example
         * ```
         * auto x = C163q::Ok<const char*>(2);
         * auto y = C163q::Err<int, std::string_view>("late error");
         * auto z = x.or_else(y);
         * static_assert(std::is_same_v<C163q::Result<int, std::string_view>, decltype(z)>);
         * assert(2 == z.unwrap());
         *
         * auto a = C163q::Err<int>("early error");
         * auto b = C163q::Ok<std::string_view>(2);
         * assert(2 == a.or_else(b).unwrap());
         *
         * auto c = C163q::Err<const char*>("not a 2");
         * auto d = C163q::Err<const char*>("late error");
         * assert(std::string_view("late error") == c.or_else(d).unwrap_err());
         *
         * auto e = C163q::Ok<const char*>(2);
         * auto f = C163q::Ok<const char*>(100);
         * assert(2 == e.or_else(f).unwrap());
         * ```
         */
        template<typename F>
        [[nodiscard]] constexpr Result<T, F> or_else(Result<T, F> res)
        noexcept(std::is_nothrow_move_constructible_v<Result<T, F>> &&
                 std::is_nothrow_constructible_v<Result<T, F>, std::in_place_index_t<0>, T&&> &&
                 std::is_nothrow_move_constructible_v<T>) {
            if (is_err()) return res;
            return Result<T, F>(std::in_place_index<0>, std::move(get<0>()));
        }

        template<typename F>
        [[nodiscard]] constexpr Result<T, F> or_else(Result<T, F> res) const
        noexcept(std::is_nothrow_move_constructible_v<Result<T, F>> &&
                 std::is_nothrow_constructible_v<Result<T, F>, std::in_place_index_t<0>, const T&> &&
                 std::is_nothrow_copy_constructible_v<T>) {
            if (is_err()) return res;
            return Result<T, F>(std::in_place_index<0>, get<0>());
        }

        /**
         * @brief 如果自身为Err则调用op，否则返回自身的Ok值
         *        非const时移动存储值，const时复制存储值。
         *
         * @tparam F 返回的Result的Err值的类型，可以与E的类型不同
         * @tparam O 可对用对象的类型，参数为T&&（或const T&，const时），返回类型为Result<T, F>
         *
         * @param op 可调用对象，参数类型见{@tparam O}
         *
         * @result 当自身为Ok时，返回值为用自身Ok值构造Result的结果。
         *         当自身为Err时，返回值为用Err值调用op的结果。
         *
         * @example
         * ```
         * auto sq = [](unsigned x) { return C163q::Ok<unsigned>(x * x); };
         * auto err = [](unsigned x) { return C163q::Err<unsigned>(x); };
         *
         * assert(2 == C163q::Ok<unsigned>(2u).or_else<unsigned>(sq).or_else<unsigned>(sq).unwrap());
         * assert(2 == C163q::Ok<unsigned>(2u).or_else<unsigned>(err).or_else<unsigned>(sq).unwrap());
         * assert(9 == C163q::Err<unsigned>(3u).or_else<unsigned>(sq).or_else<unsigned>(err).unwrap());
         * assert(3 == C163q::Err<unsigned>(3u).or_else<unsigned>(err).or_else<unsigned>(err).unwrap_err());
         * ```
         */
        template<typename F, typename O>
            requires (requires (O f, E e) {
                { std::invoke(f, std::move(e)) } -> std::convertible_to<Result<T, F>>;
            })
        [[nodiscard]] constexpr Result<T, F> or_else(O&& op)
        noexcept(std::is_nothrow_invocable_v<O, E&&> && std::is_nothrow_move_constructible_v<E> &&
                 std::is_nothrow_move_constructible_v<T> &&
                 std::is_nothrow_constructible_v<Result<T, F>, std::in_place_index_t<0>, T&&> &&
                 std::is_nothrow_constructible_v<Result<T, E>, std::invoke_result_t<O, E&&>>) {
            if (is_err()) return std::invoke(std::forward<O>(op), std::move(get<1>()));
            return Result<T, F>(std::in_place_index<0>, std::move(get<0>()));
        }

        template<typename F, typename O>
            requires (requires (O f, const E& e) {
                { std::invoke(f, e) } -> std::convertible_to<Result<T, F>>;
            })
        [[nodiscard]] constexpr Result<T, F> or_else(O&& op) const
        noexcept(std::is_nothrow_invocable_v<O, const E&> && std::is_nothrow_copy_constructible_v<T> &&
                 std::is_nothrow_constructible_v<Result<T, F>, std::in_place_index_t<0>, const T&> &&
                 std::is_nothrow_constructible_v<Result<T, F>, std::invoke_result_t<O, const E&>>) {
            if (is_err()) return std::invoke(std::forward<O>(op), get<1>());
            return Result<T, F>(std::in_place_index<0>, get<0>());
        }

        /**
         * @brief 若为Ok状态则返回存储值，否则返回所给值。
         *        非const时移动存储值，const时复制存储值。
         *
         * @param default_value 当Err状态时，返回该值。
         *
         * @example
         * ```
         * auto val = 2;
         * auto x = C163q::Result<unsigned, const char*>(9u);
         * assert(x.unwrap_or(val) == 9);
         *
         * auto y = C163q::Result<unsigned, const char*>("error");
         * assert(y.unwrap_or(val) == 2);
         * ```
         */
        [[nodiscard]] constexpr T unwrap_or(T default_value)
        noexcept(std::is_nothrow_move_constructible_v<T>) {
            if (is_ok()) return std::move(get<0>());
            return default_value;
        }

        [[nodiscard]] constexpr T unwrap_or(T default_value) const
        noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_copy_constructible_v<T>) {
            if (is_ok()) return get<0>();
            return default_value;
        }

        /**
         * @brief 若为Ok状态则返回存储值，否则使用Err值调用可调用对象并返回。
         *        非const时移动存储值，const时复制存储值。
         *
         * @tparam F 可调用对象的类型，参数为E&&（或const E&，const时），返回类型可以转换为T类型。
         *
         * @param op 可调用对象
         *
         * @example
         * ```
         * auto count = [](const std::string_view& str) { return unsigned(str.size()); };
         *
         * assert(C163q::Ok<const char*>(2u).unwrap_or(count) == 2);
         * assert(C163q::Err<unsigned>("foo").unwrap_or(count) == 3);
         * ```
         */
        template<typename F>
            requires requires (F f, E e) {
                { std::invoke(f, std::move(e)) } -> std::convertible_to<T>;
            }
        [[nodiscard]] constexpr T unwrap_or(F&& op)
        noexcept(std::is_nothrow_invocable_v<F, E&&> && std::is_nothrow_move_constructible_v<T> &&
                 std::is_nothrow_move_constructible_v<E> &&
                 std::is_nothrow_constructible_v<T, std::invoke_result_t<F, E&&>>) {
            if (is_ok()) return std::move(get<0>());
            return std::invoke(std::forward<F>(op), std::move(get<1>()));
        }

        template<typename F>
            requires requires (F f, const E& e) {
                { std::invoke(f, e) } -> std::convertible_to<T>;
            }
        [[nodiscard]] constexpr T unwrap_or(F&& op) const
        noexcept(std::is_nothrow_invocable_v<F, const E&> && std::is_nothrow_copy_constructible_v<T> &&
                 std::is_nothrow_constructible_v<T, std::invoke_result_t<F, const E&>>) {
            if (is_ok()) return get<0>();
            return std::invoke(std::forward<F>(op), get<1>());
        }

        /**
         * @brief 返回值的拷贝
         */
        [[nodiscard]] constexpr Result<T, E> clone() const noexcept(std::is_nothrow_copy_constructible_v<Result<T, E>>) {
            return *this;
        }

        template<typename U = my_type<T>::type>
            requires std::is_copy_constructible_v<T> && std::same_as<T, std::reference_wrapper<U>> &&
                     std::is_move_constructible_v<E>
        [[nodiscard]] constexpr Result<U, E> copied() {
            if (is_err()) return Result<U, E>(std::in_place_index<1>, std::move(get<1>()));
            return Result<U, E>(std::in_place_index<0>, get<0>().get());
        }

        template<typename U = my_type<T>::type>
            requires std::is_copy_constructible_v<T> && std::same_as<T, std::reference_wrapper<U>> &&
                     std::is_copy_constructible_v<E>
        [[nodiscard]] constexpr Result<U, E> copied() const {
            if (is_err()) return Result<U, E>(std::in_place_index<1>, get<1>());
            return Result<U, E>(std::in_place_index<0>, get<0>().get());
        }

        /**
         * @brief 使用other进行赋值
         */
        template<typename U, typename F>
        Result<T, E>& assign(const Result<U, F>& other) {
            if (other.is_ok()) m_data.emplace<0>(other.template get<0>());
            else m_data.emplace<1>(other.template get<1>());
            return *this;
        }

        template<typename U, typename F>
        Result<T, E>& assign(Result<U, F>&& other) {
            if (other.is_ok()) m_data.emplace<0>(std::move(other.template get<0>()));
            else m_data.emplace<1>(std::move(other.template get<1>()));
            return *this;
        }


        [[nodiscard]] constexpr const Result<T, E>& as_const() const noexcept {
            return *this;
        }

        [[nodiscard]] constexpr Result<T, E>& as_mut() const noexcept {
            return const_cast<Result<T, E>&>(*this);
        }
        
        /**
         * @brief 将Result<T, E>转换为Result<&T, &E>。
         *        产生一个新的Result，包含对原始值的引用。
         *
         * @warning 注意保存元素的生命周期！
         */
        [[nodiscard]] constexpr as_cref_t as_ref() const noexcept {
            if (is_ok()) return as_cref_t(std::in_place_index<0>, std::cref(get<0>()));
            return as_cref_t(std::in_place_index<1>, std::cref(get<1>()));
        }

        [[nodiscard]] constexpr as_ref_t as_ref() noexcept {
            if (is_ok()) return as_ref_t(std::in_place_index<0>, std::ref(get<0>()));
            return as_ref_t(std::in_place_index<1>, std::ref(get<1>()));
        }

    private:

        /**
         * @brief 调用panic，同时，若类型是formattable时，打印其值
         */
        template<size_t I>
            requires (I == 0 || I == 1)
        [[noreturn]] constexpr void call_panic_with_TE_uncheck(const std::string_view& msg, const std::string_view& sep) const {
            using U = std::conditional_t<I == 0, T, E>;
            if constexpr (
#ifdef MY_CXX23
                std::formattable<U, char>
#else // C++23 ^^^ /  vvv C++20
                requires (std::formatter<U>& f, const std::formatter<U>& cf, U&& t,
                    std::format_parse_context& p, std::format_context& fc) {
                    { f.parse(p) };
                    { cf.format(t, fc) };
                }
#endif
            ) {
                auto&& str = std::format("{}{}{}", msg, sep, get<I>());
                panic(str);
            } else {
                panic(msg);
            }
        }

    private:
        std::variant<T, E> m_data;
    };


    /**
     * @brief Result的Ok值为void类型的特化，API基本保持不变。
     *
     * @tparam E 可能返回的错误的类型
     */
    template<typename E>
        requires std::is_object_v<E>
    class Result<void, E> {
    private:
        using as_cref_t = Result<void, std::reference_wrapper<const E>>;
        using as_ref_t = Result<void, std::reference_wrapper<E>>;
        
        template<size_t I>
        using alternative_t = std::conditional_t<I == 0, void,
            std::variant_alternative_t<I, std::variant<std::monostate, E>>>;

        template<size_t I>
        using alternative_ref_t = std::conditional_t<I == 0, void,
            std::variant_alternative_t<I, std::variant<std::monostate, E>>&>;

        template<typename U>
        using ref_t = std::conditional_t<std::is_same_v<U, void>, void, U&>;

    public:
        constexpr Result() noexcept : m_data() {}

        template<typename F>
            requires std::constructible_from<E, F>
        constexpr explicit Result(F&& err) noexcept(std::is_nothrow_constructible_v<E, F>)
            : m_data(std::forward<F>(err)) {}

        template<typename U = E, typename ...Args>
            requires (std::is_same_v<E, U> && std::is_constructible_v<U, Args...> &&
                     !std::is_same_v<std::monostate, E>)
        constexpr explicit Result(std::in_place_type_t<U>, Args&&... args)
            :m_data(std::in_place_type<U>, std::forward<Args>(args)...) {}

        constexpr explicit Result<void>(std::in_place_type_t<void>) : m_data() {}

        template<size_t I = 1, typename ...Args>
            requires ((I == 1) && std::is_constructible_v<E, Args...>)
        constexpr explicit Result(std::in_place_index_t<I>, Args&&... args)
            : m_data(std::in_place_index<I>, std::forward<Args>(args)...) {}

        constexpr explicit Result<0>(std::in_place_index_t<0>) : m_data() {}

        constexpr Result(const Result&) = default;
        constexpr Result(Result&&) noexcept(std::is_nothrow_constructible_v<E>) = default;
        constexpr Result& operator=(const Result&) = default;
        constexpr Result& operator=(Result&&) noexcept(std::is_nothrow_move_constructible_v<E> &&
                std::is_nothrow_move_assignable_v<E>) = default;

        [[nodiscard]] constexpr bool is_ok() const noexcept {
            return !m_data.index();
        }

        template<typename F>
            requires requires (F f) {
                { std::invoke(f) } -> std::same_as<bool>;
            }
        [[nodiscard]] constexpr bool is_ok_and(F&& f) const noexcept(std::is_nothrow_invocable_v<F>) {
            return !m_data.index() && std::invoke(std::forward<F>(f));
        }

        [[nodiscard]] constexpr bool is_err() const noexcept {
            return !!m_data.index();
        }

        template<typename F>
            requires requires (F f, const E& e) {
                { std::invoke(f, e) } -> std::same_as<bool>;
            }
        [[nodiscard]] constexpr bool is_err_and(F&& f)
            const noexcept(std::is_nothrow_invocable_v<F, const E&>) {
            return !!m_data.index() && std::invoke(std::forward<F>(f), const_cast<const E&>(std::get<1>(m_data)));
        }

        template<typename U>
            requires (std::is_same_v<U, void> || std::is_same_v<U, E>)
        [[nodiscard]] constexpr ref_t<U> get() {
            try {
                if constexpr (std::is_same_v<U, void>) {
                    (void) std::get<0>(m_data); // necessary
                    return;
                } else {
                    return std::get<1>(m_data);
                }
            } catch(...) {
                panic("Invaild access to Result");
            }
        }

        template<typename U>
            requires (std::is_same_v<U, void> || std::is_same_v<U, E>)
        [[nodiscard]] constexpr const ref_t<U> get() const {
            try {
                if constexpr (std::is_same_v<U, void>) {
                    (void) std::get<0>(m_data); // necessary
                    return;
                } else {
                    return std::get<1>(m_data);
                }
            } catch(...) {
                panic("Invaild access to Result");
            }
        }

        template<size_t I>
            requires (I == 0 || I == 1)
        [[nodiscard]] constexpr alternative_ref_t<I> get() {
            try {
                if constexpr (I == 0) {
                    (void) std::get<0>(m_data);
                    return;
                }
                else {
                    return std::get<1>(m_data);
                }
            } catch(...) {
                panic("Invaild access to Result");
            }
        }

        template<size_t I>
            requires (I == 0 || I == 1)
        [[nodiscard]] constexpr const alternative_ref_t<I> get() const {
            try {
                if constexpr (I == 0) {
                    (void) std::get<0>(m_data);
                    return;
                } else {
                    return std::get<I>(m_data);
                }
            } catch(...) {
                panic("Invaild access to Result");
            }
        }

        [[nodiscard]] constexpr const std::variant<std::monostate, E>& data() const noexcept {
            return m_data;
        }

        // FIXME: 理论上应当返回std::optional<void>，但std::optional不允许含有void值，
        //        等待手动实现一个Option<T>后，再修改这里！
        [[nodiscard]] constexpr std::optional<std::monostate> ok() noexcept {
            if (is_err()) return std::nullopt;
            return std::move(get<0>());
        }

        [[nodiscard]] constexpr std::optional<E> err() noexcept(std::is_nothrow_move_constructible_v<E>) {
            if (is_ok()) return std::nullopt;
            return std::move(get<1>());
        }

        template<typename U, typename F>
            requires (requires (F f) {
                { std::invoke(f) } -> std::convertible_to<U>;
            } && std::is_copy_constructible_v<E>)
        [[nodiscard]] constexpr Result<U, E> map(F&& func) const
            noexcept(std::is_nothrow_constructible_v<U, std::invoke_result_t<F>> &&
                     std::is_nothrow_copy_constructible_v<E> && std::is_nothrow_invocable_v<F>) {
            if (is_err()) return Result<U, E>(std::in_place_index<1>, get<1>());
            return Result<U, E>(std::in_place_index<0>, std::invoke(std::forward<F>(func)));
        }

        // 非const同下
        template<typename U, typename F>
            requires (requires (F f) {
                { std::invoke(f) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<U>)
        [[nodiscard]] constexpr U map(U default_value, F&& func) const
            noexcept(std::is_nothrow_constructible_v<U, std::invoke_result_t<F>> &&
                     std::is_nothrow_move_constructible_v<U> && std::is_nothrow_invocable_v<F>) {
            if (is_err()) return default_value;
            return std::invoke(std::forward<F>(func));
        }

        template<typename U, typename D, typename F>
            requires (requires (D f0, F f1, const E& e) {
                { std::invoke(f0, e) } -> std::convertible_to<U>;
                { std::invoke(f1) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<U>)
        [[nodiscard]] constexpr U map(D&& fallback, F&& func) const
            noexcept(std::is_nothrow_constructible_v<U, std::invoke_result_t<D, const E&>> &&
                     std::is_nothrow_constructible_v<U, std::invoke_result_t<F>> &&
                     std::is_nothrow_move_constructible_v<U> && std::is_nothrow_invocable_v<D, const E&> &&
                     std::is_nothrow_invocable_v<F>) {
            if (is_err()) return std::invoke(std::forward<D>(fallback), get<1>());
            return std::invoke(std::forward<F>(func));
        }

        template<typename F, typename O>
            requires (requires (O op, const E& e) {
                { std::invoke(op, e) } -> std::convertible_to<F>;
            })
        [[nodiscard]] constexpr Result<void, F> map_err(O&& op) const
            noexcept(std::is_nothrow_constructible_v<F, std::invoke_result_t<O, const E&>> &&
                     std::is_nothrow_invocable_v<O, const E&>) {
            if (is_ok()) return Result<void, F>();
            return Result<void, F>(std::in_place_index<1>, std::invoke(std::forward<O>(op), get<1>()));
        }

        template<typename U, typename F>
            requires (requires (F f) {
                { std::invoke(f) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<E>)
        [[nodiscard]] constexpr Result<U, E> map(F&& func)
            noexcept(std::is_nothrow_constructible_v<U, std::invoke_result_t<F>> &&
                     std::is_nothrow_move_constructible_v<E> &&
                     std::is_nothrow_invocable_v<F>) {
            if (is_err()) return Result<U, E>(std::in_place_index<1>, std::move(get<1>()));
            return Result<U, E>(std::in_place_index<0>, std::invoke(std::forward<F>(func)));
        }

        template<typename U, typename D, typename F>
            requires (requires (D f0, F f1, E e) {
                { std::invoke(f0, std::move(e)) } -> std::convertible_to<U>;
                { std::invoke(f1) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<U> && std::is_move_constructible_v<E>)
        [[nodiscard]] constexpr U map(D&& fallback, F&& func)
            noexcept (std::is_nothrow_constructible_v<U, std::invoke_result_t<D, E>> &&
                      std::is_nothrow_constructible_v<U, std::invoke_result_t<F>> &&
                      std::is_nothrow_move_constructible_v<U> && std::is_nothrow_move_constructible_v<E> &&
                      std::is_nothrow_invocable_v<D, E> && std::is_nothrow_invocable_v<F>) {
            if (is_err()) return std::invoke(std::forward<D>(fallback), std::move(get<1>()));
            return std::invoke(std::forward<F>(func));
        }

        template<typename F, typename O>
            requires (requires (O op, E e) {
                { std::invoke(op, std::move(e)) } -> std::convertible_to<F>;
            } && std::is_move_constructible_v<E>)
        [[nodiscard]] constexpr Result<void, F> map_err(O&& op)
            noexcept(std::is_nothrow_constructible_v<F, std::invoke_result_t<O, E>> &&
                     std::is_nothrow_move_constructible_v<E> && std::is_nothrow_invocable_v<O, E>) {
            if (is_ok()) return Result<void, F>();
            return Result<void, F>(std::in_place_index<1>, std::invoke(
                            std::forward<O>(op), std::move(get<1>())));
        }

        // 非const同下
        constexpr void expect(const std::string_view& msg) const {
            if (is_ok()) return;
            call_panic_with_TE_uncheck<1>(msg, ": ");
        }
        
        constexpr void unwrap() const {
            if (is_ok()) return;
            call_panic_with_TE_uncheck<1>("", "");
        }

        constexpr void unwrap_or_default() const {
            return;
        }

        [[nodiscard]] constexpr E expect_err(const std::string_view& msg) {
            if (is_err()) return std::move(get<1>());
            call_panic_with_TE_uncheck<0>(msg, ": ");
        }

        [[nodiscard]] constexpr E expect_err(const std::string_view& msg) const {
            if (is_err()) return get<1>();
            call_panic_with_TE_uncheck<0>(msg, ": ");
        }

        [[nodiscard]] constexpr E unwrap_err() {
            if (is_err()) return std::move(get<1>());
            call_panic_with_TE_uncheck<0>("", "");
        }

        [[nodiscard]] constexpr E unwrap_err() const {
            if (is_err()) return get<1>();
            call_panic_with_TE_uncheck<0>("", "");
        }

        template<typename U>
        [[nodiscard]] constexpr Result<U, E> and_then(Result<U, E> res)
        noexcept(std::is_nothrow_move_constructible_v<Result<U, E>> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::in_place_index_t<1>, E&&> &&
                 std::is_nothrow_move_constructible_v<E>) {
            if (is_ok()) return res;
            return Result<U, E>(std::in_place_index<1>, std::move(get<1>()));
        }

        template<typename U>
        [[nodiscard]] constexpr Result<U, E> and_then(Result<U, E> res) const
        noexcept(std::is_nothrow_move_constructible_v<Result<U, E>> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::in_place_index_t<1>, const E&> &&
                 std::is_nothrow_copy_constructible_v<E>) {
            if (is_ok()) return res;
            return Result<U, E>(std::in_place_index<1>, get<1>());
        }

        template<typename U, typename F>
            requires (requires (F f) {
                { std::invoke(f) } -> std::convertible_to<Result<U, E>>;
            })
        [[nodiscard]] constexpr Result<U, E> and_then(F&& op)
        noexcept(std::is_nothrow_invocable_v<F> && std::is_nothrow_move_constructible_v<E> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::in_place_index_t<1>, E&&> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::invoke_result_t<F>>) {
            if (is_ok()) return std::invoke(std::forward<F>(op), std::move(get<0>()));
            return Result<U, E>();
        }

        template<typename U, typename F>
            requires (requires (F f) {
                { std::invoke(f) } -> std::convertible_to<Result<U, E>>;
            })
        [[nodiscard]] constexpr Result<U, E> and_then(F&& op) const
        noexcept(std::is_nothrow_invocable_v<F> && std::is_nothrow_copy_constructible_v<E> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::in_place_index_t<1>, const E&> &&
                 std::is_nothrow_constructible_v<Result<U, E>, std::invoke_result_t<F>>) {
            if (is_ok()) return std::invoke(std::forward<F>(op));
            return Result<U, E>(std::in_place_index<1>, get<1>());
        }

        template<typename F>
        [[nodiscard]] constexpr Result<void, F> or_else(Result<void, F> res) const
        noexcept(std::is_nothrow_move_constructible_v<Result<void, F>>) {
            if (is_err()) return res;
            return Result<void, F>();
        }

        template<typename F, typename O>
            requires (requires (O f, E e) {
                { std::invoke(f, std::move(e)) } -> std::convertible_to<Result<void, F>>;
            })
        [[nodiscard]] constexpr Result<void, F> or_else(O&& op)
        noexcept(std::is_nothrow_invocable_v<O, E&&> && std::is_nothrow_move_constructible_v<E> &&
                 std::is_nothrow_constructible_v<Result<void, E>, std::invoke_result_t<O, E&&>>) {
            if (is_err()) return std::invoke(std::forward<O>(op), std::move(get<1>()));
            return Result<void, F>();
        }

        template<typename F, typename O>
            requires (requires (O f, const E& e) {
                { std::invoke(f, e) } -> std::convertible_to<Result<void, F>>;
            })
        [[nodiscard]] constexpr Result<void, F> or_else(O&& op) const
        noexcept(std::is_nothrow_invocable_v<O, const E&> &&
                 std::is_nothrow_constructible_v<Result<void, F>, std::invoke_result_t<O, const E&>>) {
            if (is_err()) return std::invoke(std::forward<O>(op), get<1>());
            return Result<void, F>();
        }

        constexpr void unwrap_or() const noexcept {
            return;
        }

        [[nodiscard]] constexpr Result<void, E> clone() const noexcept(std::is_nothrow_copy_constructible_v<Result<void, E>>) {
            return *this;
        }

        template<typename U, typename F>
        [[nodiscard]] Result<void, E>& assign(const Result<U, F>& other) {
            if (other.is_ok()) m_data.template emplace<0>();
            else m_data.emplace<1>(other.template get<1>());
            return *this;
        }

        template<typename U, typename F>
        [[nodiscard]] Result<void, E>& assign(Result<U, F>&& other) {
            if (other.is_ok()) m_data.template emplace<0>();
            else m_data.emplace<1>(std::move(other.template get<1>()));
            return *this;
        }

        [[nodiscard]] constexpr const Result<void, E>& as_const() const noexcept {
            return *this;
        }

        [[nodiscard]] constexpr Result<void, E>& as_mut() const noexcept {
            return const_cast<Result<void, E>&>(*this);
        }

        [[nodiscard]] constexpr as_cref_t as_ref() const noexcept {
            if (is_ok()) return as_cref_t();
            return as_cref_t(std::in_place_index<1>, std::cref(get<1>()));
        }

        [[nodiscard]] constexpr as_ref_t as_ref() noexcept {
            if (is_ok()) return as_ref_t();
            return as_ref_t(std::in_place_index<1>, std::ref(get<1>()));
        }

    private:
        /**
         * @brief 调用panic，同时，若类型是formattable时，打印其值
         */
        template<size_t I>
            requires (I == 0 || I == 1)
        [[noreturn]] constexpr void call_panic_with_TE_uncheck(const std::string_view& msg, const std::string_view& sep) const {
            using U = E;
            if constexpr (
#ifdef MY_CXX23
                (std::formattable<U, char>)
#else // C++23 ^^^ /  vvv C++20
                (requires (std::formatter<U>& f, const std::formatter<U>& cf, U&& t,
                    std::format_parse_context& p, std::format_context& fc) {
                    { f.parse(p) };
                    { cf.format(t, fc) };
                })
#endif
            && I == 1) {
                auto&& str = std::format("{}{}{}", msg, sep, get<I>());
                panic(str);
            } else {
                panic(msg);
            }
        }

    private:
        std::variant<std::monostate, E> m_data;
    };


    template<typename T, typename E>
        requires (std::three_way_comparable<T> && std::three_way_comparable<E>)
    constexpr std::common_comparison_category_t<
        std::compare_three_way_result_t<T>, std::compare_three_way_result_t<E>>
        operator<=>(const Result<T, E>& lhs, const Result<T, E>& rhs) {
        return lhs.data() <=> rhs.data();
    }

    template<typename T, typename E>
        requires (std::equality_comparable<T> && std::equality_comparable<E>)
    constexpr bool operator==(const Result<T, E>& lhs, const Result<T, E>& rhs) {
        return lhs.data() == rhs.data();
    }

    
    template<class T>
    struct result_transform {
    private:
        using U = std::remove_reference_t<T>;
    public:
        using type = std::conditional_t<
            std::is_array_v<U>,
            std::add_pointer_t<std::remove_extent_t<U>>,
            std::conditional_t<
                std::is_function_v<U>,
                std::add_pointer_t<U>,
                U>>;
    };

    template<typename T>
    using result_transform_t = result_transform<T>::type;

    /**
     * @brief Result对象的工厂函数。
     *
     * 由于T类型可推导，因此类型参数中E在前。
     *
     * @tparam E 构造的Result的错误类型result_transform_t<E>
     * @tparam T 构造的Result的存储数据的类型result_transform_t<T>
     *
     * @param value 构造Result的Ok状态的值
     *
     * @return 构造的Result
     *
     * @warning 请务必注意类型变量的位置！
     *
     * @example
     * ```cpp
     * // Result<int, const char *>
     * auto x = C163q::Ok<const char*>(-3);
     * ```
     */
    template<typename E, typename T>
        requires std::move_constructible<result_transform_t<T>>
    [[nodiscard]] constexpr Result<result_transform_t<T>, result_transform_t<E>> Ok(T&& value)
        noexcept(std::is_nothrow_constructible_v<result_transform_t<T>, T>) {
        return Result<result_transform_t<T>, result_transform_t<E>>(std::in_place_index<0>, std::forward<T>(value));
    }

    /**
     * @brief Result对象的工厂函数。
     *
     * 为了保持与另一个重载的签名一致，因此类型参数中E在前。
     *
     * @tparam E 构造的Result的错误类型
     * @tparam T 构造的Result的存储数据的类型
     *
     * @param args 构造Result的Ok状态的参数
     *
     * @return 构造的Result
     *
     * @warning 请务必注意类型变量的位置！
     *
     * @example
     * ```cpp
     * // Result<std::tuple<int, float>, std::exception>
     * auto x = C163q::Ok<std::exception, std::tuple<int, float>>(1, 1.0f);
     * ```
     */
    template<typename E, typename T, typename ...Args>
        requires std::constructible_from<result_transform_t<T>, Args...>
    [[nodiscard]] constexpr Result<result_transform_t<T>, result_transform_t<E>> Ok(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<result_transform_t<T>, Args...>) {
        return Result<result_transform_t<T>, result_transform_t<E>>(std::in_place_index<0>, std::forward<Args>(args)...);
    }

    /**
     * @brief Result对象的工厂函数。
     *
     * 由于E类型可推导，因此类型参数中T在前。
     *
     * @tparam T 构造的Result的存储数据的类型
     * @tparam E 构造的Result的错误类型
     *
     * @param err 构造Result的Err状态的值
     *
     * @return 构造的Result
     *
     * @example
     * ```
     * // Result<int, const char *>
     * auto y = C163q::Err<int, const char*>("Some error message");
     * ```
     */
    template<typename T, typename E>
        requires std::move_constructible<result_transform_t<E>>
    [[nodiscard]] constexpr Result<result_transform_t<T>, result_transform_t<E>> Err(E&& err)
        noexcept(std::is_nothrow_constructible_v<result_transform_t<E>, E>) {
        return Result<result_transform_t<T>, result_transform_t<E>>(std::in_place_index<1>, std::forward<E>(err));
    }

    /**
     * @brief Result对象的工厂函数。
     *
     * 为了保持与另一个重载的签名一致，因此类型参数中T在前。
     *
     * @tparam T 构造的Result的存储数据的类型
     * @tparam E 构造的Result的错误类型
     *
     * @param args 构造Result的Err状态的参数
     *
     * @return 构造的Result
     *
     * @example
     * ```cpp
     * // Result<std::tuple<int, float>, std::exception>
     * auto y = C163q::Err<std::tuple<int, float>, std::exception>();
     * ```
     */
    template<typename T, typename E, typename ...Args>
        requires std::constructible_from<result_transform_t<E>, Args...>
    [[nodiscard]] constexpr Result<result_transform_t<T>, result_transform_t<E>> Err(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<result_transform_t<E>, Args...>) {
        return Result<result_transform_t<T>, result_transform_t<E>>(
                std::in_place_index<1>, std::forward<Args>(args)...);
    }

    /**
     * @brief 代理调用函数，正常返回值或捕获异常值都会存储在Result中。
     *
     * @example
     * ```
     * std::string s1 = "123456";
     * std::string s2 = "foo";
     * auto f = [](const std::string& s) {
     *     int(*f)(const std::string&, size_t*, int) = std::stoi;
     *     return f(s, nullptr, 10);
     * };
     *
     * auto val1 = C163q::result_helper<std::exception>::invoke(f, s1).unwrap_or_default();
     * assert(val1 == 123456);
     *
     * auto val2 = C163q::result_helper<std::exception>::invoke(f, s2).unwrap_or_default();
     * assert(val2 == 0);
     * ```
     */
    template<typename E>
    struct result_helper {
        template<typename F, typename ...Args>
            using Ret_t = std::invoke_result_t<F, Args...>;

        template<typename F, typename ...Args>
            using T = std::conditional_t<std::is_lvalue_reference_v<Ret_t<F, Args...>>,
                  std::reference_wrapper<std::remove_reference_t<Ret_t<F, Args...>>>,
                  std::remove_reference_t<Ret_t<F, Args...>>>;

        template<typename F, typename ...Args>
            requires std::invocable<F, Args...>
        static Result<T<F, Args...>, E> invoke(F&& f, Args&&... args) {
            using Type = T<F, Args...>;
            try {
                return Result<Type, E>(std::in_place_index<0>,
                        ::std::invoke(std::forward<F>(f), std::forward<Args>(args)...));
            } catch (E& e) {
                return Result<Type, E>(std::in_place_index<1>, std::move(e));
            }
        }

        template<typename F, typename ...Args>
            requires std::invocable<F, Args...>
        static Result<T<F, Args...>, E> invoke_else_panic(F&& f, Args&&... args) {
            using Type = T<F, Args...>;
            try {
                return Result<Type, E>(std::in_place_index<0>,
                        ::std::invoke(std::forward<F>(f), std::forward<Args>(args)...));
            } catch (E& e) {
                return Result<Type, E>(std::in_place_index<1>, std::move(e));
            } catch (...) {
                panic("panics after call `result_helper::invoke_else_panic`");
            }
        }
    };

}

namespace std {

    /**
     * @brief 访问Result内所保有元素
     *
     * @tparam U 要访问的元素的类型。若为T，尝试访问Ok时所保有的值
     *           若为E，尝试访问Err时所保有的值
     * @tparam I 要访问的元素的索引。若为0，尝试访问Ok时所保有的值
     *           若为1，尝试访问Err时所保有的值
     *
     * @param result 要访问的Result对象
     *
     * @panic 当访问错误的类型或索引时将导致panic
     */
    template<typename U, typename T, typename E>
        requires (std::is_same_v<U, T> || std::is_same_v<U, E>)
    [[nodiscard]] constexpr U& get(C163q::Result<T, E>& result) {
        return result.template get<U>();
    }

    template<typename U, typename T, typename E>
        requires (std::is_same_v<U, T> || std::is_same_v<U, E>)
    [[nodiscard]] constexpr const U& get(const C163q::Result<T, E>& result) {
        return result.template get<U>();
    }

    template<size_t I, typename T, typename E>
        requires (I == 0 || I == 1)
    [[nodiscard]] constexpr variant_alternative_t<I, std::variant<T, E>>& get(
            C163q::Result<T, E>& result) {
        return result.template get<I>();
    }

    template<size_t I, typename T, typename E>
        requires (I == 0 || I == 1)
    [[nodiscard]] constexpr const std::variant_alternative_t<I, std::variant<T, E>>& get(
            const C163q::Result<T, E>& result) {
        return result.template get<I>();
    }

}



#endif // MY_CXX20
#endif // !C163Q_MY_CPP_UTILS_RS_RESULT20_HPP
