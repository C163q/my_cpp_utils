/*!
 * @file rs/option.hpp
 * @brief 在C++中实现rust当中的Option类
 *
 * 至少需要C++20
 *
 * @since Aug 4, 2025
 */

#ifndef C163Q_MY_CPP_UTILS_RS_OPTION_HPP
#define C163Q_MY_CPP_UTILS_RS_OPTION_HPP

#include"../core/config.hpp"

#ifndef MY_CXX20
    static_assert(false, "Require C++20!");
#else

#include"result.hpp"
#include"panic.hpp"
#include<algorithm>
#include<concepts>
#include<cstddef>
#include<functional>
#include<initializer_list>
#include<memory>
#include<optional>
#include<string_view>
#include<tuple>
#include<type_traits>
#include<utility>
#include<variant>

namespace C163q {

    template<typename T, typename E>
        requires (std::is_object_v<T> && std::is_object_v<E>) || std::is_same_v<T, void>
    class Result;

    template<typename E>
    class Result<void, E>;


    template<typename T>
        requires (std::is_object_v<T> && !std::is_array_v<T> &&
                 !std::is_same_v<std::remove_cv_t<T>, std::nullopt_t> &&
                 !std::is_same_v<std::remove_cv_t<T>, std::in_place_t>) ||
                  std::is_same_v<T, void>
    class Option {
    private:
        template<typename U>
        using possibly_convert_to_option = std::disjunction<
            std::is_constructible<T, Option<U>&>,
            std::is_constructible<T, const Option<U>&>,
            std::is_constructible<T, Option<U>&&>,
            std::is_constructible<T, const Option<U>&&>,
            std::is_convertible<Option<U>&, T>,
            std::is_convertible<const Option<U>&, T>,
            std::is_convertible<Option<U>&&, T>,
            std::is_convertible<const Option<U>&&, T>
        >;

        template<typename U>
        using possibly_convert_assign_to_option = std::disjunction<
            possibly_convert_to_option<U>,
            std::is_assignable<T&, Option<U>&>,
            std::is_assignable<T&, const Option<U>&>,
            std::is_assignable<T&, Option<U>&&>,
            std::is_assignable<T&, const Option<U>&&>
        >;

        template<typename U>
        using is_possibly_self_t = std::disjunction<
            std::is_same<std::remove_cvref_t<U>, Option<T>>,
            std::is_same<std::remove_cvref_t<U>, std::optional<T>>
        >;

        using as_ref_t = Option<std::reference_wrapper<T>>;
        using as_cref_t = Option<std::reference_wrapper<const T>>;


        template<size_t I, typename U>
        struct my_tuple_element {
            template<typename V>
            static std::type_identity<typename std::tuple_element<I, V>::type> test(std::tuple_element<I, V>*)
            { return {}; }

            template<typename V>
            static std::type_identity<void> test(...)
            { return {}; }

            using type = decltype(test<U>(nullptr))::type;
        };

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


    public:
        /**
         * @brief 构造函数，std::optional构造函数的包装
         */
        constexpr Option() noexcept : m_data(std::nullopt) {}
        constexpr Option(std::nullopt_t) noexcept : m_data(std::nullopt) {}

        constexpr Option(const Option& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
            : m_data(other.m_data) {}
        constexpr Option(Option&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
            : m_data(std::move(other.m_data)) {}

        // 使std::optional能够隐式转换为Option
        constexpr Option(const std::optional<T>& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
            : m_data(other) {}
        constexpr Option(std::optional<T>&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
            : m_data(std::move(other)) {}

        template<typename U>
            requires (std::is_constructible_v<T, const U&> &&
                      std::conditional_t<std::is_same_v<std::remove_cv_t<T>, bool>,
                        std::true_type, std::negation<possibly_convert_to_option<U>>>::value)
        constexpr explicit(!std::is_convertible_v<const U&, T>)
            Option(const Option<U>& other) noexcept(std::is_nothrow_constructible_v<T, const U&>)
            : m_data(other.m_data) {}

        template<typename U>
            requires (std::is_constructible_v<T, U> &&
                      std::conditional_t<std::is_same_v<std::remove_cv_t<T>, bool>,
                        std::true_type, std::negation<possibly_convert_to_option<U>>>::value)
        constexpr explicit(!std::is_convertible_v<U, T>)
            Option(Option<U>&& other) noexcept(std::is_nothrow_constructible_v<T, U>)
            : m_data(std::move(other.m_data)) {}

        // 使std::optional能够隐式转换为Option
        template<typename U>
            requires (std::is_constructible_v<T, const U&> &&
                      std::conditional_t<std::is_same_v<std::remove_cv_t<T>, bool>,
                        std::true_type, std::negation<possibly_convert_to_option<U>>>::value)
        constexpr explicit(!std::is_convertible_v<const U&, T>)
            Option(const std::optional<U>& other) noexcept(std::is_nothrow_constructible_v<T, const U&>)
            : m_data(other) {}

        template<typename U>
            requires (std::is_constructible_v<T, U> &&
                      std::conditional_t<std::is_same_v<std::remove_cv_t<T>, bool>,
                        std::true_type, std::negation<possibly_convert_to_option<U>>>::value)
        constexpr explicit(!std::is_convertible_v<U, T>)
            Option(std::optional<U>&& other) noexcept(std::is_nothrow_constructible_v<T, U>)
            : m_data(std::move(other.m_data)) {}

        template<typename ...Args>
            requires std::is_constructible_v<T, Args...>
        constexpr explicit Option(std::in_place_t, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args...>)
            : m_data(std::in_place, std::forward<Args>(args)...) {}

        template<typename U, typename ...Args>
            requires std::is_constructible_v<T, std::initializer_list<U>&, Args...>
        constexpr explicit Option(std::in_place_t, std::initializer_list<U> ilist, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>)
            : m_data(std::in_place, ilist, std::forward<Args>(args)...) {}

        template<typename U = std::remove_cv_t<T>>
            requires (std::is_constructible_v<T, U> &&
                     !std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> &&
                     !is_possibly_self_t<U>::value &&
                      std::conditional_t<std::is_same_v<std::remove_cv_t<T>, bool>,
                        std::negate<is_possibly_self_t<U>>, std::true_type>::value)
        constexpr explicit(!std::is_convertible_v<U, T>) Option(U&& value)
            noexcept(std::is_nothrow_constructible_v<T, U>)
            : m_data(std::forward<U>(value)) {}

        constexpr ~Option() = default;


        constexpr Option& operator=(std::nullopt_t) noexcept {
            m_data.operator=(std::nullopt);
            return *this;
        }

        constexpr Option& operator=(const Option& other)
            noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>) {
            if (this == std::addressof(other)) return *this;
            m_data.operator=(other.m_data);
            return *this;
        }

        constexpr Option& operator=(Option&& other)
            noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>) {
            if (this == std::addressof(other)) return *this;
            m_data.operator=(std::move(other.m_data));
            return *this;
        }

        // 使std::optional能够隐式转换为Option
        constexpr Option& operator=(const std::optional<T>& other)
            noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>) {
            m_data.operator=(other);
            return *this;
        }

        constexpr Option& operator=(std::optional<T>&& other)
            noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>) {
            m_data.operator=(std::move(other));
            return *this;
        }

