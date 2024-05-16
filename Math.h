#pragma once

// Maffs
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <array>

template<glm::length_t L, typename T, glm::qualifier Q = glm::defaultp>
struct aabb
{
    using vec_type = glm::vec<L, T, Q>;

    glm::vec<L, T, Q> mins, maxs;

    constexpr aabb(T size, const vec_type &origin = {}) noexcept
    {
        T half = size * 0.5f;
        mins = origin + vec_type { -half };
        maxs = origin + vec_type { half };
    }

    constexpr aabb() noexcept :
        mins(std::numeric_limits<T>::infinity()),
        maxs(-std::numeric_limits<T>::infinity())
    {
    }

    static constexpr aabb fromMinsMaxs(const vec_type &mins, const vec_type &maxs)
    {
        aabb bounds;
        bounds.mins = mins;
        bounds.maxs = maxs;
        return bounds;
    }

    constexpr void add(const vec_type &pt) noexcept
    {
        for (glm::length_t i = 0; i < L; i++)
        {
            mins[i] = std::min(mins[i], pt[i]);
            maxs[i] = std::max(maxs[i], pt[i]);
        }
    }

    constexpr vec_type centroid() const noexcept
    {
        return (maxs + mins) * 0.5f;
    }

    inline bool empty() const noexcept
    {
        return std::isinf(mins[0]);
    }

    inline bool contains(const vec_type &pt) const noexcept
    {
        return glm::all(glm::greaterThanEqual(pt, mins)) && glm::all(glm::lessThan(pt, maxs));
    }

    inline aabb normalize() const noexcept
    {
        aabb bounds(*this);

        for (size_t i = 0; i < bounds.mins.length(); i++)
            if (bounds.maxs[i] < bounds.mins[i])
                std::swap(bounds.maxs[i], bounds.mins[i]);

        return bounds;
    }
};

using aabb2 = aabb<2, float>;
using aabb3 = aabb<3, float>;

struct Color
{
    union {
        struct {
            uint8_t r, g, b, a;
        };
        std::array<uint8_t, 4> rgba;
        uint32_t u32;
    };

    Color() = default;
    Color(const Color &) = default;
    Color(Color &&) = default;
    Color &operator=(const Color &) = default;
    Color &operator=(Color &&) = default;

    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) :
        r(r),
        g(g),
        b(b),
        a(a)
    {
    }

    constexpr Color(uint8_t r, uint8_t g, uint8_t b) :
        Color(r, g, b, 255)
    {
    }
    
    constexpr const uint8_t &operator[](int index) const { return rgba[index]; }
    constexpr uint8_t &operator[](int index) { return rgba[index]; }

    constexpr glm::vec4 as_vec4() const
    {
        return { r / 255.f, g / 255.f, b / 255.f, a / 255.f };
    }
};

template<typename T>
constexpr T wrap(T v, T min, T max)
{
    T range = max - min + 1;

    if (v < min)
	    v += range * ((min - v) / range + 1);

    return min + (v - min) % range;
}

template <typename T>
inline void hash_combine (std::size_t& seed, const T& val)
{
    seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

// auxiliary generic functions to create a hash value using a seed
template <typename T, typename... Types>
inline void hash_combine (std::size_t& seed, const T& val, const Types&... args)
{
    hash_combine(seed,val);
    hash_combine(seed,args...);
}

// optional auxiliary generic functions to support hash_val() without arguments
inline void hash_combine (std::size_t& seed)
{
}

//  generic function to create a hash value out of a heterogeneous list of arguments
template <typename... Types>
inline std::size_t hash_val (const Types&... args)
{
    std::size_t seed = 0;
    hash_combine (seed, args...);
    return seed;
}