/**
 * @file rs/result.hpp
 * @brief 在C++中实现rust当中的Result类
 *
 * 至少需要C++23
 *
 * @since Jul 17, 2025
 */

#include"../core/config.hpp"
#ifndef MY_CXX20
    static_assert(false, "Require C++20!")
#endif

#include<concepts>
#include<functional>
#include<optional>
#include<type_traits>
#include<utility>
#include<variant>
#include"panic.hpp"

namespace C163q {

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
     */
    template<typename T, typename E>
        requires (std::is_object_v<T> && std::is_object_v<E> &&
                 !std::is_same_v<std::decay_t<T>, std::decay_t<E>>)
    class Result {

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
        template<typename U ,typename ...Args>
            requires (std::is_same_v<T, U> || std::is_same_v<E, U>) &&
                      std::is_constructible_v<U, Args...>
        constexpr explicit Result(std::in_place_type_t<U>, Args&&... args)
            : m_data(std::in_place_type<U>, std::forward<Args>(args)...) {}

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
            requires requires (F f, const T& t) {
                { std::invoke(f, t) } -> std::same_as<bool>;
            }
        [[nodiscard]] constexpr bool is_ok_and(F&& f)
            const noexcept(std::is_nothrow_invocable_v<F, T>) {
            return !m_data.index() && std::invoke(std::forward<F>(f), std::get<T>(m_data));
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
            requires requires (F f, const E& e) {
                { std::invoke(f, e) } -> std::same_as<bool>;
            }
        [[nodiscard]] constexpr bool is_err_and(F&& f)
            const noexcept(noexcept(f(std::declval<E>()))) {
            return !!m_data.index() && std::invoke(std::forward<F>(f), std::get<E>(m_data));
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
            requires (std::is_same_v<U, T> || std::is_same_v<U, E>)
        [[nodiscard]] constexpr U& get() noexcept {
            try {
                return std::get<U>(m_data);
            } catch(...) {
                panic("Invaild access to Result");
            }
        }

        template<typename U>
            requires (std::is_same_v<U, T> || std::is_same_v<U, E>)
        [[nodiscard]] constexpr const U& get() const noexcept {
            try {
                return std::get<U>(m_data);
            } catch(...) {
                panic("Invaild access to Result");
            }
        }

        template<size_t I>
            requires (I == 0 || I == 1)
        [[nodiscard]] constexpr std::variant_alternative_t<I, std::variant<T, E>>& get() noexcept {
            try {
                return std::get<I>(m_data);
            } catch(...) {
                panic("Invaild access to Result");
            }
        }

        template<size_t I>
            requires (I == 0 || I == 1)
        [[nodiscard]] constexpr const std::variant_alternative_t<I, std::variant<T, E>>& get() const noexcept {
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
         * @brief 将Result<T, E>转换为std::optional<T>
         *
         * 将自身转换为std::optional<T>，自身保有的值将会被移动，若为Err状态，则为空值。
         *
         * @return 一个std::optional<T>。在Ok状态下，移动构造一个有值的std::optional<T>，
         *         在Err状态下，返回一个无值的std::optional<T>
         *
         * @example
         * ```
         * auto x = C163q::Ok<const char*>(std::vector{ 1, 2, 3, 4 });
         * auto x_cmp = std::vector{ 1, 2, 3, 4 };
         * assert(x.ok().value() == x_cmp);
         * assert(std::get<0>(x).size() == 0); // moved
         * 
         * auto y = C163q::Err<unsigned>("Err");
         * assert(y.ok().has_value() == false);
         * ```
         */
        [[nodiscard]] constexpr std::optional<T> ok() noexcept(std::is_nothrow_move_constructible_v<T>) {
            if (is_err()) return std::nullopt;
            return std::move(get<0>());
        }

        /**
         * @brief 将Result<T, E>转换为std::optional<E>
         *
         * 将自身转换为std::optional<E>，自身保有的值将会被移动，若为Ok状态，则为空值。
         *
         * @return 一个std::optional<E>。在Err状态下，移动构造一个有值的std::optional<E>，
         *         在Err状态下，返回一个无值的std::optional<E>
         *
         * @example
         * ```
         * auto x = C163q::Ok<const char*>(1);
         * assert(x.err().has_value() == false);
         *
         * auto y = C163q::Err<unsigned>("Err");
         * assert(y.err().value()[0] == 'E');
         * ```
         */
        [[nodiscard]] constexpr std::optional<E> err() noexcept(std::is_nothrow_move_constructible_v<E>) {
            if (is_ok()) return std::nullopt;
            return std::move(get<1>());
        }

        /**
         * @brief 使用可调用对象将Result<T, E>映射为Result<U, E>，在Ok时调用，而Err时什么都不做
         *
         * 在Ok状态下时，const T&会被传入并将返回值作为构造Result<U, E>的U类型的值
         * 在Err状态下，仅仅是复制构造E的值
         *
         * @param 用于映射的可调用对象，接收一个const T&并返回一个可以构造为U的类型
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
         *     auto val = res.map<double>([](auto val) { return val * 1.5; });
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
            if (is_err()) return Result<U, E>(std::in_place_type<E>, get<1>());
            return Result<U, E>(std::in_place_type<U>, std::move(std::invoke(std::forward<F>(func), get<0>())));
        }

        /**
         * @brief 如果处于Err状态，返回默认值，否则用保有值调用可调用对象
         *
         * @param default_value 当处于Err状态时返回的值
         * @param func          当处于Ok状态时所调用的可调用对象，参数类型为const T&，返回值类型为U
         *
         * @tparam U 返回值类型，default_value应该为该类型，func的返回值应当可以转换为该类型
         * @tparam F 可调用对象的类型
         *
         * @return 类型为U，在Err状态下返回default_value，否则使用保有值调用可调用对象，并返回
         *
         * @example
         * ```
         * auto x = C163q::Ok<std::exception, std::string>("foo");
         * assert(x.map(42, [](auto&& str) { return str.size(); }) == 3);
         *
         * auto y = C163q::Err<std::string, const char*>("bar");
         * assert(y.map(42, [](auto&& str) { return str.size(); }) == 42);
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
         * @brief 使用可调用对象将Result<T, E>映射为Result<U, E>，在Ok时调用，而Err时什么都不做
         *
         * 在Ok状态下时，T&&会被移动传入并将返回值作为构造Result<U, E>的U类型的值
         * 在Err状态下，仅仅是移动构造E的值
         *
         * @warning 不同于map()方法，map_into()调用后原始值被移动
         *
         * @param 用于映射的可调用对象，接收一个const T&并返回一个可以构造为U的类型
         * 
         * @tparam U 返回的Result中，处于Ok状态时保有的值的类型，func的返回值应该能够转换为该类型
         * @tparam F 可调用对象的类型                                                                                             @example
         * 
         * @example
         * ```
         * C163q::Result<std::vector<int>, const char*> src(std::vector{1, 2, 3, 4});
         * auto dst = src.map_into<std::vector<int>>([](auto&& v) {
         *         for (auto&& e : v) {
         *             e *= 2;
         *         }
         *         return v;
         *     });
         * std::vector<int> check{ 2, 4, 6, 8 };
         * assert(dst.get<0>() == check);
         * assert(src.get<0>().size() == 0);
         * ```
         */
        template<typename U, typename F>
            requires (requires (F f, T t) {
                { std::invoke(f, std::move(t)) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<E> && std::is_move_constructible_v<T>)
        [[nodiscard]] constexpr Result<U, E> map_into(F&& func)
            noexcept(std::is_nothrow_constructible_v<U, std::invoke_result_t<F, T>> &&
                     std::is_nothrow_move_constructible_v<E> && std::is_nothrow_invocable_v<F, T>) {
            if (is_err()) return Result<U, E>(std::in_place_type<E>, std::move(get<1>()));
            return Result<U, E>(std::in_place_type<U>, std::move(std::invoke(
                            std::forward<F>(func), std::move(get<0>()))));
        }

        /**
         * @brief 如果处于Err状态，返回默认值，否则移动保有值调用可调用对象
         *
         * @warning 不同于map()方法，map_into()调用后原始值被移动
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
         * auto dst = src.map_into<int>(0, [](std::vector<int> v) {
         *         return std::accumulate(std::begin(v), std::end(v), 0);
         *     });
         * assert(dst == 10);
         * assert(src.get<0>().size() == 0);
         *
         * C163q::Result<const char*, std::string> x(std::in_place_type<std::string>, "bar");
         * auto y = x.map_into<std::string>("Error", [](std::string v) {
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
        [[nodiscard]] constexpr U map_into(U default_value, F&& func)
            noexcept(std::is_nothrow_constructible_v<U, std::invoke_result_t<F, T>> &&
                     std::is_nothrow_move_constructible_v<U> && std::is_nothrow_invocable_v<F, T>) {
            if (is_err()) return default_value;
            return std::invoke(std::forward<F>(func), std::move(get<0>()));
        }

    private:
        std::variant<T, E> m_data;
    };


    /**
     * @brief Result对象的工厂函数。
     *
     * 由于T类型可推导，因此类型参数中E在前。
     *
     * @tparam E 构造的Result的错误类型std::decay_t<E>
     * @tparam T 构造的Result的存储数据的类型std::decay_t<T>
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
        requires std::move_constructible<std::decay_t<T>>
    Result<std::decay_t<T>, std::decay_t<E>> Ok(T&& value)
        noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T>) {
        return Result<std::decay_t<T>, std::decay_t<E>>(std::in_place_type<std::decay_t<T>>, std::forward<T>(value));
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
        requires std::constructible_from<std::decay_t<T>, Args...>
    Result<std::decay_t<T>, std::decay_t<E>> Ok(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, Args...>) {
        return Result<std::decay_t<T>, std::decay_t<E>>(std::in_place_type<std::decay_t<T>>, std::forward<Args>(args)...);
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
        requires std::move_constructible<std::decay_t<E>>
    Result<std::decay_t<T>, std::decay_t<E>> Err(E&& err)
        noexcept(std::is_nothrow_constructible_v<std::decay_t<E>, E>) {
        return Result<std::decay_t<T>, std::decay_t<E>>(std::in_place_type<std::decay_t<E>>, std::forward<E>(err));
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
        requires std::constructible_from<std::decay_t<E>, Args...>
    Result<std::decay_t<T>, std::decay_t<E>> Err(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<std::decay_t<E>, Args...>) {
        return Result<std::decay_t<T>, std::decay_t<E>>(std::in_place_type<std::decay_t<E>>, std::forward<Args>(args)...);
    }

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
    [[nodiscard]] constexpr U& get(C163q::Result<T, E>& result) noexcept {
        return result.template get<U>();
    }

    template<typename U, typename T, typename E>
        requires (std::is_same_v<U, T> || std::is_same_v<U, E>)
    [[nodiscard]] constexpr const U& get(const C163q::Result<T, E>& result) noexcept {
        return result.template get<U>();
    }

    template<size_t I, typename T, typename E>
        requires (I == 0 || I == 1)
    [[nodiscard]] constexpr variant_alternative_t<I, std::variant<T, E>>& get(
            C163q::Result<T, E>& result) noexcept {
        return result.template get<I>();
    }

    template<size_t I, typename T, typename E>
        requires (I == 0 || I == 1)
    [[nodiscard]] constexpr const std::variant_alternative_t<I, std::variant<T, E>>& get(
            const C163q::Result<T, E>& result) noexcept {
        return result.template get<I>();
    }

}

