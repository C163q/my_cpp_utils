
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
#if !defined (MY_CXX23) && !defined (MYCXX20) && !defined (MY_CXX17)
    static_assert(false, "At least C++17!");
#endif // !defined (MY_CXX23) && !defined (MYCXX20) && !defined (MY_CXX17)

