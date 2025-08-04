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

#include"panic.hpp"
#include<algorithm>
#include<concepts>
#include<functional>
#include<initializer_list>
#include<memory>
#include<optional>
#include<string_view>
#include<type_traits>
#include<utility>

namespace C163q {
    template<typename T>
        requires (std::is_object_v<T> && !std::is_array_v<T> &&
                 !std::is_same_v<std::remove_cv_t<T>, std::nullopt_t> &&
                 !std::is_same_v<std::remove_cv_t<T>, std::in_place_t>)
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

        [[nodiscard]] constexpr T unwrap_unchecked() {
            return std::move(*m_data);
        }

        [[nodiscard]] constexpr T unwrap_unchecked() const {
            return *m_data;
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
;

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
}

#endif // MY_CXX20
#endif // C163Q_MY_CPP_UTILS_RS_OPTION_HPP
