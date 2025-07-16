/**
 * @file format/span.hpp
 * @brief 为std::span类提供std::formatter的实现
 */

#include<algorithm>
#include<cstddef>
#include<format>
#include<string>
#include<string_view>
#include<utility>
#include<span>


/**
 * @brief 为标准库std::formatter实现std::span的特化。
 *
 * 对于一个std::span其格式化字符串的形式如下：
 * 
 * 使用'|'表示格式化字符串中，前面部分是用于控制span格式的，后半部分是用于控制其中每个元素的格式的
 * 当不存在'|'时，默认认为整个格式化字符串都是用于控制span的。
 * 
 * 例如：对于std::span<float>的格式化字符串"{:v\nv|.5f}"，
 * 前面部分"<\n<"用于控制span中元素之间的分割符为"\n"，后面部分".5f"用于控制每个float元素的格式。
 *
 * 使用'<'来表示设置打印第一个元素前打印的字符串，默认为"["，
 * 使用另一个'<'结束打印字符串的设置，如果要使用字符'<'，请使用"\<"。（一个\字符和一个<字符）
 *
 * 使用'>'来表示设置打印最后一个元素后打印的字符串，默认为"]"，
 * 使用另一个'>'结束打印字符串的设置，如果要使用字符'>'，请使用"\>"。
 *
 * 使用'v'来表示设置元素之间的分割符，默认为", "（一个,字符和一个空格），
 * 使用另一个'v'结束打印字符串的设置，如果要使用字符'v'，请使用"\v"。
 *
 * 此外，如果要使用字符'|'，可以使用"\|"。
 *
 * @example
 * ```cpp
 * std::vector v { 1, 2, 3, 4, 5, 6 };
 * assert(std::format("{}", std::span(v))
 *         == std::string_view("[1, 2, 3, 4, 5, 6]"));
 *
 * int arr1[10] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
 * assert(std::format("{:<\\< <v \\| v> \\>>}", std::span(arr1))
 *         == std::string_view("< 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 >"));
 *
 * std::array arr2 { 1.0, 2.0, 3.0 };
 * assert(std::format("{:<[\n\t<v\n\tv>\n]>|.6f}", std::span(arr2))
 *         == std::string_view("[\n"
 *             "\t1.000000\n"
 *             "\t2.000000\n"
 *             "\t3.000000\n"
 *             "]"));
 *
 * std::string str = "abc";
 * assert(std::format(L"{}", std::span(str.begin(), str.end()))
 *         == std::wstring_view(L"[a, b, c]"));
 * ```
 */
template<typename T, size_t Extend, typename CharT>
class std::formatter<std::span<T, Extend>, CharT> {
public:
    template<typename ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx) {
        // '|'前的部分的格式化处理
        auto [iter, element_format] = span_format_parse(ctx, ctx.begin());
        if (!element_format) return iter;
        basic_format_parse_context<CharT> element_format_string
            { std::basic_string_view<CharT> { iter, ctx.end() } };
        return element_formatter.parse(element_format_string);  // 委托给元素的formatter
    }
    
    template<typename FormatContext>
    FormatContext::iterator format(const std::span<T, Extend>& value, FormatContext& ctx) const {
        auto iter = ctx.out();
        iter = std::ranges::copy(border_begin, iter).out;
        for (size_t i = 0; i < value.size(); ++i) {
            iter = element_formatter.format(value[i], ctx); // 使用元素的formatter
            if (i != value.size() - 1) {    // value.size() != 0 here
                // 不为最后一个元素后面添加分隔符
                iter = std::ranges::copy(separator, iter).out;
            }
        }
        return std::ranges::copy(border_end, iter).out;
    }

private:
    /**
     * 解析有关span的格式，而不解析元素的格式
     * @return 返回两个变量，ParseContext::iterator是ParseContext解析后的尾迭代器
     *         bool是是否有'|'标识符，如果有为true，意味着需要解析元素格式
     */
    template<typename ParseContext>
    constexpr std::pair<typename ParseContext::iterator, bool>
    span_format_parse(ParseContext& ctx, ParseContext::iterator iter) {
        while (iter != ctx.end()) {
            switch (*iter) {
            case CharT('|'):
                ++iter;
                return { iter, true };
            case CharT('}'):
                return { iter, false };
            case CharT('<'):
                ++iter;
                span_text_format(CharT('<'), border_begin, ctx, iter);
                break;
            case CharT('>'):
                ++iter;
                span_text_format(CharT('>'), border_end, ctx, iter);
                break;
            case CharT('v'):
                ++iter;
                span_text_format(CharT('v'), separator, ctx, iter);
                break;
            default:
                ++iter;
                break;
            }
        }
        return { iter, false };
    }

    /**
     * 用于对分隔符、前边界、后边界进行解析的通用函数。
     *
     * @warning         传入的iter应当指向indicator的下一个位置！
     * 
     * @param indicator 表明指示解析结束的字符。
     *                  例如'<'，则遇到'<'时返回指向后一个的迭代器，遇到"\<"时继续解析。
     * @param aim       决定最终将解析的字符串结果放入到哪个字符串变量中。
     * @param ctx       parse中的ctx。
     * @param iter      ctx迭代器内容的迭代器，从中开始解析，由于是引用，因此会修改迭代器本身。
     * @return          即参数iter。
     */
    template<typename ParseContext>
    constexpr ParseContext::iterator& span_text_format(
            CharT indicator,
            std::basic_string_view<CharT>& view,
            ParseContext& ctx,
            ParseContext::iterator& iter) {
        bool ignore = false;
        auto iter_begin = storage.end();
        while (iter != ctx.end()) {
            if ((*iter == indicator) && !ignore) {
                ++iter;
                break;
            }
            if ((*iter == CharT('\\')) && !ignore) {
                ++iter;
                ignore = true;
                continue;
            }
            storage.push_back(*iter);
            ++iter;
            ignore = false;
        }
        view = std::basic_string_view<CharT>(iter_begin, storage.end());
        return iter;
    }

public:
    // 默认设置
    constexpr static CharT default_separator    [3] { CharT(','), CharT(' '), 0 };
    constexpr static CharT default_border_begin [2] { CharT('['), 0 };
    constexpr static CharT default_border_end   [2] { CharT(']'), 0 };
    
private:
    std::formatter<T, CharT>        element_formatter;
    std::basic_string_view<CharT>   separator{ default_separator };
    std::basic_string_view<CharT>   border_begin{ default_border_begin };
    std::basic_string_view<CharT>   border_end{ default_border_end };
    std::basic_string<CharT>        storage;
};


