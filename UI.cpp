#include <unordered_map>
#include <nfd.hpp>
#include <fstream>
#include <imgui.h>

#include "UI.h"
#include "Events.h"
#include "ModelLoader.h"
#include "Settings.h"
#include "Widgets.h"
#include "TOML.h"
#include "UndoRedo.h"
#include "Format.h"
#include "Log.h"

static ImGuiStyle defaultStyle;

void UI::Init()
{
    ImGui::StyleColorsDark();
    defaultStyle = ImGui::GetStyle();

    ImGui::LoadIniSettingsFromMemory(settings().imguiData.c_str(), settings().imguiData.size());

    // TODO: move to ModelLoader
    static constexpr nfdfilteritem_t filterItem[] = {
        { "Supported", "md2,mdl,qim" },
        { "Quake II MD2", "md2" },
        { "Quake MDL", "mdl" },
        { "QMDLR Model", "qim" },
    };

    events().Register(EventType::Open, [this](auto) {
        nfdchar_t *outPath;
        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, std::size(filterItem), settings().modelDialogLocation.c_str());
        if (result == NFD_OKAY)
        {
            settings().modelDialogLocation = outPath;
            model().Load(std::filesystem::path(outPath));
            NFD_FreePath(outPath);
        }
    });

    events().Register(EventType::SaveAs, [this](auto) {
        nfdchar_t *outPath;
        nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, std::size(filterItem), settings().modelDialogLocation.c_str(), nullptr);
        if (result == NFD_OKAY)
        {
            settings().modelDialogLocation = outPath;
            model().Save(std::filesystem::path(outPath));
            NFD_FreePath(outPath);
        }
    });

    events().Register(EventType::SyncSelection, [this](auto) {
        syncSelection = !syncSelection;

        if (syncSelection)
        {
            if (activeEditor == EventContext::EditorUV)
                model().mutator().syncSelectionUV();
            else
                model().mutator().syncSelection3D();
        }
    });
    
    events().Register(EventType::Undo, [this](auto) { undo().Undo(); });
    events().Register(EventType::Redo, [this](auto) { undo().Redo(); });

    _editorUV.Init();
    _editor3D.Init();
}

// Demonstrate creating a simple log window with basic filtering.
void UI::DrawLog()
{
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    // Actually call in the regular Log helper (which will Begin() into the same window as we just did)
    logger().Draw("Console");
}

#include <imgui_internal.h>

void UI::Draw()
{
    ImGui::DockSpaceOverViewport();

	DrawMainMenu();

    DrawLog();

    eventContext = EventContext::Any;

    _editor3D.renderer().updateTextures();
    
    _editorUV.Draw();
    _editor3D.Draw();

    DrawThemeWindows();
    DrawKeyShortcuts();
}

static std::filesystem::path getDebugPath(std::string_view v)
{
    std::filesystem::path p(v);

    if (std::filesystem::exists(p))
        return p;

    return "../../.." / p;
}

const std::filesystem::path themesFolder = getDebugPath("themes");

namespace toml::qmdlr
{
template<>
void TryLoadMember<ImVec2>(const toml::node_view<toml::node> &node, const char *key, ImVec2 &out)
{
	if (auto s = node[key].as_array(); s && s->size() == 2 && (*s)[0].is_number() && (*s)[1].is_number())
	{
        out = ImVec2 { *(*s)[0].value<float>(), *(*s)[1].value<float>() };
	}
}

template<>
void TrySaveMember(toml::v3::table &table, const char *key, const ImVec2 &in)
{
    TrySaveMember(table, key, toml::array { in[0], in[1] });
}

template<>
void TryLoadMember<ImVec4>(const toml::node_view<toml::node> &node, const char *key, ImVec4 &out)
{
	if (auto s = node[key].as_array(); s && s->size() == 4 && (*s)[0].is_number() && (*s)[1].is_number() && (*s)[2].is_number() && (*s)[3].is_number())
	{
        out = ImVec4 { *(*s)[0].value<float>(), *(*s)[1].value<float>(), *(*s)[2].value<float>(), *(*s)[3].value<float>() };
	}
}

template<>
void TrySaveMember(toml::v3::table &table, const char *key, const ImVec4 &in)
{
    TrySaveMember(table, key, toml::array { in.x, in.y, in.z, in.w });
}
}

