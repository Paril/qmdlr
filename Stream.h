#pragma once

#include <istream>
#include <ostream>
#include <iostream>
#include <bit>

// Binary streams; by default, streams use the native endianness
// (unchanged bytes) but can be changed to a specific endianness
// with the manipulator below.
namespace detail
{
// 0 is the default for iwords
enum class st_en : long
{
    na = 0,
    le = 1,
    be = 2,
};

inline int32_t endian_i()
{
    static int32_t i = std::ios_base::xalloc();
    return i;
}

inline bool need_swap(std::ios_base &os)
{
    st_en e = static_cast<st_en>(os.iword(endian_i()));

    // if we're in a "default state" of native endianness, we never
    // need to swap.
    if (e == st_en::na)
        return false;

    return (static_cast<int32_t>(e) - 1) != static_cast<int32_t>(std::endian::native);
}

template<typename T>
inline std::ostream &write_swapped(std::ostream &s, const T &val)
{
    const char *pVal = reinterpret_cast<const char *>(&val);

    if (need_swap(s))
    {
        for (int32_t i = sizeof(T) - 1; i >= 0; i--)
            s.write(&pVal[i], 1);
    }
    else
        s.write(pVal, sizeof(T));

    return s;
}

template<typename T>
inline std::istream &read_swapped(std::istream &s, T &val)
{
    char *pRetVal = reinterpret_cast<char *>(&val);

    if (need_swap(s))
    {
        for (int32_t i = sizeof(T) - 1; i >= 0; i--)
            s.read(&pRetVal[i], 1);
    }
    else
        s.read(pRetVal, sizeof(T));

    return s;
}
} // namespace detail

template<std::endian e>
inline std::ios_base &endianness(std::ios_base &os)
{
    os.iword(detail::endian_i()) = static_cast<int32_t>(e) + 1;
    return os;
}

// blank type used for paddings
template<size_t n>
struct padding
{
};

struct padding_n
{
    size_t n;

    constexpr padding_n(size_t np)
        : n(np)
    {
    }
};

template<typename T>
concept IsStreamBinaryWriteCapable = requires(const T &a, std::ostream &o)
{
    { o <= a };
};

template<typename T>
concept IsStreamBinaryReadCapable = requires(T &a, std::istream &i)
{
    { i >= a };
};

template<size_t n>
inline std::ostream &operator<=(std::ostream &s, const padding<n> &)
{
    for (size_t i = 0; i < n; i++)
        s.put(0);

    return s;
}

inline std::ostream &operator<=(std::ostream &s, const padding_n &p)
{
    for (size_t i = 0; i < p.n; i++)
        s.put(0);

    return s;
}

template<size_t n>
inline std::istream &operator>=(std::istream &s, padding<n> &)
{
    s.seekg(n, std::ios_base::cur);

    return s;
}

template<size_t n>
inline std::istream &operator>=(std::istream &s, padding_n &p)
{
    s.seekg(p.n, std::ios_base::cur);

    return s;
}

template<typename T>
struct is_trivially_streamable : public std::false_type {};

template<typename T>
concept IsDefaultTriviallyStreamable = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>;

template<IsDefaultTriviallyStreamable T>
struct is_trivially_streamable<T> : public std::true_type {};

template<typename T>
inline constexpr bool is_trivially_streamable_v = is_trivially_streamable<T>::value;

template<typename T>
concept IsTriviallyStreamable = is_trivially_streamable_v<T>;

template<IsTriviallyStreamable T>
inline std::istream &operator>=(std::istream &stream, T &value)
{
	return detail::read_swapped(stream, value);
}

template<IsTriviallyStreamable T>
inline std::ostream &operator<=(std::ostream &stream, const T &value)
{
	return detail::write_swapped(stream, value);
}

// string stuff
enum class str_style : long
{
    unset = 0, // initial value (same as pr64)
    sz = 1,    // null terminated
    pr8 = 2,   // length-prefixed (8-bit)
    pr16 = 3,  // length-prefixed (16-bit)
    pr32 = 4,  // length-prefixed (32-bit)
    pr64 = 5,  // length-prefixed (64-bit)
};

#include <string>

namespace detail
{
inline int32_t stringstyle_i()
{
    static int32_t i = std::ios_base::xalloc();
    return i;
}

inline str_style stringstyle(std::ios_base &os)
{
    return static_cast<str_style>(os.iword(stringstyle_i()));
}

template<typename T>
inline void string_read(std::istream &stream, std::string &value)
{
    T size;
    stream >= size;
    value.resize(static_cast<T>(size));
    stream.read(value.data(), size);
}

template<typename T>
inline void string_write(std::ostream &stream, const char *value, size_t len)
{
    if (len > std::numeric_limits<T>::max())
        throw std::runtime_error("string exceeds PR limit");

    stream <= static_cast<T>(len);
    stream.write(value, len);
}
} // namespace detail