        template<typename U>
            requires (std::negate<possibly_convert_assign_to_option<U>>::value &&
                      std::is_constructible_v<T, const U&> && std::is_assignable_v<T&, const U&>)
        constexpr Option& operator=(const Option<U>& other)
            noexcept(std::is_nothrow_constructible_v<T, const U&> && std::is_nothrow_assignable_v<T&, const U&>) {
            if (this == std::addressof(other)) return *this;
            m_data.opeator=(other.m_data);
            return *this;
        }

        template<typename U>
            requires (std::negate<possibly_convert_assign_to_option<U>>::value &&
                      std::is_constructible_v<T, U> && std::is_assignable_v<T&, U>)
        constexpr Option& operator=(Option<U>&& other)
            noexcept(std::is_nothrow_constructible_v<T, U> && std::is_nothrow_assignable_v<T&, U>) {
            if (this == std::addressof(other)) return *this;
            m_data.operator=(std::move(other.m_data));
            return *this;
        }

        // 使std::optional能够隐式转换为Option
        template<typename U>
            requires (std::negate<possibly_convert_assign_to_option<U>>::value &&
                      std::is_constructible_v<T, const U&> && std::is_assignable_v<T&, const U&>)
        constexpr Option& operator=(const std::optional<U>& other)
            noexcept(std::is_nothrow_constructible_v<T, const U&> && std::is_nothrow_assignable_v<T&, const U&>) {
            m_data.opeator=(other);
            return *this;
        }

        template<typename U>
            requires (std::negate<possibly_convert_assign_to_option<U>>::value &&
                      std::is_constructible_v<T, U> && std::is_assignable_v<T&, U>)
        constexpr Option& operator=(std::optional<U>&& other)
            noexcept(std::is_nothrow_constructible_v<T, U> && std::is_nothrow_assignable_v<T&, U>) {
            m_data.operator=(std::move(other));
            return *this;
        }

        template<typename U = std::remove_cv_t<T>>
            requires (!is_possibly_self_t<U>::value &&
                       std::is_constructible_v<T, U> && std::is_assignable_v<T&, U> &&
                     (!std::is_scalar_v<U> || !std::is_same_v<std::decay_t<U>, T>))
        constexpr Option& operator=(U&& value)
            noexcept(std::is_nothrow_constructible_v<T, U&&> && std::is_nothrow_assignable_v<T&, U&&>) {
            m_data.operator=(std::forward<U>(value));
            return *this;
        }


        [[nodiscard]] constexpr explicit operator std::optional<T>() const&
            noexcept(std::is_nothrow_copy_constructible_v<T>) {
            return m_data;
        }


        [[nodiscard]] constexpr explicit operator std::optional<T>() const&&
            noexcept(std::is_nothrow_move_constructible_v<T>) {
            return std::move(m_data);
        }


        [[nodiscard]] constexpr bool is_some() const noexcept {
            return m_data.has_value();
        }


        template<typename F>
            requires std::predicate<F, const T&>
        [[nodiscard]] constexpr bool is_some_and(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F, const T&> &&
                     std::is_nothrow_constructible_v<bool, std::invoke_result_t<F, const T&>>) {
            return is_some() && std::invoke(std::forward<F>(f), const_cast<const T&>(*m_data));
        }


        [[nodiscard]] constexpr bool is_none() const noexcept {
            return !m_data.has_value();
        }


        template<typename F>
            requires std::predicate<F, const T&>
        [[nodiscard]] constexpr bool is_none_or(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F, const T&> &&
                     std::is_nothrow_constructible_v<bool, std::invoke_result_t<F, const T&>>) {
            return is_none() || std::invoke(std::forward<F>(f), const_cast<const T&>(*m_data));
        }


        [[nodiscard]] constexpr T expect(const std::string_view& msg) {
            if (is_some()) return std::move(*m_data);
            call_panic_with_TN_uncheck<1>(msg, ": ");
        }


        [[nodiscard]] constexpr T expect(const std::string_view& msg) const {
            if (is_some()) return *m_data;
            call_panic_with_TN_uncheck<1>(msg, ": ");
        }


