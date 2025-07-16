#include<cstdio>
#include<cstdlib>
#include<source_location>
#include<string_view>
#include<print>
#include"panic.hpp"
#include<stacktrace>

namespace C163q {
    bool enable_traceback = false;

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
        abort();
    }
}



