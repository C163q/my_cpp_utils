#include<cstdio>
#include<cstdlib>
#include<source_location>
#include<string_view>
#include"../../include/rs/panic.hpp"
#include"../../include/core/config.hpp"
#ifdef MY_CXX23
#include<print>
#include<stacktrace>
#else
#include<iostream>
#endif

namespace C163q {
    bool enable_traceback = false;

#ifdef MY_CXX23
    [[noreturn]] void call_panic_(const std::string_view& message,
            const std::source_location location) noexcept {
        if (location.function_name())
            println(stderr, "panicked at {} in function {}:{}:{}:\n{}", location.file_name(),
                    location.function_name(), location.line(), location.column(), message);
        else
            println(stderr, "panicked at {}:{}:{}:\n{}", location.file_name(),
                    location.line(), location.column(), message);
        if (enable_traceback) {
            println("{}", std::stacktrace::current());
        }
#else
    [[noreturn]] void call_panic_(const std::string_view& message,
            const std::source_location location) noexcept {
        if (location.function_name())
            std::cerr << "panicked at " << location.file_name() << " in function "
                << location.function_name() << ":" << location.line() << ":"
                << location.column() << ":\n" << message << std::endl;
        else
            std::cerr << "panicked at " << location.file_name() << ":"
                << location.line() << ":" << location.column() << ":\n"
                << message << std::endl;
#endif
        abort();
    }
}



