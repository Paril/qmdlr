#include <nfd.hpp>

#include "EditorUV.h"
#include "Widgets.h"
#include "Settings.h"
#include "ModelLoader.h"
#include "Images.h"

static const std::unordered_map<EditorTool, EventType> toolToEvents = {
    { EditorTool::Move, EventType::ChangeTool_Move },
    { EditorTool::Pan, EventType::ChangeTool_Pan },
    { EditorTool::Rotate, EventType::ChangeTool_Rotate },
    { EditorTool::Scale, EventType::ChangeTool_Scale },
    { EditorTool::Select, EventType::ChangeTool_Select }
};

void EditorUV::Init()
{
    for (auto &pair : toolToEvents)
        events().Register(pair.second, [this, pair](auto) { _uvTool = pair.first; }, EventContext::EditorUV);
    
    events().Register(EventType::ToggleModifyX, [this](auto) { _uvAxis.X = !_uvAxis.X; }, EventContext::EditorUV);
    events().Register(EventType::ToggleModifyY, [this](auto) { _uvAxis.Y = !_uvAxis.Y; }, EventContext::EditorUV);
    
    events().Register(EventType::UV_VerticesNone, [this](auto) { _uvVertexMode = VertexDisplayMode::None; });
    events().Register(EventType::UV_VerticesDot, [this](auto) { _uvVertexMode = VertexDisplayMode::Pixels; });
    events().Register(EventType::UV_VerticesCircle, [this](auto) { _uvVertexMode = VertexDisplayMode::Circles; });
    events().Register(EventType::UV_LineMode, [this](auto) { _uvLineMode = (_uvLineMode == LineDisplayMode::None) ? LineDisplayMode::Simple : LineDisplayMode::None; });
    
    events().Register(EventType::ZoomIn, [this](auto) { _scale++; }, EventContext::EditorUV);
    events().Register(EventType::ZoomOut, [this](auto) { _scale = std::max(1, _scale - 1); }, EventContext::EditorUV);

    events().Register(EventType::SelectAll, [this](auto) {
        if (_uvSelectMode == SelectMode::Vertex)
            model().mutator().selectAllVerticesUV();
        else
            model().mutator().selectAllTrianglesUV();
    }, EventContext::EditorUV);

    events().Register(EventType::SelectInverse, [this](auto) {
        if (_uvSelectMode == SelectMode::Vertex)
            model().mutator().selectInverseVerticesUV();
        else
            model().mutator().selectInverseTrianglesUV();
    }, EventContext::EditorUV);

    events().Register(EventType::SelectNone, [this](auto) {
        if (_uvSelectMode == SelectMode::Vertex)
            model().mutator().selectNoneVerticesUV();
        else
            model().mutator().selectNoneTrianglesUV();
    }, EventContext::EditorUV);

    events().Register(EventType::SelectTouching, [this](auto) {
        if (_uvSelectMode == SelectMode::Vertex)
            model().mutator().selectTouchingVerticesUV();
        else
            model().mutator().selectTouchingTrianglesUV();
    }, EventContext::EditorUV);

    events().Register(EventType::SelectConnected, [this](auto) {
        if (_uvSelectMode == SelectMode::Vertex)
            model().mutator().selectConnectedVerticesUV();
        else
            model().mutator().selectConnectedTrianglesUV();
    }, EventContext::EditorUV);
    
    events().Register(EventType::SelectMode_Vertex, [this](auto) { _uvSelectMode = SelectMode::Vertex; }, EventContext::EditorUV);
    events().Register(EventType::SelectMode_Face, [this](auto) { _uvSelectMode = SelectMode::Face; }, EventContext::EditorUV);
    
    events().Register(EventType::AddSkin, [this](auto) { model().mutator().addSkin(); });
    events().Register(EventType::DeleteSkin, [this](auto) { model().mutator().deleteSkin(); });

/*

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
        nfdfilteritem_t filterItem[] = {
            { "Supported", "md2,mdl,qim" },
            { "Quake II MD2", "md2" },
            { "Quake MDL", "mdl" },
            { "QMDLR Model", "qim" },
        };
        nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, std::size(filterItem), settings().modelDialogLocation.c_str(), nullptr);
        if (result == NFD_OKAY)
        {
            settings().modelDialogLocation = outPath;
            model().Save(std::filesystem::path(outPath));
            NFD_FreePath(outPath);
        }
    });
*/
    events().Register(EventType::ImportSkin, [this](auto) {
        nfdchar_t *inPath;
        nfdresult_t result = NFD_OpenDialog(&inPath, images().SupportedFormats().data(), images().SupportedFormats().size(), settings().modelDialogLocation.c_str());
        if (result == NFD_OKAY)
        {
            Image img = images().Load(inPath);

            if (img.is_valid())
                model().mutator().importSkin(img);
            NFD_FreePath(inPath);
        }
    });
    events().Register(EventType::ExportSkin, [this](auto) {
        nfdchar_t *outPath;
        nfdresult_t result = NFD_SaveDialog(&outPath, images().SupportedFormats().data(), images().SupportedFormats().size(), settings().modelDialogLocation.c_str(), nullptr);
        if (result == NFD_OKAY)
        {
            images().Save(model().model().getSelectedSkin()->image, outPath);
            NFD_FreePath(outPath);
        }
    });
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void EditorUV::DrawResize()
{
    if (_showResize)
    {
        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(250, 100), ImGuiCond_Appearing);
        ImGui::OpenPopup("Resize Skin");
        
        auto &skin = model().model().skins[model().model().selectedSkin.value()];
        _resizeWidth = skin.width;
        _resizeHeight = skin.height;
        _resizeWHRatio = (float) skin.width / skin.height;
        _resizeHWRatio = (float) skin.height / skin.width;

        _showResize = false;
    }

    if (ImGui::BeginPopupModal("Resize Skin", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Dimensions");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(130);
        if (ImGui::InputInt("##DimensionsX", &_resizeWidth, 1, 32, ImGuiInputTextFlags_AutoSelectAll))
        {
            if (_resizeConstrain)
                _resizeHeight = _resizeWidth * _resizeHWRatio;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(130);
        if (ImGui::InputInt("##DimensionsY", &_resizeHeight, 1, 32, ImGuiInputTextFlags_AutoSelectAll))
        {
            if (_resizeConstrain)
                _resizeWidth = _resizeHeight * _resizeWHRatio;
        }
        ImGui::SameLine();
        if (ImGui::qmdlr::CheckBoxButton("C", _resizeConstrain))
        {
            _resizeConstrain = !_resizeConstrain;

            if (_resizeConstrain)
            {
                if (_resizeWidth >= _resizeHeight)
                    _resizeHeight = _resizeWidth * _resizeHWRatio;
                else
                    _resizeWidth = _resizeHeight * _resizeWHRatio;
            }
        }
        if (ImGui::BeginItemTooltip())
        {
            ImGui::Text("Constrain to aspect ratio");
            ImGui::EndTooltip();
        }

        if (_resizeWidth < 1)
            _resizeWidth = 1;
        if (_resizeHeight < 1)
            _resizeHeight = 1;
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Resize UVs");
        ImGui::SameLine();
        HelpMarker("If checked, the UVs will scale to fit the new image (such that 1.0f is still the right/bottom side of the texture). If unchecked, the UVs will retain their aspect ratio and stay in their current positions.");
        ImGui::SameLine();
        ImGui::Checkbox("##Resize UVs", &_resizeUVs);
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Resize Image");
        ImGui::SameLine();
        HelpMarker("If checked, the image will be scaled using nearest-neighbor filtering to match the wanted width/height.");
        ImGui::SameLine();
        ImGui::Checkbox("##Resize Image", &_resizeImage);
        
        if (ImGui::Button("Resize"))
        {
            model().mutator().resizeSkin(_resizeWidth, _resizeHeight, !_resizeUVs, _resizeImage);
            ui().editor3D().renderer().updateTextures();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void EditorUV::DrawMove()
{
    if (_showMove)
    {
        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_Appearing);
        ImGui::OpenPopup("Move Skin");
        
        auto &skin = model().model().skins[model().model().selectedSkin.value()];
        _moveTarget = model().model().selectedSkin.value();
        _moveDir = 0;

        _showMove = false;
    }

    if (ImGui::BeginPopupModal("Move Skin", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushItemWidth(80);
        ImGui::Combo("##Direction", &_moveDir, "Before\0After\0\0");
        ImGui::SameLine();
        
        ImGui::PushItemWidth(250);
        ImGui::Combo("##Active Skin", &_moveTarget, [](void *, int idx) -> const char *
        {
            return model().model().skins[idx].name.c_str();
        }, nullptr, model().model().skins.size());

        if (ImGui::Button("Move"))
        {
            model().mutator().moveSkin(_moveTarget, _moveDir ? true : false);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void EditorUV::Draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool render = ImGui::Begin("UV Editor", nullptr, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    ImGui::DockSpace(ImGui::GetID("UV Editor Dock"));

    if (render)
    {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_DockHierarchy | ImGuiFocusedFlags_ChildWindows))
            ui().eventContext = ui().activeEditor = EventContext::EditorUV;

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Skins"))
            {
                ImGui::qmdlr::MenuItemWithEvent("Import...", EventType::ImportSkin, EventContext::EditorUV);
                ImGui::qmdlr::MenuItemWithEvent("Export...", EventType::ExportSkin, EventContext::EditorUV);
                ImGui::Separator();
                ImGui::qmdlr::MenuItemWithEvent("Add Skin", EventType::AddSkin, EventContext::EditorUV);
                ImGui::qmdlr::MenuItemWithEvent("Delete Skin", EventType::DeleteSkin, EventContext::EditorUV);
                if (ImGui::MenuItem("Resize Skin..."))
                    _showResize = true;
                if (ImGui::MenuItem("Move Skin..."))
                    _showMove = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::MenuItem("Project...", "V");
                ImGui::Separator();
                ImGui::qmdlr::MenuItemWithEvent("Select All", EventType::SelectAll, EventContext::EditorUV);
                ImGui::qmdlr::MenuItemWithEvent("Select None", EventType::SelectNone, EventContext::EditorUV);
                ImGui::qmdlr::MenuItemWithEvent("Select Inverse", EventType::SelectInverse, EventContext::EditorUV);
                ImGui::qmdlr::MenuItemWithEvent("Select Connected", EventType::SelectConnected, EventContext::EditorUV);
                ImGui::qmdlr::MenuItemWithEvent("Select Touching", EventType::SelectTouching, EventContext::EditorUV);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Hide Selected", "H");
                ImGui::MenuItem("Hide Unselected", "S");
                ImGui::MenuItem("Unhide All", "U");
                ImGui::Separator();
                ImGui::qmdlr::MenuItemWithEvent("Zoom In", EventType::ZoomIn, EventContext::EditorUV);
                ImGui::qmdlr::MenuItemWithEvent("Zoom Out", EventType::ZoomOut, EventContext::EditorUV);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options"))
            {
                ImGui::qmdlr::MenuItemWithEvent("Lines", EventType::UV_LineMode, EventContext::Any, _uvLineMode == LineDisplayMode::Simple);
                if (ImGui::BeginMenu("Vertices"))
                {
                    ImGui::qmdlr::MenuItemWithEvent("None", EventType::UV_VerticesNone, EventContext::Any, _uvVertexMode == VertexDisplayMode::None);
                    ImGui::qmdlr::MenuItemWithEvent("Dot", EventType::UV_VerticesDot, EventContext::Any, _uvVertexMode == VertexDisplayMode::Pixels);
                    ImGui::qmdlr::MenuItemWithEvent("Circle", EventType::UV_VerticesCircle, EventContext::Any, _uvVertexMode == VertexDisplayMode::Circles);
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImVec2 topLeft = ImGui::GetCursorScreenPos();
        ImVec2 bottomRight { topLeft.x + ImGui::GetContentRegionAvail().x, topLeft.y + ImGui::GetContentRegionAvail().y };

        DrawUVToolBox();
        DrawUVSkinSelector();
        DrawUVViewport();
    }

    ImGui::End();

    DrawResize();
    DrawMove();
}

void EditorUV::DrawUVToolBox()
{
    ImGui::Begin("UV Tools");

    ImGui::qmdlr::DrawToolBoxButton("Select", EditorTool::Select, _uvTool, EventType::ChangeTool_Select, EventContext::EditorUV);

    ImGui::Separator();
    
    ImGui::qmdlr::DrawToolBoxButton("Move", EditorTool::Move, _uvTool, EventType::ChangeTool_Move, EventContext::EditorUV);
    ImGui::qmdlr::DrawToolBoxButton("Scale", EditorTool::Scale, _uvTool, EventType::ChangeTool_Scale, EventContext::EditorUV);
    ImGui::qmdlr::DrawToolBoxButton("Rotate", EditorTool::Rotate, _uvTool, EventType::ChangeTool_Rotate, EventContext::EditorUV);

    ImGui::Separator();
    
    if (ImGui::BeginTable("Axis", 2, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::qmdlr::CheckBoxButton("X", _uvAxis.X, ImVec2(-1, 0)))
            events().Push(EventType::ToggleModifyX, EventContext::EditorUV);
        ImGui::TableNextColumn();
        if (ImGui::qmdlr::CheckBoxButton("Y", _uvAxis.Y, ImVec2(-1, 0)))
            events().Push(EventType::ToggleModifyY, EventContext::EditorUV);

        ImGui::EndTable();
    }
    
    ImGui::Separator();
    
    if (ImGui::BeginTable("Type", 2, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::qmdlr::CheckBoxButton("Vertex", _uvSelectMode == SelectMode::Vertex, ImVec2(-1, 0)))
            events().Push(EventType::SelectMode_Vertex);
        ImGui::TableNextColumn();
        if (ImGui::qmdlr::CheckBoxButton("Face", _uvSelectMode == SelectMode::Face, ImVec2(-1, 0)))
            events().Push(EventType::SelectMode_Face);

        ImGui::EndTable();
    }
    
    ImGui::End();
}

void EditorUV::DrawUVViewport()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("UV Viewport", nullptr, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::qmdlr::HandleViewport(_renderer);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void EditorUV::DrawUVSkinSelector()
{
    ImGui::Begin("Skin Data");

    int item = model().model().selectedSkin.value_or(-1);
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Active Skin");
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if (ImGui::Combo("##Active Skin", &item, [](void *, int idx) -> const char *
    {
        return model().model().skins[idx].name.c_str();
    }, nullptr, model().model().skins.size()))
    {
        model().mutator().setSelectedSkin(item);
    }
    ImGui::PopItemWidth();
    
    auto skinPtr = model().model().getSelectedSkin();
    bool is_disabled = !skinPtr;

    ImGui::BeginDisabled(is_disabled);

    ImGui::SeparatorText("Generic Data");

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Skin Name");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::qmdlr::BufferedInputText("##Skin Name", is_disabled ? nullptr : skinPtr->name.c_str(), [](std::string &str) {
        model().mutator().setSelectedSkinName(str);
    });

    ImGui::EndDisabled();

    ImGui::End();
}