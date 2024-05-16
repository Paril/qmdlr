#pragma once

#include <string>
#include <filesystem>
#include <variant>
#include <SDL2/SDL.h>
#include <unordered_map>
#include <format>

#include "Math.h"
#include "EnumArray.h"
#include "Events.h"

enum class BuiltinTheme
{
    Dark,
    Light,
	Classic
};

using Theme = std::variant<BuiltinTheme, std::string>;

enum class EditorColorId
{
	VertexTickUnselected2D,
	VertexTickSelected2D,
	FaceLineUnselected2D,
	FaceLineSelected2D,
	FaceUnselected2D,
	FaceSelected2D,

	VertexTickUnselected3D,
	VertexTickSelected3D,
	FaceLineUnselected3D,
	FaceLineSelected3D,
	FaceUnselected3D,
	FaceSelected3D,
	
	VertexTickUnselectedUV,
	VertexTickSelectedUV,
	FaceLineUnselectedUV,
	FaceLineSelectedUV,
	FaceUnselectedUV,
	FaceSelectedUV,

	Grid,
	OriginX,
	OriginY,
	OriginZ,
	SelectBox,

	Total
};

static constexpr const char *EditorColorNames[] = {
	"VertexTickUnselected2D",
	"VertexTickSelected2D",
	"FaceLineUnselected2D",
	"FaceLineSelected2D",
	"FaceUnselected2D",
	"FaceSelected2D",

	"VertexTickUnselected3D",
	"VertexTickSelected3D",
	"FaceLineUnselected3D",
	"FaceLineSelected3D",
	"FaceUnselected3D",
	"FaceSelected3D",

	"VertexTickUnselectedUV",
	"VertexTickSelectedUV",
	"FaceLineUnselectedUV",
	"FaceLineSelectedUV",
	"FaceUnselectedUV",
	"FaceSelectedUV",

	"Grid",
	"OriginX",
	"OriginY",
	"OriginZ",
	"SelectBox",
};

using theme_color_array = enum_array<Color, EditorColorId>;

static constexpr theme_color_array EditorColorDefaults = {
	Color(135, 107, 87),
	Color(255, 235, 31),
	Color(255, 255, 255),
	Color(255, 235, 31),
	Color(0, 0, 0),
	Color(255, 171, 7),

	Color(235, 159, 39),
	Color(255, 235, 31),
	Color(255, 255, 255),
	Color(255, 235, 31),
	Color(0, 0, 0),
	Color(255, 171, 7),

	Color(135, 107, 87),
	Color(255, 235, 31),
	Color(123, 123, 123),
	Color(255, 235, 31),
	Color(255, 255, 255, 31),
	Color(255, 171, 7, 63),

	Color(235, 211, 199),
	Color(255, 0, 0, 127),
	Color(0, 255, 0, 127),
	Color(0, 0, 255, 127),
	Color(115, 151, 167),
};

static_assert(std::size(EditorColorNames) == ((size_t) EditorColorId::Total), "need to match color names");
static_assert(std::size(EditorColorDefaults) == std::size(EditorColorNames), "need to match color names");

enum class RenderMode : uint8_t
{
    Wireframe,
    Flat,
    Textured
};

struct RenderParameters
{
    RenderMode  mode;
    bool        drawBackfaces = false;
    bool        smoothNormals = true;
    bool        shaded        = true;
	bool		showOverlay	  = true;
    bool        showGrid      = true;
	bool        showOrigin    = true;
	bool        showTicks     = true;
	bool        showNormals   = false;
	bool		filtered      = false;
};

struct KeyShortcut
{
	SDL_Scancode	scancode;
	bool			ctrl = false, shift = false, alt = false;

	constexpr bool operator==(const KeyShortcut &v) const
	{
		return scancode == v.scancode && ctrl == v.ctrl && shift == v.shift && alt == v.alt;
	}

	constexpr bool operator!=(const KeyShortcut &v) const
	{
		return !(*this == v);
	}
};

template<>
struct std::formatter<KeyShortcut> {
    template<class ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
	
