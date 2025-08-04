/*!
 * @file rs/result.hpp
 * @brief 在C++中实现rust当中的Result类
 *
 * 至少需要C++20。如果是C++23，则使用std::expected版本的Result
 *
 * @since Aug 4, 2025
 */

#ifndef C163Q_MY_CPP_UTILS_RS_RESULT_HPP
#define C163Q_MY_CPP_UTILS_RS_RESULT_HPP

#include"../core/config.hpp"
#ifndef MY_CXX20
    static_assert(false, "Require C++20!");
#else
#ifndef MY_CXX23
#include"result20.hpp"
#else
#include"result23.hpp"
#endif // !MY_CXX23
#endif // MY_CXX20
#endif // C163Q_MY_CPP_UTILS_RS_RESULT_HPP
