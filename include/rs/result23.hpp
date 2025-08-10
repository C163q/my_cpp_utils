/*!
 * @file rs/result.hpp
 * @brief 在C++中实现rust当中的Result类
 *
 * 至少需要C++23
 *
 * @since Aug 4, 2025
 */

#ifndef C163Q_MY_CPP_UTILS_RS_RESULT23_HPP
#define C163Q_MY_CPP_UTILS_RS_RESULT23_HPP

#include"../core/config.hpp"
#ifndef MY_CXX23
    static_assert(false, "Require C++23!");
#else

// TODO: 实现使用std::expect的Result

#include"result20.hpp"

#endif // MY_CXX23
#endif // C163Q_MY_CPP_UTILS_RS_RESULT23_HPP
