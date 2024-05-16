#include <fstream>
#include <imgui.h>
#include <ranges>
#include "settings.h"
#include "TOML.h"

namespace toml::qmdlr
{
template<>
void TryLoadMember<RenderMode>(const toml::node_view<toml::node> &node, const char *key, RenderMode &out)
{
	if (auto s = node.as_string(); s)
	{
		if (s->get() == "Wireframe")
			out = RenderMode::Wireframe;
		else if (s->get() == "Wireframe")
			out = RenderMode::Wireframe;
		else if (s->get() == "Textured")
			out = RenderMode::Textured;
	}
}

template<>
void TrySaveMember(toml::v3::table &table, const char *key, const RenderMode &in)
{
	switch (in)
	{
	case RenderMode::Wireframe:
		TrySaveMember(table, key, "Wireframe");
		break;
	case RenderMode::Flat:
		TrySaveMember(table, key, "Flat");
		break;
	case RenderMode::Textured:
		TrySaveMember(table, key, "Textured");
		break;
	}
}

template<>
void TryLoadMember(const toml::node_view<toml::node> &node, const char *key, RenderParameters &out)
{
	if (auto t = node[key].as_table(); t)
	{
		auto tn = toml::node_view<toml::node>(t);
		TryLoadMember(tn, "Mode", out.mode);
		TryLoadMember(tn, "DrawBackfaces", out.drawBackfaces);
		TryLoadMember(tn, "SmoothNormals", out.smoothNormals);
		TryLoadMember(tn, "Shaded", out.shaded);
		TryLoadMember(tn, "ShowOverlay", out.showOverlay);
		TryLoadMember(tn, "Filtered", out.filtered);
		TryLoadMember(tn, "ShowGrid", out.showGrid);
		TryLoadMember(tn, "ShowOrigin", out.showOrigin);
		TryLoadMember(tn, "ShowTicks", out.showTicks);
		TryLoadMember(tn, "ShowNormals", out.showNormals);
	}
}

template<>
void TrySaveMember(toml::v3::table &table, const char *key, const RenderParameters &in)
{
	if (auto &sub = *(*table.emplace(key, toml::table{}).first).second.as_table(); true)
	{
		TrySaveMember(sub, "Mode", in.mode);
		TrySaveMember(sub, "DrawBackfaces", in.drawBackfaces);
		TrySaveMember(sub, "SmoothNormals", in.smoothNormals);
		TrySaveMember(sub, "Shaded", in.shaded);
		TrySaveMember(sub, "ShowOverlay", in.showOverlay);
		TrySaveMember(sub, "Filtered", in.filtered);
		TrySaveMember(sub, "ShowGrid", in.showGrid);
		TrySaveMember(sub, "ShowOrigin", in.showOrigin);
		TrySaveMember(sub, "ShowTicks", in.showTicks);
		TrySaveMember(sub, "ShowNormals", in.showNormals);
	}
}

template<>
inline void TryLoadMember(const toml::node_view<toml::node> &node, const char *key, KeyShortcutMap &out)
{
	if (auto t = node[key].as_table(); t)
	{
		out.clear();

		for (auto &kvp : *t)
		{
			size_t i = 0;

			for (auto &eventName : EventTypeNames)
			{
				if (kvp.first.str() == eventName)
					break;

				i++;
			}

			if (i == std::size(EventTypeNames))
				continue;
			else if (!kvp.second.is_string())
				continue;

			auto &str = *kvp.second.as_string();
			KeyShortcut shortcut {};
			
			for (const auto word : std::views::split(str.get(), "+"sv))
			{
				if (std::string_view(word.begin(), word.end()) == "Ctrl")
					shortcut.ctrl = true;
				else if (std::string_view(word.begin(), word.end()) == "Shift")
					shortcut.shift = true;
				else if (std::string_view(word.begin(), word.end()) == "Alt")
					shortcut.alt = true;
				else
					shortcut.scancode = SDL_GetScancodeFromName(std::string(word.begin(), word.end()).c_str());
			}

			out.insert((EventType) i, shortcut);
		}
	}
}

template<>
inline void TrySaveMember(toml::v3::table &table, const char *key, const KeyShortcut &in)
{
	std::string s;
	
	if (in.ctrl)
		s += "Ctrl+";
	if (in.shift)
		s += "Shift+";
	if (in.alt)
		s += "Alt+";
	s += SDL_GetScancodeName(in.scancode);

	toml::qmdlr::TrySaveMember(table, key, s);
}

template<>
inline void TrySaveMember(toml::v3::table &table, const char *key, const KeyShortcutMap &in)
{
	if (auto &t = *(*table.emplace(key, toml::table{}).first).second.as_table(); true)
	{
		for (auto kvp : in)
		{
			toml::qmdlr::TrySaveMember(t, EventTypeNames[(size_t) kvp.second], kvp.first);
		}
	}
}
}