    template<class FmtContext>
    auto format(const KeyShortcut& obj, FmtContext& ctx) const {
		auto it = ctx.out();
		
		if (obj.ctrl)
			it = std::format_to(it, "Ctrl + ");
		if (obj.shift)
			it = std::format_to(it, "Shift + ");
		if (obj.alt)
			it = std::format_to(it, "Alt + ");

		if (obj.scancode == SDL_SCANCODE_UNKNOWN)
			it = std::format_to(it, "...");
		else
			it = std::format_to(it, "{}", SDL_GetScancodeName(obj.scancode));

        return it;
    }
};

struct KeyShortcutHash
{
	std::size_t operator()(const KeyShortcut &v) const
	{
		return hash_val(v.scancode, v.ctrl, v.shift, v.alt);
	}
};

class KeyShortcutMap
{
private:
	std::unordered_map<KeyShortcut, EventType, KeyShortcutHash> keyToEvent;
	std::unordered_map<EventType, KeyShortcut> eventToKey;

public:
	KeyShortcutMap(std::initializer_list<std::pair<const KeyShortcut, EventType>> keyEvents) :
		keyToEvent(keyEvents)
	{
	}

	void remove(EventType event)
	{
		if (auto v = eventToKey.find(event); v != eventToKey.end())
		{
			keyToEvent.erase(v->second);
			eventToKey.erase(v);
		}
	}

	void insert(EventType event, KeyShortcut shortcut)
	{
		keyToEvent.insert_or_assign(shortcut, event);
		eventToKey.insert_or_assign(event, shortcut);
	}

	void clear()
	{
		keyToEvent.clear();
		eventToKey.clear();
	}

	EventType find(KeyShortcut shortcut)
	{
		if (auto v = keyToEvent.find(shortcut); v != keyToEvent.end())
			return v->second;

		return EventType::Last;
	}

	KeyShortcut find(EventType event)
	{
		if (auto v = eventToKey.find(event); v != eventToKey.end())
			return v->second;

		return {};
	}

	
	auto begin() { return keyToEvent.begin(); }
	auto end() { return keyToEvent.end(); }
	auto begin() const { return keyToEvent.begin(); }
	auto end() const { return keyToEvent.end(); }
	auto cbegin() const { return keyToEvent.cbegin(); }
	auto cend() const { return keyToEvent.cend(); }
};

// static read/write access to saved settings.
// not all of these are necessarily synced to disk.
struct SettingsContainer
{
	void Load();
	void Save();
	void ResetDefaults();

