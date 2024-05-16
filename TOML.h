#pragma once

#include <toml++/toml.hpp>

namespace toml::qmdlr
{
	template<typename T>
	inline void TryLoadMember(const toml::node_view<toml::node> &node, const char *key, T &out)
	{
		if (auto val = node[key].value<T>())
			out = val.value();
	}

	template<typename T>
	inline void TrySaveMember(toml::v3::table &table, const char *key, const T &in)
	{
		table.insert(key, in);
	}

	template<>
	inline void TryLoadMember(const toml::node_view<toml::node> &node, const char *key, theme_color_array &out)
	{
		if (auto t = node[key].as_table(); t)
		{
			for (auto &kvp : *t)
			{
				size_t i = 0;

				for (auto &colorName : EditorColorNames)
				{
					if (kvp.first.str() == colorName)
						break;

					i++;
				}

				if (i == std::size(EditorColorNames))
					continue;
				else if (!kvp.second.is_array())
					continue;

				auto &arr = *kvp.second.as_array();

				if (arr.size() != 4 ||
					!arr[0].is_integer() ||
					!arr[1].is_integer() ||
					!arr[2].is_integer() ||
					!arr[3].is_integer())
					continue;

				out[i] = {
					std::clamp(*arr[0].value<uint8_t>(), (uint8_t) 0, (uint8_t) 255),
					std::clamp(*arr[1].value<uint8_t>(), (uint8_t) 0, (uint8_t) 255),
					std::clamp(*arr[2].value<uint8_t>(), (uint8_t) 0, (uint8_t) 255),
					std::clamp(*arr[3].value<uint8_t>(), (uint8_t) 0, (uint8_t) 255)
				};
			}
		}
	}

	template<>
	inline void TrySaveMember(toml::v3::table &table, const char *key, const theme_color_array &in)
	{
		if (auto &t = *(*table.emplace(key, toml::table{}).first).second.as_table(); true)
		{
			for (size_t i = 0; i < in.size(); i++)
				toml::qmdlr::TrySaveMember(t, EditorColorNames[i], toml::array {
					in[i].r,
					in[i].g,
					in[i].b,
					in[i].a
				});
		}
	}
}