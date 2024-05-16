#pragma once

#include <array>
#include <type_traits>

template<typename TV, typename TEnum, size_t Num = (size_t) TEnum::Total>
struct enum_array
{
	using array = std::array<TV, Num>;

	array values;
	
	constexpr TV &operator[](size_t e) { return values[e]; }
	constexpr const TV &operator[](size_t e) const { return values[e]; }
	
	constexpr TV &operator[](TEnum e) { return values[(size_t) e]; }
	constexpr const TV &operator[](TEnum e) const { return values[(size_t) e]; }

	constexpr size_t size() const { return values.size(); }
};