#define UI_THEME_IMGUI_KEYS(a) \
    a(Alpha), \
    a(DisabledAlpha), \
    a(WindowPadding), \
    a(WindowRounding), \
    a(WindowBorderSize), \
    a(WindowMinSize), \
    a(WindowTitleAlign), \
    a(WindowMenuButtonPosition), \
    a(ChildRounding), \
    a(ChildBorderSize), \
    a(PopupRounding), \
    a(PopupBorderSize), \
    a(FramePadding), \
    a(FrameRounding), \
    a(FrameBorderSize), \
    a(ItemSpacing), \
    a(ItemInnerSpacing), \
    a(CellPadding), \
    a(TouchExtraPadding), \
    a(IndentSpacing), \
    a(ColumnsMinSpacing), \
    a(ScrollbarSize), \
    a(ScrollbarRounding), \
    a(GrabMinSize), \
    a(GrabRounding), \
    a(LogSliderDeadzone), \
    a(TabRounding), \
    a(TabBorderSize), \
    a(TabMinWidthForCloseButton), \
    a(TabBarBorderSize), \
    a(TableAngledHeadersAngle), \
    a(TableAngledHeadersTextAlign), \
    a(ColorButtonPosition), \
    a(ButtonTextAlign), \
    a(SelectableTextAlign), \
    a(SeparatorTextBorderSize), \
    a(SeparatorTextAlign), \
    a(SeparatorTextPadding), \
    a(DisplayWindowPadding), \
    a(DisplaySafeAreaPadding), \
    a(DockingSeparatorSize), \
    a(MouseCursorScale), \
    a(AntiAliasedLines), \
    a(AntiAliasedLinesUseTex), \
    a(AntiAliasedFill), \
    a(CurveTessellationTol), \
    a(CircleTessellationMaxError), \
    a(HoverStationaryDelay), \
    a(HoverDelayShort), \
    a(HoverDelayNormal), \
    a(HoverFlagsForTooltipMouse), \
    a(HoverFlagsForTooltipNav)

#define UI_THEME_IMGUI_COLORS(a) \
    a(Text), \
    a(TextDisabled), \
    a(WindowBg), \
    a(ChildBg), \
    a(PopupBg), \
    a(Border), \
    a(BorderShadow), \
    a(FrameBg), \
    a(FrameBgHovered), \
    a(FrameBgActive), \
    a(TitleBg), \
    a(TitleBgActive), \
    a(TitleBgCollapsed), \
    a(MenuBarBg), \
    a(ScrollbarBg), \
    a(ScrollbarGrab), \
    a(ScrollbarGrabHovered), \
    a(ScrollbarGrabActive), \
    a(CheckMark), \
    a(SliderGrab), \
    a(SliderGrabActive), \
    a(Button), \
    a(ButtonHovered), \
    a(ButtonActive), \
    a(Header), \
    a(HeaderHovered), \
    a(HeaderActive), \
    a(Separator), \
    a(SeparatorHovered), \
    a(SeparatorActive), \
    a(ResizeGrip), \
    a(ResizeGripHovered), \
    a(ResizeGripActive), \
    a(Tab), \
    a(TabHovered), \
    a(TabActive), \
    a(TabUnfocused), \
    a(TabUnfocusedActive), \
    a(DockingPreview), \
    a(DockingEmptyBg), \
    a(PlotLines), \
    a(PlotLinesHovered), \
    a(PlotHistogram), \
    a(PlotHistogramHovered), \
    a(TableHeaderBg), \
    a(TableBorderStrong), \
    a(TableBorderLight), \
    a(TableRowBg), \
    a(TableRowBgAlt), \
    a(TextSelectedBg), \
    a(DragDropTarget), \
    a(NavHighlight), \
    a(NavWindowingHighlight), \
    a(NavWindowingDimBg), \
    a(ModalWindowDimBg)