        [[nodiscard]] constexpr T unwrap() {
            if (is_some()) return std::move(*m_data);
            call_panic_with_TN_uncheck<1>("", "");
        }


        [[nodiscard]] constexpr T unwrap() const {
            if (is_some()) return *m_data;
            call_panic_with_TN_uncheck<1>("", "");
        }

        
        [[nodiscard]] constexpr T unwrap_or(T default_value)
            noexcept(std::is_nothrow_move_constructible_v<T>) {
            if (is_some()) return std::move(*m_data);
            return default_value;
        }

        
        [[nodiscard]] constexpr T unwrap_or(T default_value) const
            noexcept(std::is_nothrow_copy_constructible_v<T>) {
            if (is_some()) return *m_data;
            return default_value;
        }


        template<typename F>
            requires requires (F f) {
                { std::invoke(f) } -> std::convertible_to<T>;
            }
        [[nodiscard]] constexpr T unwrap_or(F&& f)
            noexcept(std::is_nothrow_invocable_v<F> && std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_constructible_v<T, std::invoke_result_t<F>>) {
            if (is_some()) return std::move(*m_data);
            return std::invoke(std::forward<F>(f));
        }


        template<typename F>
            requires requires (F f) {
                { std::invoke(f) } -> std::convertible_to<T>;
            }
        [[nodiscard]] constexpr T unwrap_or(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F> && std::is_nothrow_copy_constructible_v<T> &&
                     std::is_nothrow_constructible_v<T, std::invoke_result_t<F>>) {
            if (is_some()) return *m_data;
            return std::invoke(std::forward<F>(f));
        }


        [[nodiscard]] constexpr T unwrap_or()
            noexcept(std::is_nothrow_default_constructible_v<T> && std::is_nothrow_move_constructible_v<T>) {
            if (is_some()) return std::move(*m_data);
            return T();
        }


        [[nodiscard]] constexpr T unwrap_or() const
            noexcept(std::is_nothrow_default_constructible_v<T> && std::is_nothrow_copy_constructible_v<T>) {
            if (is_some()) return *m_data;
            return T();
        }


        [[nodiscard]] constexpr T unwrap_unchecked() {
            return std::move(*m_data);
        }


        [[nodiscard]] constexpr T unwrap_unchecked() const {
            return *m_data;
        }


        [[nodiscard]] constexpr T& get() {
            try {
                return m_data.value();
            } catch (...) {
                panic("Option has no value");
            }
        }


        [[nodiscard]] constexpr const T& get() const {
            try {
                return m_data.value();
            } catch (...) {
                panic("Option has no value");
            }
        }


        [[nodiscard]] constexpr T& get_uncheck() noexcept {
            return *m_data;
        }


        [[nodiscard]] constexpr const T& get_uncheck() const noexcept {
            return *m_data;
        }


