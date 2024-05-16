#pragma once

#include <vector>

template<size_t N>
struct template_string
{
    constexpr template_string(const char (&str)[N])
    {
        std::copy_n(str, N, value);
    }
    
    char value[N];
};

template<typename T>
constexpr size_t vector_element_size(const std::vector<T> &v)
{
    return sizeof(T) * v.size();
}

namespace detail
{
    template<typename M> struct extract_member_type;
    template<typename R, class C> struct extract_member_type<R C::*> { using type = R; };

    template<typename M> struct extract_class_type;
    template<typename R, class C> struct extract_class_type<R C::*> { using type = C; };

    template<typename M>
    using extract_class_type_t = typename extract_class_type<M>::type;

    template<typename M>
    using extract_member_type_t = typename extract_member_type<M>::type;
}

template<typename It>     // It models LegacyRandomAccessIterator
constexpr auto cut_paste(It cut_begin, It cut_end, It paste_begin) -> std::pair<It, It>       // return the final location of the range [cut_begin, cut_end)
{
    if (paste_begin < cut_begin)  // handles left-rotate(case #1)
    {
        auto const updated_cut_begin = paste_begin;
        auto const updated_cut_end = std::rotate(paste_begin, cut_begin, cut_end);
        return { updated_cut_begin, updated_cut_end };
    }

    if (cut_end < paste_begin) // handles right-rotate(case #2)
    {
        // Reinterpreting the right-rotate as a left rotate
        auto const updated_cut_begin = std::rotate(cut_begin, cut_end, paste_begin);
        auto const updated_cut_end = paste_begin;
        return { updated_cut_begin, updated_cut_end };
    }
    // else - no-operation required, there will be no change in the relative arrangement of data

    return { cut_begin, cut_end }; // (case #3)
}