template<str_style s>
inline std::ios_base &stringstyle(std::ios_base &os)
{
    os.iword(detail::stringstyle_i()) = static_cast<int32_t>(s);
    return os;
}

inline std::ostream &operator<=(std::ostream &stream, const char *value)
{
    str_style style = detail::stringstyle(stream);

    if (style == str_style::unset)
        style = str_style::pr64;

    size_t len = strlen(value);

    if (style == str_style::sz)
    {
        stream.write(value, len);
        stream.write("\0", 1);
    }
    else if (style == str_style::pr8)
        detail::string_write<uint8_t>(stream, value, len);
    else if (style == str_style::pr16)
        detail::string_write<uint16_t>(stream, value, len);
    else if (style == str_style::pr32)
        detail::string_write<uint32_t>(stream, value, len);
    else if (style == str_style::pr64)
        detail::string_write<uint64_t>(stream, value, len);

    return stream;
}


inline std::istream &operator>=(std::istream &stream, std::string &value)
{
    str_style style = detail::stringstyle(stream);

    if (style == str_style::unset)
        style = str_style::pr64;

    if (style == str_style::sz)
    {
        value.clear();

        char c = '\0';

        while (stream.good())
        {
            stream.read(&c, 1);

            if (!stream.good() || c == '\0')
                break;

            value.push_back(c);
        }
    }
    else if (style == str_style::pr8)
        detail::string_read<uint8_t>(stream, value);
    else if (style == str_style::pr16)
        detail::string_read<uint16_t>(stream, value);
    else if (style == str_style::pr32)
        detail::string_read<uint32_t>(stream, value);
    else if (style == str_style::pr64)
        detail::string_read<uint64_t>(stream, value);

    return stream;
}

inline std::ostream &operator<=(std::ostream &stream, const std::string &value)
{
    str_style style = detail::stringstyle(stream);

    if (style == str_style::unset)
        style = str_style::pr64;

    if (style == str_style::sz)
    {
        stream.write(value.c_str(), value.size());
        stream.write("\0", 1);
    }
    else if (style == str_style::pr8)
        detail::string_write<uint8_t>(stream, value.c_str(), value.size());
    else if (style == str_style::pr16)
        detail::string_write<uint16_t>(stream, value.c_str(), value.size());
    else if (style == str_style::pr32)
        detail::string_write<uint32_t>(stream, value.c_str(), value.size());
    else if (style == str_style::pr64)
        detail::string_write<uint64_t>(stream, value.c_str(), value.size());

    return stream;
}

// C-string support for formats that have fixed
// length strings
template<size_t N>
struct cstring_t
{
	std::array<char, N>	data;
	
	const char *c_str() const { return data.data(); }
	char *c_str() { return data.data(); }
};

template<size_t N>
inline std::istream &operator>=(std::istream &stream, cstring_t<N> &value)
{
    stream.read(value.c_str(), N);
    value.data[N - 1] = '\0';
	return stream;
}

template<size_t N>
inline std::ostream &operator<=(std::ostream &stream, const cstring_t<N> &value)
{
    return stream.write(value.c_str(), N);
}

// support for std::array
#include <array>

template<typename T, size_t N>
inline std::ostream &operator<=(std::ostream &s, const std::array<T, N> &c)
{
    for (auto &v : c)
        s <= v;

    return s;
}

template<typename T, size_t N>
inline std::istream &operator>=(std::istream &s, std::array<T, N> &c)
{
    for (auto &v : c)
        s >= v;

    return s;
}

// support for std::tuple
#include <tuple>

template<typename... T>
concept AreAllStreamBinaryReadCapable =
    ((IsStreamBinaryReadCapable<T>) && ...);

template<typename... T>
concept AreAllStreamBinaryWriteCapable =
    ((IsStreamBinaryWriteCapable<T>) && ...);

template<typename... T> requires AreAllStreamBinaryWriteCapable<T...>
inline std::ostream &operator<=(std::ostream &s, const std::tuple<T &...> tuple)
{
    std::apply([&s](auto &&...args) {
        ((s <= args), ...);
    }, tuple);
    return s;
}

template<typename... T> requires AreAllStreamBinaryReadCapable<T...>
inline std::istream &operator>=(std::istream &s, std::tuple<T &...> tuple)
{
    std::apply([&s](auto &&...args) {
        ((s >= args), ...);
    }, tuple);
    return s;
}

// support for T::stream_data
template<typename T>
concept IsStreamDataCapable = requires(T a)
{
    { a.stream_data() };
};