        template<typename U, typename F>
            requires (requires (F f, T t) {
                { std::invoke(f, t) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<T>)
        [[nodiscard]] constexpr Option<U> map(F&& f)
            noexcept(std::is_nothrow_invocable_v<F, T> && std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_constructible_v<Option<U>, std::in_place_t, std::invoke_result_t<F, T>>) {
            if (is_some()) return Option<U>(std::in_place, std::invoke(std::forward<F>(f), std::move(*m_data)));
            return std::nullopt;
        }


        template<typename U, typename F>
            requires requires (F f, const T& t) {
                { std::invoke(f, t) } -> std::convertible_to<U>;
            }
        [[nodiscard]] constexpr Option<U> map(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F, const T&> &&
                     std::is_nothrow_constructible_v<Option<U>, std::in_place_t, std::invoke_result_t<F, const T&>>) {
            if (is_some()) return Option<U>(std::in_place, std::invoke(std::forward<F>(f), *m_data));
            return std::nullopt;
        }


        template<typename F>
            requires std::invocable<F, T&> && std::is_move_constructible_v<T>
        [[nodiscard]] constexpr Option& inspect(F&& f)
            noexcept(std::is_nothrow_invocable_v<F, T&>) {
            if (is_some()) std::invoke(std::forward<F>(f), *m_data);
            return *this;
        }


        template<typename F>
            requires std::invocable<F, const T&>
        [[nodiscard]] constexpr const Option& inspect(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F, const T&>) {
            if (is_some()) std::invoke(std::forward<F>(f), *m_data);
            return *this;
        }


        template<typename U, typename F>
            requires (requires (F f, T t) {
                { std::invoke(f, std::move(t)) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<U> && std::is_move_constructible_v<T>)
        [[nodiscard]] constexpr U map(U default_value, F&& f)
            noexcept(std::is_nothrow_invocable_v<F, T> && std::is_nothrow_move_constructible_v<U> &&
                     std::is_nothrow_constructible_v<U, std::invoke_result_t<F, T>> &&
                     std::is_nothrow_move_constructible_v<T>) {
            if (is_some()) std::invoke(std::forward<F>(f), std::move(*m_data));
            return default_value;
        }


        template<typename U, typename F>
            requires (requires (F f, const T& t) {
                { std::invoke(f, t) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<U>)
        [[nodiscard]] constexpr U map(U default_value, F&& f) const
            noexcept(std::is_nothrow_invocable_v<F, const T&> && std::is_nothrow_move_constructible_v<U> &&
                     std::is_nothrow_constructible_v<U, std::invoke_result_t<F, const T&>>) {
            if (is_some()) std::invoke(std::forward<F>(f), *m_data);
            return default_value;
        }


        template<typename U, typename D, typename F>
            requires (requires (F f, D d, T t) {
                { std::invoke(f, std::move(t)) } -> std::convertible_to<U>;
                { std::invoke(d) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<T>)
        [[nodiscard]] constexpr U map(D&& fallback, F&& f)
            noexcept(std::is_nothrow_invocable_v<F, T> && std::is_nothrow_invocable_v<D> &&
                     std::is_nothrow_move_constructible_v<U> && std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_constructible_v<U, std::invoke_result_t<F, T>> &&
                     std::is_nothrow_constructible_v<U, std::invoke_result_t<D>>) {
            if (is_some()) std::invoke(std::forward<F>(f), std::move(*m_data));
            return std::invoke(std::forward<D>(fallback));
        }


        template<typename U, typename D, typename F>
            requires (requires (F f, D d, const T& t) {
                { std::invoke(f, t) } -> std::convertible_to<U>;
                { std::invoke(d) } -> std::convertible_to<U>;
            })
        [[nodiscard]] constexpr U map(D&& fallback, F&& f) const
            noexcept(std::is_nothrow_invocable_v<F, const T&> && std::is_nothrow_invocable_v<U> &&
                     std::is_nothrow_move_constructible_v<U> &&
                     std::is_nothrow_constructible_v<U, std::invoke_result_t<F, const T&>> &&
                     std::is_nothrow_constructible_v<U, std::invoke_result_t<D>>) {
            if (is_some()) std::invoke(std::forward<F>(f), *m_data);
            return std::invoke(std::forward<D>(fallback));
        }


        template<typename E>
            requires std::is_move_constructible_v<T> && std::is_move_constructible_v<E>
        [[nodiscard]] constexpr Result<T, E> ok_or(E err)
            noexcept(std::is_nothrow_move_constructible_v<E> && std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_constructible_v<Result<T, E>, std::in_place_index_t<0>, T> &&
                     std::is_nothrow_constructible_v<Result<T, E>, std::in_place_index_t<1>, E>) {
            if (is_some()) return Result<T, E>(std::in_place_index<0>, std::move(*m_data));
            return Result<T, E>(std::in_place_index<1>, std::move(err));
        }


        template<typename E>
            requires std::is_copy_constructible_v<T> && std::is_move_constructible_v<E>
        [[nodiscard]] constexpr Result<T, E> ok_or(E err) const
            noexcept(std::is_nothrow_move_constructible_v<E> && std::is_nothrow_copy_constructible_v<T> &&
                     std::is_nothrow_constructible_v<Result<T, E>, std::in_place_index_t<0>, const T&> &&
                     std::is_nothrow_constructible_v<Result<T, E>, std::in_place_index_t<1>, E>) {
            if (is_some()) return Result<T, E>(std::in_place_index<0>, *m_data);
            return Result<T, E>(std::in_place_index<1>, std::move(err));
        }


        template<typename E, typename F>
            requires (requires (F&& err) {
                { std::invoke(err) } -> std::convertible_to<E>;
            } && std::is_move_constructible_v<T>)
        [[nodiscard]] constexpr Result<T, E> ok_or_else(F&& err)
            noexcept(std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_constructible_v<Result<T, E>, std::in_place_index_t<0>, T> &&
                     std::is_nothrow_constructible_v<Result<T, E>, std::in_place_index_t<1>, std::invoke_result_t<F>>) {
            if (is_some()) return Result<T, E>(std::in_place_index<0>, std::move(*m_data));
            return Result<T, E>(std::in_place_index<1>, std::invoke(std::forward<F>(err)));
        }


        template<typename E, typename F>
            requires (requires (F&& err) {
                { std::invoke(err) } -> std::convertible_to<E>;
            } && std::is_copy_constructible_v<T>)
        [[nodiscard]] constexpr Result<T, E> ok_or_else(F&& err) const
            noexcept(std::is_nothrow_copy_constructible_v<T> &&
                     std::is_nothrow_constructible_v<Result<T, E>, std::in_place_index_t<0>, const T&> &&
                     std::is_nothrow_constructible_v<Result<T, E>, std::in_place_index_t<1>, std::invoke_result_t<F>>) {
            if (is_some()) return Result<T, E>(std::in_place_index<0>, *m_data);
            return Result<T, E>(std::in_place_index<1>, std::invoke(std::forward<F>(err)));
        }


        template<typename U>
            requires std::is_move_constructible_v<U>
        [[nodiscard]] constexpr Option<U> and_then(Option<U> other) const
            noexcept(std::is_nothrow_move_constructible_v<U>) {
            if (is_some()) return other;
            return std::nullopt;
        }

        
        template<typename U, typename F>
            requires requires (F f, T t) {
                { std::invoke(f, std::move(t)) } -> std::convertible_to<Option<U>>;
            } && std::is_move_constructible_v<T>
        [[nodiscard]] constexpr Option<U> and_then(F&& f)
            noexcept(std::is_nothrow_invocable_v<F, T> && std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_constructible_v<Option<U>, std::invoke_result_t<F, T>>) {
            if (is_some()) std::invoke(f, std::move(*m_data));
            return std::nullopt;
        }


        template<typename U, typename F>
            requires requires (F f, const T& t) {
                { std::invoke(f, t) } -> std::convertible_to<Option<U>>;
            }
        [[nodiscard]] constexpr Option<U> and_then(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F, T> &&
                     std::is_nothrow_constructible_v<Option<U>, std::invoke_result_t<F, const T&>>) {
            if (is_some()) std::invoke(f, *m_data);
            return std::nullopt;
        }

        
        template<typename P>
            requires std::predicate<P, const T&> && std::is_move_constructible_v<T>
        [[nodiscard]] constexpr Option<T> filter(P&& predicate)
            noexcept(std::is_nothrow_invocable_v<P, const T&> && std::is_nothrow_move_constructible_v<T>) {
            if (is_none()) return std::nullopt;
            if (std::invoke(predicate, *m_data)) return std::move(*this);
            return std::nullopt;
        }


        template<typename P>
            requires std::predicate<P, const T&> && std::is_copy_constructible_v<T>
        [[nodiscard]] constexpr Option<T> filter(P&& predicate) const
            noexcept(std::is_nothrow_invocable_v<P, const T&> && std::is_nothrow_copy_constructible_v<T>) {
            if (is_none()) return std::nullopt;
            if (std::invoke(predicate, *m_data)) return *this;
            return std::nullopt;
        }


        [[nodiscard]] constexpr Option<T> or_else(Option<T> other)
            noexcept(std::is_nothrow_move_constructible_v<T>) {
            if (is_some()) return std::move(*this);
            return other;
        }


        [[nodiscard]] constexpr Option<T> or_else(Option<T> other) const
            noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_move_constructible_v<T>) {
            if (is_some()) return *this;
            return other;
        }


        template<typename F>
            requires requires (F f) {
                { std::invoke(f) } -> std::convertible_to<Option<T>>;
            } && std::is_move_constructible_v<T>
        [[nodiscard]] constexpr Option<T> or_else(F&& f)
            noexcept(std::is_nothrow_invocable_v<F> && std::is_nothrow_move_constructible_v<T> &&
                     std::is_nothrow_constructible_v<Option<T>, std::invoke_result_t<F>>) {
            if (is_some()) return std::move(*this);
            return std::invoke(std::forward<F>(f));
        }


        template<typename F>
            requires requires (F f) {
                { std::invoke(f) } -> std::convertible_to<Option<T>>;
            } && std::is_copy_constructible_v<T>
        [[nodiscard]] constexpr Option<T> or_else(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F> && std::is_nothrow_copy_constructible_v<T> &&
                     std::is_nothrow_constructible_v<Option<T>, std::invoke_result_t<F>>) {
            if (is_some()) return *this;
            return std::invoke(std::forward<F>(f));
        }


        [[nodiscard]] constexpr Option<T> xor_else(Option<T> other)
            noexcept(std::is_move_constructible_v<T>) {
            if (is_some() == other.is_some()) return std::nullopt;
            if (is_some()) return std::move(*this);
            return other;
        }

        
        [[nodiscard]] constexpr Option<T> xor_else(Option<T> other) const
            noexcept(std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>) {
            if (is_some() == other.is_some()) return std::nullopt;
            if (is_some()) return *this;
            return other;
        }


        [[nodiscard]] constexpr Option<T>& insert(T value)
            noexcept(std::is_nothrow_move_constructible_v<T>) {
            m_data.emplace(std::move(value));
            return *this;
        }


        [[nodiscard]] constexpr T& get_or_insert(T value)
            noexcept(std::is_nothrow_move_constructible_v<T>) {
            if (is_none()) m_data.emplace(std::move(value));
            return *m_data;
        }


        [[nodiscard]] constexpr T& get_or_insert()
            noexcept(std::is_nothrow_default_constructible_v<T>) {
            if (is_none()) m_data.emplace();
            return *this;
        }


        template<typename F>
        [[nodiscard]] constexpr T& get_or_insert(F&& f)
            noexcept(std::is_nothrow_invocable_v<F> &&
                     std::is_nothrow_constructible_v<T, std::invoke_result_t<F>>) {
            if (is_none()) m_data.emplace(std::invoke(std::forward<F>(f)));
            return *this;
        }


        [[nodiscard]] constexpr Option<T> take()
            noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>) {
            Option<T> ret(std::move(m_data));
            m_data.reset();
            return ret;
        }


        template<typename P>
            requires std::predicate<P, T&> && std::is_move_constructible_v<T> && std::is_destructible_v<T>
        [[nodiscard]] constexpr Option<T> take(P&& predicate)
            noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T> &&
                     std::is_nothrow_invocable_v<P, T&> &&
                     std::is_nothrow_constructible_v<bool, std::invoke_result_t<P, T&>>) {
            if (is_none()) return std::nullopt;
            if (std::invoke(predicate, *m_data)) return take();
            return std::nullopt;
        }

        [[nodiscard]] constexpr Option<T> replace(T value)
            noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>) {
            Option<T> ret(std::in_place, value);
            m_data.swap(ret.m_data);
            return ret;
        }


        template<typename U>
            requires std::is_move_constructible_v<U> && std::is_move_constructible_v<T>
        [[nodiscard]] constexpr Option<std::pair<T, U>> zip(Option<U> other)
            noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_constructible_v<U>) {
            if (is_some() && other.is_some())
                return Option<std::pair<T, U>>(std::in_place, std::move(*m_data), std::move(*(other.m_data)));
            return std::nullopt;
        }

        
        template<typename U>
            requires std::is_move_constructible_v<U> && std::is_copy_constructible_v<T>
        [[nodiscard]] constexpr Option<std::pair<T, U>> zip(Option<U> other) const
            noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_move_constructible_v<U>) {
            if (is_some() && other.is_some())
                return Option<std::pair<T, U>>(std::in_place, *m_data, std::move(*(other.m_data)));
            return std::nullopt;
        }


        template<typename U = my_tuple_element<0, T>::type, typename V = my_tuple_element<1, T>::type>
            requires std::is_move_constructible_v<U> && std::is_move_constructible_v<T>
        [[nodiscard]] constexpr std::pair<Option<U>, Option<V>> unzip() {
            if (is_none()) return std::make_pair(Option<U>(std::nullopt), Option<V>(std::nullopt));
            auto& [a, b] = *m_data;
            return std::make_pair(Option<U>(std::in_place, std::move(a)), Option<V>(std::in_place, std::move(b)));
        }


        template<typename U = my_tuple_element<0, T>::type, typename V = my_tuple_element<1, T>::type>
            requires std::is_copy_constructible_v<U> && std::is_copy_constructible_v<T>
        [[nodiscard]] constexpr std::pair<Option<U>, Option<V>> unzip() const {
            if (is_none()) return std::make_pair(Option<U>(std::nullopt), Option<V>(std::nullopt));
            auto& [a, b] = *m_data;
            return std::make_pair(Option<U>(std::in_place, a), Option<V>(std::in_place, b));
        }


        template<typename U = my_type<T>::type>
            requires std::is_copy_constructible_v<T> && std::same_as<T, std::reference_wrapper<U>>
        [[nodiscard]] constexpr Option<U> copied() const {
            if (is_none()) return std::nullopt;
            return Option<U>(std::in_place, m_data->get());
        }


        [[nodiscard]] constexpr Option<T> clone() const noexcept(std::is_nothrow_copy_constructible_v<T>) {
            return *this;
        }


        template<typename U>
            requires (std::negate<possibly_convert_assign_to_option<U>>::value &&
                      std::is_constructible_v<T, const U&> && std::is_assignable_v<T&, const U&>)
        constexpr Option& assign(const Option<U>& other)
            noexcept(std::is_nothrow_constructible_v<T, const U&> && std::is_nothrow_assignable_v<T&, const U&>) {
            if (this == std::addressof(other)) return *this;
            m_data.opeator=(other.m_data);
            return *this;
        }

        template<typename U>
            requires (std::negate<possibly_convert_assign_to_option<U>>::value &&
                      std::is_constructible_v<T, U> && std::is_assignable_v<T&, U>)
        constexpr Option& assign(Option<U>&& other)
            noexcept(std::is_nothrow_constructible_v<T, U> && std::is_nothrow_assignable_v<T&, U>) {
            if (this == std::addressof(other)) return *this;
            m_data.operator=(std::move(other.m_data));
            return *this;
        }


        [[nodiscard]] constexpr const Option& as_const() const noexcept {
            return *this;
        }


        [[nodiscard]] constexpr Option& as_mut() const noexcept {
            return const_cast<Option&>(*this);
        }


        [[nodiscard]] constexpr as_ref_t as_ref() noexcept {
            if (is_none()) return std::nullopt;
            return as_ref_t(std::in_place, std::ref(*m_data));
        }


        [[nodiscard]] constexpr as_cref_t as_ref() const noexcept {
            return as_cref();
        }


        [[nodiscard]] constexpr as_cref_t as_cref() const noexcept {
            if (is_none()) return std::nullopt;
            return as_cref_t(std::in_place, std::cref(*m_data));
        }


        [[nodiscard]] const std::optional<T>& data() const noexcept {
            return m_data;
        }

    private:
        /**
         * @brief 调用panic，同时，若类型是formattable时，打印其值
         */
        template<size_t I>
            requires (I == 0 || I == 1)
        [[noreturn]] constexpr void call_panic_with_TN_uncheck(const std::string_view& msg, const std::string_view& sep) const {
            if constexpr (I == 1) {
                auto && str = std::format("{}{}{}", msg, sep, "None");
                panic(str);
            }
            else if constexpr (
#ifdef MY_CXX23
                std::formattable<T, char>
#else // C++23 ^^^ /  vvv C++20
                requires (std::formatter<T>& f, const std::formatter<T>& cf, T&& t,
                    std::format_parse_context& p, std::format_context& fc) {
                    { f.parse(p) };
                    { cf.format(t, fc) };
                }
#endif
            ) {
                auto&& str = std::format("{}{}{}", msg, sep, *m_data);
                panic(str);
            } else {
                panic(msg);
            }
        }


    private:
        std::optional<T> m_data;
    };


    template<>
    class Option<void> {
    private:
        using as_ref_t = Option<void>;
        using as_cref_t = Option<void>;

    public:
        using value_type = void;

    public:
        constexpr Option() : m_data(std::nullopt) {}
        constexpr Option(std::nullopt_t) : m_data(std::nullopt) {}

        constexpr Option(const Option& other) noexcept : m_data(other.m_data) {}
        constexpr Option(Option&& other) noexcept : m_data(std::move(other.m_data)) {}

        // 使std::optional能够隐式转换为Option
        constexpr Option(const std::optional<std::monostate>& other) noexcept
            : m_data(other) {}
        constexpr Option(std::optional<std::monostate>&& other) noexcept
            : m_data(std::move(other)) {}

        template<typename ...Args>
            requires std::is_constructible_v<std::monostate, Args...>
        constexpr explicit Option(std::in_place_t, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<std::monostate, Args...>)
            : m_data(std::in_place, std::forward<Args>(args)...) {}

        template<typename U, typename ...Args>
            requires std::is_constructible_v<std::monostate, std::initializer_list<U>&, Args...>
        constexpr explicit Option(std::in_place_t, std::initializer_list<U> ilist, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<std::monostate, std::initializer_list<U>&, Args...>)
            : m_data(std::in_place, ilist, std::forward<Args>(args)...) {}

        constexpr ~Option() = default;

        constexpr Option& operator=(std::nullopt_t) noexcept {
            m_data.operator=(std::nullopt);
            return *this;
        }

        constexpr Option& operator=(const Option& other) noexcept {
            if (this == std::addressof(other)) return *this;
            m_data.operator=(other.m_data);
            return *this;
        }

        constexpr Option& operator=(Option&& other) noexcept {
            if (this == std::addressof(other)) return *this;
            m_data.operator=(std::move(other.m_data));
            return *this;
        }


        [[nodiscard]] constexpr explicit operator std::optional<std::monostate>() const& noexcept {
            return m_data;
        }

        [[nodiscard]] constexpr explicit operator std::optional<std::monostate>() const&& noexcept {
            return std::move(m_data);
        }


        [[nodiscard]] constexpr bool is_some() const noexcept {
            return m_data.has_value();
        }


        template<typename F>
            requires std::predicate<F>
        [[nodiscard]] constexpr bool is_some_and(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F> &&
                     std::is_nothrow_constructible_v<bool, std::invoke_result_t<F>>) {
            return is_some() && std::invoke(std::forward<F>(f));
        }


        [[nodiscard]] constexpr bool is_none() const noexcept {
            return !m_data.has_value();
        }


        template<typename F>
            requires std::predicate<F>
        [[nodiscard]] constexpr bool is_none_or(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F> &&
                     std::is_nothrow_constructible_v<bool, std::invoke_result_t<F>>) {
            return is_none() || std::invoke(std::forward<F>(f));
        }


        constexpr void expect(const std::string_view& msg) const {
            if (is_some()) return;
            call_panic_with_TN_uncheck<1>(msg, ": ");
        }


        constexpr void unwrap() const {
            if (is_some()) return;
            call_panic_with_TN_uncheck<1>("", "");
        }


        constexpr void unwrap_or() const noexcept {
            return;
        }

        template<typename F>
            requires requires (F f) {
                { std::invoke(f) };
            }
        constexpr void unwrap_or(F&& f) const noexcept(std::is_nothrow_invocable_v<F>) {
            if (is_some()) return;
            std::invoke(std::forward<F>(f));
            return;
        }


        constexpr void unwrap_unchecked() const {
            return;
        }


        constexpr const void get() const {
            try {
                return;
            } catch (...) {
                panic("Option has no value");
            }
        }

        constexpr const void get_uncheck() const noexcept {
            return;
        }


        template<typename U, typename F>
            requires requires (F f) {
                { std::invoke(f) } -> std::convertible_to<U>;
            }
        [[nodiscard]] constexpr Option<U> map(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F> &&
                     std::is_nothrow_constructible_v<Option<U>, std::in_place_t, std::invoke_result_t<F>>) {
            if (is_some()) return Option<U>(std::in_place, std::invoke(std::forward<F>(f)));
            return std::nullopt;
        }


        template<typename F>
            requires std::invocable<F>
        [[nodiscard]] constexpr const Option& inspect(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F>) {
            if (is_some()) std::invoke(std::forward<F>(f));
            return *this;
        }


        template<typename U, typename F>
            requires (requires (F f) {
                { std::invoke(f) } -> std::convertible_to<U>;
            } && std::is_move_constructible_v<U>)
        [[nodiscard]] constexpr U map(U default_value, F&& f) const
            noexcept(std::is_nothrow_invocable_v<F> && std::is_nothrow_move_constructible_v<U> &&
                     std::is_nothrow_constructible_v<U, std::invoke_result_t<F>>) {
            if (is_some()) std::invoke(std::forward<F>(f));
            return default_value;
        }


        template<typename U, typename D, typename F>
            requires (requires (F f, D d) {
                { std::invoke(f) } -> std::convertible_to<U>;
                { std::invoke(d) } -> std::convertible_to<U>;
            })
        [[nodiscard]] constexpr U map(D&& fallback, F&& f) const
            noexcept(std::is_nothrow_invocable_v<F> && std::is_nothrow_invocable_v<U> &&
                     std::is_nothrow_move_constructible_v<U> &&
                     std::is_nothrow_constructible_v<U, std::invoke_result_t<F>> &&
                     std::is_nothrow_constructible_v<U, std::invoke_result_t<D>>) {
            if (is_some()) std::invoke(std::forward<F>(f));
            return std::invoke(std::forward<D>(fallback));
        }


        template<typename E>
            requires std::is_move_constructible_v<E>
        [[nodiscard]] constexpr Result<void, E> ok_or(E err) const
            noexcept(std::is_nothrow_move_constructible_v<E> &&
                     std::is_nothrow_constructible_v<Result<void, E>, std::in_place_index_t<0>> &&
                     std::is_nothrow_constructible_v<Result<void, E>, std::in_place_index_t<1>, E>) {
            if (is_some()) return Result<void, E>(std::in_place_index<0>);
            return Result<void, E>(std::in_place_index<1>, std::move(err));
        }


        template<typename E, typename F>
            requires (requires (F&& err) {
                { std::invoke(err) } -> std::convertible_to<E>;
            })
        [[nodiscard]] constexpr Result<void, E> ok_or_else(F&& err) const
            noexcept(std::is_nothrow_constructible_v<Result<void, E>, std::in_place_index_t<0>> &&
                     std::is_nothrow_constructible_v<Result<void, E>, std::in_place_index_t<1>, std::invoke_result_t<F>>) {
            if (is_some()) return Result<void, E>(std::in_place_index<0>);
            return Result<void, E>(std::in_place_index<1>, std::invoke(std::forward<F>(err)));
        }


        template<typename U>
            requires std::is_move_constructible_v<U>
        [[nodiscard]] constexpr Option<U> and_then(Option<U> other) const
            noexcept(std::is_nothrow_move_constructible_v<U>) {
            if (is_some()) return other;
            return std::nullopt;
        }


        template<typename U, typename F>
            requires requires (F f) {
                { std::invoke(f) } -> std::convertible_to<Option<U>>;
            }
        [[nodiscard]] constexpr Option<U> and_then(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F> &&
                     std::is_nothrow_constructible_v<Option<U>, std::invoke_result_t<F>>) {
            if (is_some()) std::invoke(f);
            return std::nullopt;
        }


        template<typename P>
            requires std::predicate<P>
        [[nodiscard]] constexpr Option<void> filter(P&& predicate) const
            noexcept(std::is_nothrow_invocable_v<P>) {
            if (is_none()) return std::nullopt;
            if (std::invoke(predicate)) return *this;
            return std::nullopt;
        }


        [[nodiscard]] constexpr Option<void> or_else(Option<void> other) const noexcept {
            if (is_some()) return *this;
            return other;
        }


        template<typename F>
            requires requires (F f) {
                { std::invoke(f) } -> std::convertible_to<Option<void>>;
            }
        [[nodiscard]] constexpr Option<void> or_else(F&& f) const
            noexcept(std::is_nothrow_invocable_v<F> &&
                     std::is_nothrow_constructible_v<Option<void>, std::invoke_result_t<F>>) {
            if (is_some()) return *this;
            return std::invoke(std::forward<F>(f));
        }


        [[nodiscard]] constexpr Option<void> xor_else(Option<void> other) const noexcept {
            if (is_some() == other.is_some()) return std::nullopt;
            if (is_some()) return *this;
            return other;
        }


        [[nodiscard]] constexpr Option<void>& insert() noexcept {
            m_data.emplace();
            return *this;
        }


        constexpr void get_or_insert()
            noexcept {
            m_data.emplace();
            return;
        }


        template<typename F>
        constexpr void get_or_insert(F&& f)
            noexcept(std::is_nothrow_invocable_v<F>) {
            if (is_some()) return;
            std::invoke(std::forward<F>(f));
            m_data.emplace();
        }


        [[nodiscard]] constexpr Option<void> take() noexcept {
            Option<void> ret(*this);
            m_data.reset();
            return ret;
        }


        template<typename P>
            requires std::predicate<P>
        [[nodiscard]] constexpr Option<void> take(P&& predicate)
            noexcept(std::is_nothrow_invocable_v<P> &&
                     std::is_nothrow_constructible_v<bool, std::invoke_result_t<P>>) {
            if (is_none()) return std::nullopt;
            if (std::invoke(predicate)) return take();
            return std::nullopt;
        }


        [[nodiscard]] constexpr Option<void> replace() noexcept {
            Option<void> ret(std::in_place);
            m_data.swap(ret.m_data);
            return ret;
        }


        [[nodiscard]] constexpr Option<void> clone() const noexcept {
            return *this;
        }


        [[nodiscard]] constexpr const Option& as_const() const noexcept {
            return *this;
        }


        [[nodiscard]] constexpr Option& as_mut() const noexcept {
            return const_cast<Option&>(*this);
        }


        [[nodiscard]] constexpr as_ref_t as_ref() noexcept {
            if (is_none()) return std::nullopt;
            return as_ref_t(std::in_place);
        }


        [[nodiscard]] constexpr as_cref_t as_ref() const noexcept {
            return as_cref();
        }


        [[nodiscard]] constexpr as_cref_t as_cref() const noexcept {
            if (is_none()) return std::nullopt;
            return as_cref_t(std::in_place);
        }


        [[nodiscard]] const std::optional<std::monostate>& data() const noexcept {
            return m_data;
        }

    private:
        /**
         * @brief 调用panic，同时，若类型是formattable时，打印其值
         */
        template<size_t I>
            requires (I == 0 || I == 1)
        [[noreturn]] constexpr void call_panic_with_TN_uncheck(const std::string_view& msg, const std::string_view& sep) const {
            if constexpr (I == 1) {
                auto && str = std::format("{}{}{}", msg, sep, "None");
                panic(str);
            }
            else {
                auto&& str = std::format("{}{}{}", msg, sep, "void");
                panic(str);
            }
        }

    private:
        std::optional<std::monostate> m_data;
    };



    template<class T>
    struct option_transform {
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
    using option_transform_t = option_transform<T>::type;

    template<typename T>
        requires std::move_constructible<option_transform_t<T>>
    [[nodiscard]] constexpr Option<option_transform_t<T>> None() noexcept {
        return Option<option_transform_t<T>>(std::nullopt);
    }

    template<typename T>
        requires std::constructible_from<option_transform_t<T>, T&&>
    [[nodiscard]] constexpr Option<option_transform_t<T>> Some(T&& value)
        noexcept(std::is_nothrow_constructible_v<Option<option_transform_t<T>>, std::in_place_t, T&&>) {
        return Option<option_transform_t<T>>(std::in_place, std::forward<T>(value));
    }

    template<typename T, typename ...Args>
        requires std::constructible_from<option_transform_t<T>, Args&&...>
    [[nodiscard]] constexpr Option<option_transform_t<T>> Some(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<Option<option_transform_t<T>>, std::in_place_t, Args&&...>) {
        return Option<option_transform_t<T>>(std::in_place, std::forward<Args>(args)...);
    }

    template<typename T, typename U, typename ...Args>
        requires std::constructible_from<option_transform_t<T>, std::initializer_list<U>&, Args&&...>
    [[nodiscard]] constexpr Option<option_transform_t<T>> Some(std::initializer_list<U>& ini, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<Option<option_transform_t<T>>,
                std::in_place_t, std::initializer_list<U>&, Args&&...>) {
        return Option<option_transform_t<T>>(std::in_place, ini, std::forward<Args>(args)...);
    }

}

#endif // MY_CXX20
#endif // C163Q_MY_CPP_UTILS_RS_OPTION_HPP
