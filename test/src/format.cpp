#include "../../src/format/vector.hpp"
#include "../../src/format/span.hpp"
#include <array>
#include <cassert>
#include <format>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>


int main() {
    {
        assert(std::format("{}", std::vector{ 1, 2, 3, 4, 5, 6 })
                == std::string_view("[1, 2, 3, 4, 5, 6]"));

        assert(std::format("{:<\\< <v \\| v> \\>>}", std::vector{ 1, 2, 3, 4, 5, 6 })
                == std::string_view("< 1 | 2 | 3 | 4 | 5 | 6 >"));

        assert(std::format("{:<[\n\t<v\n\tv>\n]>|.6f}", std::vector{ 1.0, 2.0, 3.0 })
                == std::string_view("[\n"
                    "\t1.000000\n"
                    "\t2.000000\n"
                    "\t3.000000\n"
                    "]"));

        assert(std::format(L"{}", std::vector{ 'a', 'b', 'c' })
                == std::wstring_view(L"[a, b, c]"));
    }
    {
        std::vector v { 1, 2, 3, 4, 5, 6 };
        assert(std::format("{}", std::span(v))
                == std::string_view("[1, 2, 3, 4, 5, 6]"));

        int arr1[10] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        assert(std::format("{:<\\< <v \\| v> \\>>}", std::span(arr1))
                == std::string_view("< 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 >"));

        std::array arr2 { 1.0, 2.0, 3.0 };
        assert(std::format("{:<[\n\t<v\n\tv>\n]>|.6f}", std::span(arr2))
                == std::string_view("[\n"
                    "\t1.000000\n"
                    "\t2.000000\n"
                    "\t3.000000\n"
                    "]"));

        std::string str = "abc";
        assert(std::format(L"{}", std::span(str.begin(), str.end()))
                == std::wstring_view(L"[a, b, c]"));
    }

}


