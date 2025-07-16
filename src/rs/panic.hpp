/**
 * @file rs/panic.hpp
 * 在C++中实现rust中panic!宏的效果
 */

#include<string_view>
#include<source_location>

namespace C163q {

    /**
     * @brief 决定在panic中是否显示栈回溯，默认为false
     */
    extern bool enable_traceback;

    /**
     * @brief 使程序因不可恢复错误而崩溃。
     *
     * 该函数一般使用panic宏间接被调用
     *
     * @param message  程序崩溃时打印的错误信息
     * @param location 程序调用该函数时的上下文信息
     *
     * @warning 可能需要使用-lstdc++_libbacktrace或-lstdc++exp来启用栈追踪
     */
    [[noreturn]] void call_panic_(const std::string_view& message,
            const std::source_location location = std::source_location::current()) noexcept;
}

/**
 * @brief 使程序因不可恢复错误而崩溃。
 *
 * @param message 程序崩溃时打印的错误信息
 *
 * @warning 可能需要使用-lstdc++_libbacktrace或-lstdc++exp来启用栈追踪
 */
#define panic(message) do { ::C163q::call_panic_(message); } while(0)