template<IsStreamDataCapable T>
inline std::ostream &operator<=(std::ostream &s, const T &object)
{
    return s <= const_cast<T &>(object).stream_data();
}

template<IsStreamDataCapable T>
inline std::istream &operator>=(std::istream &s, T &object)
{
    return s >= object.stream_data();
}

// support for T::stream_read / T::stream_write
template<typename T>
concept IsStreamWriteCapable = requires(const T &a, std::ostream &i)
{
    { a.stream_write(i) };
};

template<IsStreamWriteCapable T>
inline std::ostream &operator<=(std::ostream &s, const T &object)
{
    object.stream_write(s);
    return s;
}

template<typename T>
concept IsStreamReadCapable = requires(T &a, std::istream &i)
{
    { a.stream_read(i) };
};

template<IsStreamReadCapable T>
inline std::istream &operator>=(std::istream &s, T &object)
{
    object.stream_read(s);
    return s;
}

// support for glm::vec
#include <glm/common.hpp>

template<glm::length_t L, typename T, glm::qualifier Q>
inline std::ostream &operator<=(std::ostream &s, const glm::vec<L, T, Q> &object)
{
    for (glm::length_t i = 0; i < L; i++)
        s <= object[i];

    return s;
}

template<glm::length_t L, typename T, glm::qualifier Q>
inline std::istream &operator>=(std::istream &s, glm::vec<L, T, Q> &object)
{
    for (glm::length_t i = 0; i < L; i++)
        s >= object[i];

    return s;
}

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline std::ostream &operator<=(std::ostream &s, const glm::mat<C, R, T, Q> &object)
{
    for (glm::length_t x = 0; x < C; x++)
        for (glm::length_t y = 0; y < R; y++)
            s <= object[x][y];

    return s;
}

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline std::istream &operator>=(std::istream &s, glm::mat<C, R, T, Q> &object)
{
    for (glm::length_t x = 0; x < C; x++)
        for (glm::length_t y = 0; y < R; y++)
            s >= object[x][y];

    return s;
}

// support for glm::quat
#include <glm/ext/quaternion_common.hpp>

template<typename T, glm::qualifier Q>
inline std::ostream &operator<=(std::ostream &s, const glm::qua<T, Q> &object)
{
    for (glm::length_t i = 0; i < 4; i++)
        s <= object[i];

    return s;
}

template<typename T, glm::qualifier Q>
inline std::istream &operator>=(std::istream &s, glm::qua<T, Q> &object)
{
    for (glm::length_t i = 0; i < 4; i++)
        s >= object[i];

    return s;
}

// support for vectors
#include <vector>

template<typename T>
concept IsStreamReadableVector = requires(T a)
{
    typename T::value_type;
    requires std::is_same_v<T, std::vector<typename T::value_type>>;
    requires IsStreamBinaryReadCapable<decltype(a[0])>;
};

template<typename T>
concept IsStreamWritableVector = requires(T a)
{
    typename T::value_type;
    requires std::is_same_v<T, std::vector<typename T::value_type>>;
    requires IsStreamBinaryWriteCapable<decltype(a[0])>;
};

template<IsStreamWritableVector T>
inline std::ostream &operator<=(std::ostream &s, const T &object)
{
    s <= object.size();
    
    for (auto &v : object)
        s <= v;

    return s;
}

template<IsStreamReadableVector T>
inline std::istream &operator>=(std::istream &s, T &object)
{
    using st = typename T::size_type;
    st size;

    s >= size;

    object.resize(size);

    for (auto &v : object)
        s >= v;

    return s;
}

// support for optional
#include <optional>

template<typename T> requires IsStreamBinaryWriteCapable<T>
inline std::ostream &operator<=(std::ostream &s, const std::optional<T> &object)
{
    if (!object.has_value())
        s <= false;
    else
    {
        s <= true;
        s <= object.value();
    }

    return s;
}

template<typename T> requires IsStreamBinaryReadCapable<T>
inline std::istream &operator>=(std::istream &s, std::optional<T> &object)
{
    bool set;
    s >= set;

    if (!set)
        object = std::nullopt;
    else
    {
        object = T();
        s >= object.value();
    }

    return s;
}

// dynamic_bitset support
#include <sul/dynamic_bitset.hpp>

template<typename T>
inline std::ostream &operator<=(std::ostream &s, const sul::dynamic_bitset<T> &object)
{
    s <= object.num_blocks();
    s.write((const char *) object.data(), object.num_blocks() * sizeof(T));
    return s;
}

template<typename T>
inline std::istream &operator>=(std::istream &s, sul::dynamic_bitset<T> &object)
{
    size_t num_blocks;
    s >= num_blocks;

    object.resize(num_blocks * object.bits_per_block);
    s.read((char *) object.data(), num_blocks * sizeof(T));
    return s;
}