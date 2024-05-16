#pragma once

#include <format>

template<size_t N>
struct StackFormat
{
public:
	const char *c_str() const { return data; }

	template<typename... TArgs>
	void Format(std::format_string<TArgs...> format, TArgs&&... args)
	{
		auto result = std::format_to_n(data, N - 1, format, std::forward<TArgs&&>(args)...);
		*result.out = '\0';
	}

private:
	char data[N] = { '\0' };
};