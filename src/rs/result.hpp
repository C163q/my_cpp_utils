/**
 * @file rs/result.hpp
 * @brief 在C++中实现rust当中的Result类
 */

#include<functional>
#include<type_traits>
#include<utility>
#include<variant>

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
         */
        constexpr explicit Result(T&& value) noexcept(std::is_nothrow_constructible_v<T, T>)
            : m_data(std::forward<T>(value)) {}

        /**
         * @brief 使用E构造Err所保有的类型
         *
         * 要求E至少是可移动构造的，这会使Result处于Err状态。
         *
         * @param value 要存储的异常
         */
        constexpr explicit Result(E&& err) noexcept(std::is_nothrow_constructible_v<E, E>)
            : m_data(std::forward<E>(err)) {}

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
            : m_data(std::in_place_type<U>, args...) {}

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
        constexpr bool is_ok() const noexcept {
            return !m_data.index();
        }

        /**
         * @brief 当Result处于Ok状态且其值满足谓词f时返回true，否则false
         *
         * @param f 参数为const T&且返回bool的谓词，
         *          Result内保有的数据的常引用会被传入。
         *
         * @return 当Result处于Ok状态且其保有满足谓词f的值时返回true，否则false
         *
         * @example
         * ```cpp
         * C163q::Result<unsigned, const char*> x(2);
         * assert(x.is_ok_and([](auto v) { return v > 1; }) == true);
         *
         * auto y = C163q::Ok<const char*>(0u);
         * assert(y.is_ok_and([](const unsigned& v) { return v > 1; }) == false);
         *
         * C163q::Result<unsigned, const char*> z("hey");
         * assert(z.is_ok_and([](auto v) { return v > 1; }) == false);
         * ```
         */
        constexpr bool is_ok_and(std::function<bool(const T&)> f)
            const noexcept(noexcept(f(std::declval<T>()))) {
            return !m_data.index() && f(std::get<0>(m_data));
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
        constexpr bool is_err() const noexcept {
            return !!m_data.index();
        }


    private:
        std::variant<T, E> m_data;
    };


    /**
     * @brief Result对象的工厂函数。
     *
     * 由于T类型可推导，因此类型参数中E在前。
     *
     * @tparam E 构造的Result的错误类型
     * @tparam T 构造的Result的存储数据的类型
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
    Result<T, E> Ok(T&& value) noexcept(std::is_nothrow_constructible_v<T, T>) {
        return Result<T, E>(std::forward<T>(value));
    }

    /**
     * @brief Result对象的工厂函数。
     *
     * 为了保持与另一个重载的签名一致，因此类型参数中E在前。
     *
     * @tparam E 构造的Result的错误类型
     * @tparam T 构造的Result的存储数据的类型
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
    Result<T, E> Ok(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        return Result<T, E>(std::in_place_type<T>, std::forward<Args>(args)...);
    }

    /**
     * @brief Result对象的工厂函数。
     *
     * 由于E类型可推导，因此类型参数中T在前。
     *
     * @tparam T 构造的Result的存储数据的类型
     * @tparam E 构造的Result的错误类型
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
    Result<T, E> Err(E&& err) noexcept(std::is_nothrow_constructible_v<E, E>) {
        return Result<T, E>(std::forward<E>(err));
    }

    /**
     * @brief Result对象的工厂函数。
     *
     * 为了保持与另一个重载的签名一致，因此类型参数中T在前。
     *
     * @tparam T 构造的Result的存储数据的类型
     * @tparam E 构造的Result的错误类型
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
    Result<T, E> Err(Args&&... args) noexcept(std::is_nothrow_constructible_v<E, Args...>) {
        return Result<T, E>(std::in_place_type<E>, std::forward<Args>(args)...);
    }

}


