#pragma once

#include <exception>
#include <filesystem>

#include "Editor3D.h"
#include "EditorUV.h"
#include "MDLRenderer.h"
#include "Settings.h"

struct LoadableTheme
{
    std::string shortName;
    std::string displayName;
};

namespace toml
{
    inline namespace v3
    {
        class table;
    }
}

// main UI class, which handles drawing the UI
// and handling state changes thereof
class UI
{
public:
	void Init();
	void Draw();

    Editor3D &editor3D() { return _editor3D; }
    EditorUV &editorUV() { return _editorUV; }

    bool syncSelection = false;

    ImGuiMouseButton &editorMouseToViewport() { return _editorMouseToViewport; }

    void LoadThemes();
    void LoadTheme(Theme theme);

    Color GetColor(EditorColorId id);
    void SetColor(EditorColorId id, Color c);
    
    EventContext eventContext = EventContext::Any;
    EventContext activeEditor = EventContext::Any;
    EventType shortcutWaiting = EventType::Last;
    KeyShortcut shortcutData {};

private:
	void DrawMainMenu();
    void DrawLog();
    
    bool _showThemeEditor = false;
    bool _showColorEditor = false;
    bool _showThemeExport = false;
    void DrawThemeWindows();

    bool _showKeyShortcuts = false;
    void DrawKeyShortcuts();

    Editor3D _editor3D;
    EditorUV _editorUV;

    ImGuiMouseButton _editorMouseToViewport = 0;

    std::vector<LoadableTheme> _themes;
    theme_color_array _themeColors = EditorColorDefaults;
    void LoadTheme(const std::string &s);
    void LoadThemeData(toml::v3::table &table, struct ImGuiStyle &style);
    void LoadTheme(const BuiltinTheme &b);
    void SaveTheme(const char *shortName, const char *displayName, const char *author, const char *url);
};

UI &ui();