void UI::SaveTheme(const char *shortName, const char *displayName, const char *author, const char *url)
{
    toml::table t;
    
	if (auto &table = *(*t.emplace("Theme", toml::table{}).first).second.as_table(); true)
	{
		toml::qmdlr::TrySaveMember(table, "ShortName", shortName);
		toml::qmdlr::TrySaveMember(table, "DisplayName", displayName);
		toml::qmdlr::TrySaveMember(table, "Author", author);
		toml::qmdlr::TrySaveMember(table, "URL", url);
	}
    
    auto &style = ImGui::GetStyle();

	if (auto &table = *(*t.emplace("ImGui", toml::table{}).first).second.as_table(); true)
	{
#define THEME_SAVE_MEMBERS(key) \
    toml::qmdlr::TrySaveMember(table, #key, style.key)

        UI_THEME_IMGUI_KEYS(THEME_SAVE_MEMBERS);

#undef THEME_SAVE_MEMBERS

        // colors
    	if (auto &colors = *(*table.emplace("Colors", toml::table{}).first).second.as_table(); true)
        {
#define THEME_SAVE_COLORS(col) \
    toml::qmdlr::TrySaveMember(colors, #col, style.Colors[ImGuiCol_ ## col])

            UI_THEME_IMGUI_COLORS(THEME_SAVE_COLORS);

#undef THEME_SAVE_COLORS
        }
    }

	if (auto &table = *(*t.emplace("Editor", toml::table{}).first).second.as_table(); true)
	{
        toml::qmdlr::TrySaveMember(table, "Colors", settings().overrideThemeColors ? settings().editorColors : _themeColors);
    }

    std::filesystem::path outPath((themesFolder / shortName) += ".toml");
    std::ofstream stream(outPath, std::ios_base::out);
    stream << t;
}

void UI::LoadThemeData(toml::v3::table &table, struct ImGuiStyle &style)
{
	if (auto node = table["ImGui"])
	{
#define THEME_LOAD_MEMBERS(key) \
    toml::qmdlr::TryLoadMember(node, #key, style.key)
        
        UI_THEME_IMGUI_KEYS(THEME_LOAD_MEMBERS);

#undef THEME_LOAD_MEMBERS

        if (auto colors = decltype(node)(node["Colors"].as_table()))
        {
#define THEME_LOAD_COLORS(col) \
    toml::qmdlr::TryLoadMember(colors, #col, style.Colors[ImGuiCol_ ## col])
        
            UI_THEME_IMGUI_COLORS(THEME_LOAD_COLORS);

#undef THEME_LOAD_COLORS
        }
	}

    if (auto node = table["Editor"])
    {
        toml::qmdlr::TryLoadMember(node, "Colors", _themeColors);
    }
}

void UI::DrawThemeWindows()
{
    if (_showThemeEditor)
        ImGui::ShowStyleEditor();
    
    static char shortBuff[65] = {};
    static char nameBuff[129] = {};
    static char authorBuff[129] = {};
    static char urlBuff[129] = {};

    if (_showThemeExport)
    {
        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 2.5f));
        ImGui::OpenPopup("Save Theme");

        _showThemeExport = false;
        shortBuff[0] = '\0';
        nameBuff[0] = '\0';
        authorBuff[0] = '\0';
        urlBuff[0] = '\0';
    }

    if (ImGui::BeginPopup("Save Theme", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking))
    {
        ImGui::InputText("Short Name (no extension, e.g. 'dark')\nTry to keep this unique, use a prefix.", shortBuff, sizeof(shortBuff));
        ImGui::InputText("Display Name", nameBuff, sizeof(nameBuff));
        ImGui::InputText("Author", authorBuff, sizeof(authorBuff));
        ImGui::InputText("URL", urlBuff, sizeof(urlBuff));

        bool canExport = strlen(shortBuff) && strlen(nameBuff);

        if (!canExport)
            ImGui::BeginDisabled(true);
        if (ImGui::Button("Save!", ImVec2(-1, 0)))
        {
            SaveTheme(shortBuff, nameBuff, authorBuff, urlBuff);
            ImGui::CloseCurrentPopup();
        }
        if (!canExport)
            ImGui::EndDisabled();

        ImGui::EndPopup();
    }

    if (_showColorEditor)
    {
        ImGui::SetNextWindowSize(ImVec2(580, 240), ImGuiCond_Appearing);

        bool is_open = true;

        if (ImGui::Begin("Color Editor", &is_open, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking))
        {
            if (!settings().overrideThemeColors)
                ImGui::TextWrapped("WARNING: \"Override Theme Colors\" is not turned on, changes made here will not be saved to your personal settings and are instead only for editing the current theme for re-saving.");

            for (size_t i = 0; i < std::size(EditorColorNames); i++)
            {
                ImVec4 c { GetColor((EditorColorId) i).r / 255.f, GetColor((EditorColorId) i).g / 255.f, GetColor((EditorColorId) i).b / 255.f, GetColor((EditorColorId) i).a / 255.f };
                if (ImGui::ColorEdit4(EditorColorNames[i], &c.x, ImGuiColorEditFlags_AlphaPreviewHalf))
                    SetColor((EditorColorId) i, { (uint8_t) (c.x * 255), (uint8_t) (c.y * 255), (uint8_t) (c.z * 255), (uint8_t) (c.w * 255) });
            }
        }
        ImGui::End();

        if (!is_open)
            _showColorEditor = false;
    }
}

void UI::DrawKeyShortcuts()
{
    if (_showKeyShortcuts)
    {
        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Appearing);
        ImGui::OpenPopup("Key Shortcuts");

        _showKeyShortcuts = false;
    }

    bool is_open = true;

    if (ImGui::BeginPopupModal("Key Shortcuts", &is_open, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking))
    {
        if (ImGui::BeginTable("Shortcuts", 2))
        {
            ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Buttons", ImGuiTableColumnFlags_WidthFixed);

            StackFormat<128> key;

            for (EventType t = (EventType) 0; t < EventType::Last; t = (EventType) ((int) t + 1))
            {
                ImGui::TableNextColumn();
                ImGui::Text("%s", EventTypeNames[(size_t) t]);
                ImGui::TableNextColumn();

                KeyShortcut shortcut {};
                const char *bindString = "Unbound";

                if (ui().shortcutWaiting == t)
                {
                    shortcut = ui().shortcutData;
                    key.Format("{}", shortcut);
                    bindString = key.c_str();
                }
                else
                {
                    for (auto &sc : settings().shortcuts)
                    {
                        if (sc.second == t)
                        {
                            shortcut = sc.first;
                            key.Format("{}", shortcut);
                            bindString = key.c_str();
                            break;
                        }
                    }
                }

                ImGui::Button(bindString);

                if (ImGui::IsItemClicked())
                {
                    ui().shortcutWaiting = t;
                    ui().shortcutData = {};
                }

                ImGui::TableNextRow();
            }

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }

    if (!is_open)
    {
        ui().shortcutWaiting = EventType::Last;
    }
    else
    {
        ui().eventContext = EventContext::Skip;
    }
}

void UI::LoadThemes()
{
    _themes.clear();

    for (auto const& dir_entry : std::filesystem::directory_iterator{themesFolder}) 
    {
        if (!dir_entry.is_regular_file())
            continue;

        toml::table t;
	    try
	    {
		    std::ifstream stream(dir_entry.path(), std::ios_base::in);

            if (!stream)
                continue;

            auto table = toml::parse(stream);
            auto themeTable = table.get_as<toml::table>("Theme");

            if (!themeTable)
                continue;
            
            auto shortName = themeTable->get_as<std::string>("ShortName");
            auto displayName = themeTable->get_as<std::string>("DisplayName");

            if (!shortName || !displayName)
                continue;

            _themes.emplace_back(*(*shortName), *(*displayName));
        }
        catch(const toml::parse_error& err)
        {
            continue;
        }
    }
}

Color UI::GetColor(EditorColorId id)
{
    if (settings().overrideThemeColors)
        return settings().editorColors[id];
    return _themeColors[id];
}

void UI::SetColor(EditorColorId id, Color c)
{
    if (settings().overrideThemeColors)
        settings().editorColors[id] = c;
    else
        _themeColors[id] = c;

    editor3D().renderer().colorsChanged();
}

void UI::LoadTheme(Theme theme)
{
    auto &style = ImGui::GetStyle();
    style = defaultStyle;
    _themeColors = EditorColorDefaults;

    if (std::holds_alternative<BuiltinTheme>(theme))
        LoadTheme(std::get<BuiltinTheme>(theme));
    else
        LoadTheme(std::get<std::string>(theme));

    settings().activeTheme = theme;

    if (!settings().overrideThemeColors)
        settings().editorColors = _themeColors;

    editor3D().renderer().colorsChanged();
}

void UI::LoadTheme(const std::string &s)
{
    std::filesystem::path themePath = (themesFolder / s) += ".toml";

    if (!std::filesystem::exists(themePath))
    {
        LoadThemes();

        if (!std::filesystem::exists(themePath))
        {
            LoadTheme(BuiltinTheme::Dark);
            return;
        }
    }

    toml::table t;
	try
	{
		std::ifstream stream(themePath, std::ios_base::in);

        if (!stream)
            return;

        auto table = toml::parse(stream);

        LoadThemeData(table, ImGui::GetStyle());
    }
    catch(const toml::parse_error& err)
    {
        return;
    }
}

void UI::LoadTheme(const BuiltinTheme &b)
{
    if (b == BuiltinTheme::Dark)
        ImGui::StyleColorsDark();
    else if (b == BuiltinTheme::Light)
        ImGui::StyleColorsLight();
    else
        ImGui::StyleColorsClassic();
}

template<typename V, typename T>
bool VariantContains(const V &var, const T &val)
{
    if (std::holds_alternative<T>(var))
        return std::get<T>(var) == val;
    return false;
}

void UI::DrawMainMenu()
{
    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("File"))
    {
        ImGui::MenuItem("New", "N");
        ImGui::qmdlr::MenuItemWithEvent("Open...", EventType::Open);
        ImGui::MenuItem("Save", "S");
        ImGui::qmdlr::MenuItemWithEvent("Save As...", EventType::SaveAs);
        ImGui::MenuItem("Merge", "M");
        ImGui::Separator();
        ImGui::MenuItem("Import...", "I");
        ImGui::MenuItem("Export...", "E");
        ImGui::Separator();
        ImGui::MenuItem("Exit", "X");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        // run deferred here to keep menu up to date
        undo().RunDeferred(true);

        if (ImGui::MenuItem("Undo", "Z", nullptr, undo().CanUndo()))
            undo().Undo();
        if (ImGui::MenuItem("Redo", "Y", nullptr, undo().CanRedo()))
            undo().Redo();
        ImGui::Separator();
        if (ImGui::BeginMenu("History"))
        {
            // the history displays a list of undo/redo states;
            // to match other implementations (Blender), the list
            // is of redo states, *not* undo states, so it's slightly
            // different than how `undo` is laid out.
            // `undo`'s pointer points to the last entry we had undone,
            // meaning the current state is technically + 1 from the pointers' location.
            std::optional<UndoRedoStateIterator> activeRedoPointer;

            if (undo().Pointer().has_value())
            {
                activeRedoPointer = undo().Pointer().value();

                if (activeRedoPointer == undo().List().begin())
                    activeRedoPointer = std::nullopt;
                else
                    (*activeRedoPointer)--;
            }
            else if (undo().List().size())
            {
                activeRedoPointer = undo().List().end();
                (*activeRedoPointer)--;
            }

            std::optional<decltype(undo().List().rbegin())> switchUndo;
            bool behind = false, clickedBehind = false;

            for (auto it = undo().List().rbegin(); ; ++it)
            {
                const char *name;

                if (it == undo().List().rend())
                    name = "Original";
                else
                    name = (*it)->Name();

                if ((activeRedoPointer.has_value() && it == --decltype(it)(*activeRedoPointer)) ||
                    (!activeRedoPointer.has_value() && it == undo().List().rend()))
                {
                    ImGui::BeginDisabled(true);
                    ImGui::BulletText(name);
                    ImGui::EndDisabled();
                    behind = true;
                }
                else
                {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetTreeNodeToLabelSpacing());

                    if (ImGui::MenuItem(name))
                    {
                        clickedBehind = behind;
                        switchUndo = it;
                    }
                }

                if (it == undo().List().rend())
                    break;

                if (auto combined = dynamic_cast<UndoRedoCombinedState *>(it->get()))
                {
                    if (ImGui::BeginItemTooltip())
                    {
                        for (auto &state : combined->getStates())
                            ImGui::BulletText(state->Name());

                        ImGui::EndTooltip();
                    }
                }
            }

            if (switchUndo)
            {
                if (switchUndo == undo().List().rend())
                    undo().SetPointer(undo().List().begin(), clickedBehind);
                else
                    undo().SetPointer(++UndoRedoStateIterator((*switchUndo)->get()->Iterator()), clickedBehind);
            }

            ImGui::EndMenu();
        }
        ImGui::BeginDisabled(true);

        // TODO split into function
        StackFormat<15> size;
        size_t undoSize = undo().Size();

        struct {
            size_t      ratio;
            const char  *label;
        } labels[] = {
            { 1000000000, "gb" },
            { 1000000, "mb" },
            { 1000, "kb" },
            { 0, "b" }
        };

        for (auto &ratio : labels)
        {
            if (undoSize >= ratio.ratio)
            {
                if (ratio.ratio)
                    size.Format("{:02} {}", (double) undoSize / ratio.ratio, ratio.label);
                else
                    size.Format("{} {}", undoSize, ratio.label);

                break;
            }
        }

        ImGui::Text("Undo Memory Used: %s", size.c_str());
        ImGui::EndDisabled();
        ImGui::Separator();
        ImGui::MenuItem("Copy", "C");
        ImGui::MenuItem("Paste", "V");
        ImGui::MenuItem("Paste to Range", "B");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Options"))
    {
        ImGui::qmdlr::MenuItemWithEvent("Sync UV/Face Selection", EventType::SyncSelection, EventContext::Any, syncSelection);
        if (ImGui::MenuItem("Key Shortcuts", "C"))
            _showKeyShortcuts = true;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Tools"))
    {
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help"))
    {
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Theme"))
    {
        if (ImGui::BeginMenu("Change"))
        {
            std::optional<Theme> changeThemes = std::nullopt;

            if (ImGui::MenuItem("Dark (builtin)", "D", VariantContains(settings().activeTheme, BuiltinTheme::Dark)))
                changeThemes = BuiltinTheme::Dark;
            if (ImGui::MenuItem("Light (builtin)", "L", VariantContains(settings().activeTheme, BuiltinTheme::Light)))
                changeThemes = BuiltinTheme::Light;
            if (ImGui::MenuItem("Classic (builtin)", "C", VariantContains(settings().activeTheme, BuiltinTheme::Classic)))
                changeThemes = BuiltinTheme::Classic;

            if (_themes.size())
            {
                ImGui::Separator();

                for (auto &theme : _themes)
                    if (ImGui::MenuItem(theme.displayName.c_str(), nullptr, VariantContains(settings().activeTheme, theme.shortName)))
                        changeThemes = theme.shortName;
            }

            if (changeThemes)
                LoadTheme(changeThemes.value());

            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Reload Themes"))
            LoadThemes();
        if (ImGui::MenuItem("Toggle ImGui Editor"))
            _showThemeEditor = !_showThemeEditor;
        if (ImGui::MenuItem("Toggle Color Editor"))
            _showColorEditor = !_showColorEditor;
        if (ImGui::MenuItem("Save..."))
            _showThemeExport = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Override Theme Colors", nullptr, settings().overrideThemeColors)) // TODO
            settings().overrideThemeColors = !settings().overrideThemeColors;

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Debug"))
    {
        ImGui::MenuItem("Capture RenderDoc Frame", "F");
        if (GLAD_GL_KHR_debug)
        {
            if (ImGui::MenuItem("Enable OpenGL Debug", "G", settings().openGLDebug))
            {
                settings().openGLDebug = !settings().openGLDebug;

                if (settings().openGLDebug)
                    glEnable(GL_DEBUG_OUTPUT);
                else
                    glDisable(GL_DEBUG_OUTPUT);
            }
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

UI &ui()
{
	static UI _ui;
	return _ui;
}
