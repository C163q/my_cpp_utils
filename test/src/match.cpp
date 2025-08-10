#include"../../include/rs/match.hpp"
#include<cassert>
#include<variant>
#include<string_view>

int main() {
    {
        std::variant<int, double, const char*> v { 1.0 };
        std::string_view ret = C163q::match(v,
                [] (int i) { return "int"; },
                [] (double d) { return "double"; },
                [] (const char* p) { return "const char*"; }
                );
        assert(ret == "double");
    }
    {
        std::variant<int, double, const char*> v { 1.0 };
        std::string_view ret = C163q::match(v,
                [] (int i) { return "int"; },
                [] (auto d) { return "not int"; }
                );
        assert(ret == "not int");
    }

}