	std::string modelDialogLocation = "";
	float horizontalSplit = 0.5f;
	float verticalSplit = 0.5f;
	bool overrideThemeColors = false;
	theme_color_array editorColors = EditorColorDefaults;
	RenderParameters renderParams2D = { RenderMode::Wireframe };
	RenderParameters renderParams3D = { RenderMode::Textured };
    Theme activeTheme = BuiltinTheme::Dark;
	int weaponFov = 90;
	int viewerFov = 45;
	bool openGLDebug = false;
	KeyShortcutMap shortcuts {
		{ { SDL_SCANCODE_A }, EventType::SelectAll },
		{ { SDL_SCANCODE_SLASH }, EventType::SelectNone },
		{ { SDL_SCANCODE_I }, EventType::SelectInverse },
		{ { SDL_SCANCODE_RIGHTBRACKET }, EventType::SelectConnected },
		{ { SDL_SCANCODE_LEFTBRACKET }, EventType::SelectTouching },
		{ { SDL_SCANCODE_Z, true }, EventType::Undo },
		{ { SDL_SCANCODE_Y, true }, EventType::Redo },
		{ { SDL_SCANCODE_EQUALS, true }, EventType::ZoomIn },
		{ { SDL_SCANCODE_MINUS, true }, EventType::ZoomOut }
	};
	std::string imguiData = R"INI([Window][DockSpaceViewport_11111111]
Pos=0,19
Size=900,781
Collapsed=0

[Window][3D Editor]
Pos=0,19
Size=900,781
Collapsed=0
DockId=0x8B93E3BD,0

[Window][Animation]
Pos=0,57
Size=128,187
Collapsed=0
DockId=0x00000007,0

[Window][Fit]
Pos=0,595
Size=128,126
Collapsed=0
DockId=0x0000000C,0

[Window][Visibility]
Pos=0,448
Size=128,145
Collapsed=0
DockId=0x0000000B,0

[Window][Viewport]
Pos=130,57
Size=641,664
Collapsed=0
DockId=0x00000005,0

[Window][UV Editor]
Pos=0,19
Size=900,781
Collapsed=0
DockId=0x8B93E3BD,1

[Window][Timeline]
Pos=0,723
Size=900,77
Collapsed=0
DockId=0x00000004,0

[Window][Tools]
Pos=773,57
Size=127,664
Collapsed=0
DockId=0x00000006,0

[Window][Modify]
Pos=0,246
Size=128,200
Collapsed=0
DockId=0x00000009,0

[Window][UV Tools]
Pos=773,57
Size=127,606
Collapsed=0
DockId=0x0000000E,0

[Window][UV Viewport]
Pos=0,57
Size=771,606
Collapsed=0
DockId=0x0000000D,0

[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Console]
Pos=0,19
Size=900,781
Collapsed=0
DockId=0x8B93E3BD,2

[Window][Skin Data]
Pos=0,665
Size=900,135
Collapsed=0
DockId=0x00000010,0

[Docking][Data]
DockSpace           ID=0x6A83EA60 Window=0xF8A78665 Pos=510,197 Size=900,743 Split=Y
  DockNode          ID=0x0000000F Parent=0x6A83EA60 SizeRef=900,606 Split=X
    DockNode        ID=0x0000000D Parent=0x0000000F SizeRef=527,743 CentralNode=1 HiddenTabBar=1 Selected=0x6E3D3BE4
    DockNode        ID=0x0000000E Parent=0x0000000F SizeRef=127,743 Selected=0x3216ECDF
  DockNode          ID=0x00000010 Parent=0x6A83EA60 SizeRef=900,135 Selected=0x98C789CE
DockSpace           ID=0x8B93E3BD Window=0xA787BDB4 Pos=510,159 Size=900,781 CentralNode=1 Selected=0x32A6375B
DockSpace           ID=0xA785D97D Window=0xE940481D Pos=510,197 Size=900,743 Split=Y Selected=0x13926F0B
  DockNode          ID=0x00000003 Parent=0xA785D97D SizeRef=1264,664 Split=X
    DockNode        ID=0x00000001 Parent=0x00000003 SizeRef=128,511 Split=Y Selected=0xD44407B5
      DockNode      ID=0x00000007 Parent=0x00000001 SizeRef=128,187 Selected=0xAB82399B
      DockNode      ID=0x00000008 Parent=0x00000001 SizeRef=128,475 Split=Y Selected=0xC376BBD9
        DockNode    ID=0x00000009 Parent=0x00000008 SizeRef=128,200 Selected=0xC376BBD9
        DockNode    ID=0x0000000A Parent=0x00000008 SizeRef=128,273 Split=Y Selected=0x8F1298BE
          DockNode  ID=0x0000000B Parent=0x0000000A SizeRef=128,145 Selected=0x8F1298BE
          DockNode  ID=0x0000000C Parent=0x0000000A SizeRef=128,126 Selected=0x319C5FE4
    DockNode        ID=0x00000002 Parent=0x00000003 SizeRef=1003,511 Split=X Selected=0x13926F0B
      DockNode      ID=0x00000005 Parent=0x00000002 SizeRef=641,664 CentralNode=1 HiddenTabBar=1 Selected=0x13926F0B
      DockNode      ID=0x00000006 Parent=0x00000002 SizeRef=127,664 Selected=0xD44407B5
  DockNode          ID=0x00000004 Parent=0xA785D97D SizeRef=1264,77 Selected=0x0F18B61B)INI";

private:
	std::filesystem::path _filename = "settings.toml";
};

SettingsContainer &settings();