void SettingsContainer::Load()
{
	try
	{
		if (auto stream = std::ifstream(_filename))
		{
			auto table = toml::parse(stream);
			
			if (auto node = table["UI"])
			{
				toml::qmdlr::TryLoadMember(node, "ImGUIData", imguiData);
				toml::qmdlr::TryLoadMember(node, "OverrideThemeColors", overrideThemeColors);
			}
			
			if (auto node = table["Dialogs"])
			{
				toml::qmdlr::TryLoadMember(node, "LastModelLocation", modelDialogLocation);
			}

			if (auto node = table["3DEditor"])
			{
				toml::qmdlr::TryLoadMember(node, "HorizontalSplit", horizontalSplit);
				toml::qmdlr::TryLoadMember(node, "VerticalSplit", verticalSplit);
				toml::qmdlr::TryLoadMember(node, "RenderParameters", renderParams3D);
				toml::qmdlr::TryLoadMember(node, "Fov", viewerFov);
				toml::qmdlr::TryLoadMember(node, "WeaponFov", weaponFov);
			}

			if (auto node = table["UVEditor"])
			{
				toml::qmdlr::TryLoadMember(node, "RenderParameters", renderParams2D);
			}

			if (auto node = table["Debug"])
			{
				toml::qmdlr::TryLoadMember(node, "OpenGLDebug", openGLDebug);
			}
			
			toml::qmdlr::TryLoadMember(toml::node_view<toml::node>(table), "Colors", editorColors);

			toml::qmdlr::TryLoadMember(toml::node_view<toml::node>(table), "Shortcuts", shortcuts);
		}
		else
			Save(); // save defaults if we have no config file yet
	}
    catch (const toml::parse_error& err)
    {
	}
}

void SettingsContainer::Save()
{
	toml::table settings;

	if (ImGui::GetCurrentContext())
	{
		auto &io = ImGui::GetIO();

		if (io.WantSaveIniSettings)
		{
			imguiData = ImGui::SaveIniSettingsToMemory();
			while (imguiData.size() && (imguiData.back() == '\n' || imguiData.back() == '\r'))
				imguiData.pop_back();
			io.WantSaveIniSettings = false;
		}
	}

	if (auto &table = *(*settings.emplace("UI", toml::table{}).first).second.as_table(); true)
	{
		toml::qmdlr::TrySaveMember(table, "ImGUIData", imguiData);
		toml::qmdlr::TrySaveMember(table, "OverrideThemeColors", overrideThemeColors);
	}
	
	if (auto &table = *(*settings.emplace("Dialogs", toml::table{}).first).second.as_table(); true)
	{
		toml::qmdlr::TrySaveMember(table, "LastModelLocation", modelDialogLocation);
	}
	
	if (auto &table = *(*settings.emplace("3DEditor", toml::table{}).first).second.as_table(); true)
	{
		toml::qmdlr::TrySaveMember(table, "HorizontalSplit", horizontalSplit);
		toml::qmdlr::TrySaveMember(table, "VerticalSplit", verticalSplit);
		toml::qmdlr::TrySaveMember(table, "RenderParameters", renderParams3D);
		toml::qmdlr::TrySaveMember(table, "Fov", viewerFov);
		toml::qmdlr::TrySaveMember(table, "WeaponFov", weaponFov);
	}
	
	if (auto &table = *(*settings.emplace("UVEditor", toml::table{}).first).second.as_table(); true)
	{
		toml::qmdlr::TrySaveMember(table, "RenderParameters", renderParams2D);
	}

	if (auto &table = *(*settings.emplace("Debug", toml::table{}).first).second.as_table(); true)
	{
		toml::qmdlr::TrySaveMember(table, "OpenGLDebug", openGLDebug);
	}
	
	toml::qmdlr::TrySaveMember(settings, "Colors", editorColors);

	toml::qmdlr::TrySaveMember(settings, "Shortcuts", shortcuts);

	std::ofstream stream(_filename);
	stream << settings;
}

void SettingsContainer::ResetDefaults()
{
	*this = {};
}

SettingsContainer &settings()
{
	static SettingsContainer settings;
	return settings;
}