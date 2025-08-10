#include"../../include/rs/panic.hpp"

int main() {
    C163q::enable_traceback = true;
    panic("Error and abort!");
}

// USAGE: g++ -std=c++23 -o build/panic test/src/panic.cpp src/rs/panic.cpp -lstdc++exp
// OUTPUT:
// panicked at test/src/panic.cpp in function int main():5:5:
// Error and abort!
//    0# C163q::call_panic_(std::basic_string_view<char, std::char_traits<char> > const&, std::source_location) at :0
//    1# main at :0
//    2# __libc_start_call_main at ../sysdeps/nptl/libc_start_call_main.h:58
//    3# __libc_start_main_impl at ../csu/libc-start.c:360
//    4# _start at :0
//    5# 



