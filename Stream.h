#pragma once

template<typename T>
concept is_stream_write_adapter = requires(T a, const void *data, size_t length)
{
    { a.write(data, length) };
};

template<typename T>
concept is_stream_read_adapter = requires(T a, void *data, size_t length)
{
    { a.read(data, length) };
};

struct write_adapter
{
    virtual ~write_adapter() { }
    virtual void write(const void *data, size_t length) = 0;
};

struct read_adapter
{
    virtual ~read_adapter() { }
    virtual void read(void *data, size_t length) = 0;
    virtual void skip(size_t length) = 0;
};

struct dummy_write_adapter : write_adapter
{
    virtual void write(const void *data, size_t length) override
    {
    }
};

struct dummy_read_adapter : read_adapter
{
    virtual void read(void *data, size_t length) override
    {
    }

    virtual void skip(size_t length) override
    {
    }
};

#include <iostream>

struct ostream_write_adapter : write_adapter
{
    std::ostream &stream;
    
    constexpr ostream_write_adapter(std::ostream &stream) :
        stream(stream)
    {
    }

    virtual void write(const void *data, size_t length) override
    {
        stream.write((const char *) data, length);
    }
};

struct istream_read_adapter : read_adapter
{
    std::istream &stream;

    constexpr istream_read_adapter(std::istream &stream) :
        stream(stream)
    {
    }

    virtual void read(void *data, size_t length) override
    {
        stream.read((char *) data, length);
    }

    virtual void skip(size_t length) override
    {
        stream.seekg(length, std::ios_base::cur);
    }
};

#include <vector>

struct vector_write_adapter : write_adapter
{
    std::vector<uint8_t> data;

    virtual void write(const void *in_data, size_t length) override
    {
        std::copy_n((const uint8_t *) in_data, length, std::back_inserter(data));
    }
};

struct vector_read_adapter : read_adapter
{
    const std::vector<uint8_t> &data;
    std::vector<uint8_t>::const_iterator it;

    constexpr vector_read_adapter(const std::vector<uint8_t> &data) :
        data(data),
        it(data.begin())
    {
    }
    
    virtual void read(void *out_data, size_t length) override
    {
        std::copy_n(it, length, (uint8_t *) out_data);
        it += length;
    }

    virtual void skip(size_t length) override
    {
        it += length;
    }
};

template<typename T>
struct serialize_impl : public std::false_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &a, const T)
    {
        static_assert(false, "T has no serialize writer!");
    }

    template<is_stream_read_adapter TR>
    static void read(TR &, T)
    {
        static_assert(false, "T has no serialize reader!");
    }
};

template<typename T>
concept is_serializable = serialize_impl<std::remove_reference_t<T>>::value;

template<typename... T>
concept all_are_serializable =
    ((is_serializable<T>) && ...);

template<is_stream_write_adapter TW, typename... T> requires all_are_serializable<T...>
void serialize_write(TW &writer, const T &...value)
{
    ((serialize_impl<T>::write(writer, value)), ...);
}

template<is_stream_read_adapter TR, typename... T> requires all_are_serializable<T...>
void serialize_read(TR &reader, T &...value)
{
    ((serialize_impl<T>::read(reader, value)), ...);
}

namespace detail
{
    template<typename T>
    struct serialize_primitive : public std::true_type
    {
        template<is_stream_write_adapter TW>
        static void write(TW &writer, const T &vec)
        {
            writer.write(&vec, sizeof(T));
        }
    
        template<is_stream_read_adapter TR>
        static void read(TR &reader, T &vec)
        {
            reader.read(&vec, sizeof(T));
        }
    };
}

template<> struct serialize_impl<uint8_t> : public detail::serialize_primitive<uint8_t> {};
template<> struct serialize_impl<uint16_t> : public detail::serialize_primitive<uint16_t> {};
template<> struct serialize_impl<uint32_t> : public detail::serialize_primitive<uint32_t> {};
template<> struct serialize_impl<uint64_t> : public detail::serialize_primitive<uint64_t> {};
template<> struct serialize_impl<int8_t> : public detail::serialize_primitive<int8_t> {};
template<> struct serialize_impl<int16_t> : public detail::serialize_primitive<int16_t> {};
template<> struct serialize_impl<int32_t> : public detail::serialize_primitive<int32_t> {};
template<> struct serialize_impl<int64_t> : public detail::serialize_primitive<int64_t> {};
template<> struct serialize_impl<bool> : public detail::serialize_primitive<bool> {};
template<> struct serialize_impl<float> : public detail::serialize_primitive<float> {};
template<> struct serialize_impl<double> : public detail::serialize_primitive<double> {};

