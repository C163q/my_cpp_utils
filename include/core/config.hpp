/*!
 * @file core/config.hpp
 * @brief 配置文件
 *
 * 至少需要C++17
 *
 * @since Jul 31, 2025
 */

#ifndef C163Q_MY_CPP_UTILS_CORE_CONFIG_HPP
#define C163Q_MY_CPP_UTILS_CORE_CONFIG_HPP


// MSVC中__cplusplus是非标准的，恒为199711L，因此必须使用_MSVC_LANG
// 早期的MSVC不存在_MSVC_LANG，此时使用__cplusplus，这时直接不通过编译
// 对于其他编译器，使用__cplusplus作为MY_CXX_VERSION
#ifdef _MSVC_LANG
    #define MY_CXX_VERSION _MSVC_LANG
#elif !defined(MY_CXX_VERSION)
    #define MY_CXX_VERSION __cplusplus
#endif


#if MY_CXX_VERSION >= 202302L
    #define MY_CXX23
#endif
#if MY_CXX_VERSION >= 202002L
    #define MY_CXX20
#endif
#if MY_CXX_VERSION >= 201703L
    #define MY_CXX17
#endif
// 对于C++版本低于C++17的，直接编译不通过
#if !defined (MY_CXX17)
    static_assert(false, "At least C++17!");
#endif // !defined (MY_CXX17)


#endif // !C163Q_MY_CPP_UTILS_CORE_CONFIG_HPP