template<typename T>
concept is_enum = std::is_enum_v<T>;

template<is_enum T> struct serialize_impl<T> : public detail::serialize_primitive<T> {};

// support for tuple<T&> and tuple<T> &
#include <tuple>

template<typename ...Args> requires all_are_serializable<Args...>
struct serialize_impl<std::tuple<Args &...>> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const std::tuple<Args &...> vec)
    {
        std::apply([&](auto &&...args) {
            (serialize_write(writer, args), ...);
        }, vec);
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, std::tuple<Args &...> vec)
    {
        std::apply([&](auto &&...args) {
            (serialize_read(reader, args), ...);
        }, vec);
    }
};

template<typename ...Args> requires all_are_serializable<Args...>
struct serialize_impl<std::tuple<Args...>> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const std::tuple<Args...> &vec)
    {
        std::apply([&](auto &&...args) {
            (serialize_write(writer, args), ...);
        }, vec);
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, std::tuple<Args...> &vec)
    {
        std::apply([&](auto &&...args) {
            (serialize_read(reader, args), ...);
        }, vec);
    }
};

// support for optional
#include <optional>

template<typename T> requires is_serializable<T>
struct serialize_impl<std::optional<T>> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const std::optional<T> &vec)
    {
        if (!vec.has_value())
            serialize_write(writer, false);
        else
        {
            serialize_write(writer, true);
            serialize_write(writer, vec.value());
        }
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, std::optional<T> &vec)
    {
        bool has_value;
        serialize_read(reader, has_value);

        if (has_value)
            serialize_read(reader, vec.emplace());
        else
            vec = std::nullopt;
    }
};

// support for array
#include <array>

template<typename T, size_t N> requires is_serializable<T>
struct serialize_impl<std::array<T, N>> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const std::array<T, N> &vec)
    {
        for (auto &val : vec)
            serialize_write(writer, val);
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, std::array<T, N> &vec)
    {
        for (auto &val : vec)
            serialize_read(reader, val);
    }
};

// support for glm
#include <glm/common.hpp>

template<size_t N, typename T, glm::qualifier Q> requires is_serializable<T>
struct serialize_impl<glm::vec<N, T, Q>> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const glm::vec<N, T, Q> &vec)
    {
        for (size_t i = 0; i < vec.length(); i++)
            serialize_write(writer, vec[i]);
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, glm::vec<N, T, Q> &vec)
    {
        for (size_t i = 0; i < vec.length(); i++)
            serialize_read(reader, vec[i]);
    }
};

#include <glm/ext/matrix_common.hpp>

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q> requires is_serializable<T>
struct serialize_impl<glm::mat<C, R, T, Q>> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const glm::mat<C, R, T, Q> &mat)
    {
        for (size_t i = 0; i < mat.length(); i++)
            serialize_write(writer, mat[i]);
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, glm::mat<C, R, T, Q> &mat)
    {
        for (size_t i = 0; i < mat.length(); i++)
            serialize_read(reader, mat[i]);
    }
};

#include <glm/ext/quaternion_common.hpp>

template<typename T, glm::qualifier Q> requires is_serializable<T>
struct serialize_impl<glm::qua<T, Q>> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const glm::qua<T, Q> &q)
    {
        for (glm::length_t i = 0; i < q.length(); i++)
            serialize_write(writer, q[i]);
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, glm::qua<T, Q> &q)
    {
        for (glm::length_t i = 0; i < q.length(); i++)
            serialize_read(reader, q[i]);
    }
};

// support for string
#include <string>

template<>
struct serialize_impl<std::string> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const std::string &vec)
    {
        serialize_write(writer, vec.size());
        writer.write(vec.data(), vec.size());
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, std::string &vec)
    {
        size_t s;
        serialize_read(reader, s);

        vec.resize(s);
        reader.read(vec.data(), s);
    }
};

template<>
struct serialize_impl<const char *> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const char *vec)
    {
        if (!vec)
        {
            serialize_write(writer, (size_t) 0);
            return;
        }

        size_t l = strlen(vec);
        serialize_write(writer, l);
        writer.write(vec, l);
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, const char *vec)
    {
        throw std::runtime_error("todo: remove me at compile time");
    }
};

// support for vector
#include <vector>

template<typename T> requires is_serializable<T>
struct serialize_impl<std::vector<T>> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const std::vector<T> &vec)
    {
        serialize_write(writer, vec.size());
        for (auto &val : vec)
            serialize_write(writer, val);
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, std::vector<T> &vec)
    {
        size_t s;
        serialize_read(reader, s);
        vec.resize(s);
        for (auto &val : vec)
            serialize_read(reader, val);
    }
};

// support for dynamic_bitset
#include <sul/dynamic_bitset.hpp>

template<typename T> requires is_serializable<T>
struct serialize_impl<sul::dynamic_bitset<T>> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const sul::dynamic_bitset<T> &vec)
    {
        serialize_write(writer, vec.size());
        writer.write(vec.data(), vec.num_blocks() * sizeof(T));
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, sul::dynamic_bitset<T> &vec)
    {
        size_t s;
        serialize_read(reader, s);
        vec.resize(s);
        reader.read(vec.data(), vec.num_blocks() * sizeof(T));
    }
};

// support for T::stream_data
template<typename T>
concept has_stream_data = requires(T a)
{
    { a.stream_data() }; // TODO not_void?
};

template<has_stream_data T>
struct serialize_impl<T> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const T &vec)
    {
        T &mutable_vec = const_cast<T &>(vec);
        using as_mutable = decltype(mutable_vec.stream_data());
        serialize_impl<as_mutable>::write(writer, mutable_vec.stream_data());
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, T &vec)
    {
        using data_type = decltype(vec.stream_data());
        serialize_impl<data_type>::read(reader, vec.stream_data());
    }
};

// support for T::stream_read / T::stream_write
template<typename T>
concept has_stream_write = requires(const T &a, dummy_write_adapter &adapter)
{
    { a.stream_write(adapter) };
};

template<typename T>
concept has_stream_read = requires(T &a, dummy_read_adapter &adapter)
{
    { a.stream_read(adapter) };
};

template<typename T>
concept has_stream_readwrite = has_stream_write<T> && has_stream_read<T>;

template<has_stream_readwrite T>
struct serialize_impl<T> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const T &vec)
    {
        vec.stream_write(writer);
    }

    template<is_stream_read_adapter TR>
    static void read(TR &reader, T &vec)
    {
        vec.stream_read(reader);
    }
};

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
struct serialize_impl<cstring_t<N>> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const cstring_t<N> &value)
    {
        writer.write(value.c_str(), N);
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, cstring_t<N> &value)
    {
        reader.read(value.c_str(), N);
        value.data[N - 1] = '\0';
    }
};

// fixed paddings
template<size_t n>
struct padding_t { };

template<size_t N>
struct serialize_impl<padding_t<N>> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const padding_t<N> &value)
    {
        static constexpr uint8_t padding = 0;

        for (size_t i = 0; i < N; i++)
            writer.write(&padding, 1);
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, padding_t<N> &value)
    {
        uint8_t b;

        for (size_t i = 0; i < N; i++)
            reader.read(&b, 1);
    }
};

// dynamic padding
struct padding_n { size_t n; };

template<>
struct serialize_impl<padding_n> : public std::true_type
{
    template<is_stream_write_adapter TW>
    static void write(TW &writer, const padding_n &value)
    {
        static constexpr uint8_t padding = 0;

        for (size_t i = 0; i < value.n; i++)
            writer.write(&padding, 1);
    }
    
    template<is_stream_read_adapter TR>
    static void read(TR &reader, padding_n &value)
    {
        uint8_t b;

        for (size_t i = 0; i < value.n; i++)
            reader.read(&b, 1);
    }
};
