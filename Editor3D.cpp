#include "Editor3D.h"
#include "Widgets.h"
#include "ModelLoader.h"
#include "MDLRenderer.h"
#include "Settings.h"

#include <imgui_stdlib.h>

static const std::unordered_map<EditorTool, EventType> toolToEvents = {
    { EditorTool::CreateFace, EventType::ChangeTool_CreateFace },
    { EditorTool::CreateVertex, EventType::ChangeTool_CreateVertex },
    { EditorTool::Move, EventType::ChangeTool_Move },
    { EditorTool::Pan, EventType::ChangeTool_Pan },
    { EditorTool::Rotate, EventType::ChangeTool_Rotate },
    { EditorTool::Scale, EventType::ChangeTool_Scale },
    { EditorTool::Select, EventType::ChangeTool_Select }
};

static RenderParameters *renderParameters[] = {
    &settings().renderParams2D,
    &settings().renderParams3D
};
static constexpr const std::array<std::array<EventType, 12>, 2> renderEvents { {
    {
        EventType::Editor_3D_SetRenderModeWireframe,
        EventType::Editor_3D_SetRenderModeFlat,
        EventType::Editor_3D_SetRenderModeTextured,
        EventType::Editor_3D_SetRenderDrawBackfaces,
        EventType::Editor_3D_SetRenderPerVertexNormals,
        EventType::Editor_3D_SetRenderShading,
        EventType::Editor_3D_SetRenderShowOverlay,
        EventType::Editor_3D_SetRenderFiltering,
        EventType::Editor_3D_SetRenderShowTicks,
        EventType::Editor_3D_SetRenderShowNormals,
        EventType::Editor_3D_SetRenderShowOrigin,
        EventType::Editor_3D_SetRenderShowGrid
    },
    {
        EventType::Editor_2D_SetRenderModeWireframe,
        EventType::Editor_2D_SetRenderModeFlat,
        EventType::Editor_2D_SetRenderModeTextured,
        EventType::Editor_2D_SetRenderDrawBackfaces,
        EventType::Editor_2D_SetRenderPerVertexNormals,
        EventType::Editor_2D_SetRenderShading,
        EventType::Editor_2D_SetRenderShowOverlay,
        EventType::Editor_2D_SetRenderFiltering,
        EventType::Editor_2D_SetRenderShowTicks,
        EventType::Editor_2D_SetRenderShowNormals,
        EventType::Editor_2D_SetRenderShowOrigin,
        EventType::Editor_2D_SetRenderShowGrid
    }
} };

void Editor3D::Init()
{
    for (auto &pair : toolToEvents)
        events().Register(pair.second, [this, pair](auto) { _editorTool = pair.first; }, EventContext::Editor3D);
    
    events().Register(EventType::ToggleModifyX, [this](auto) { _editorAxis.X = !_editorAxis.X; }, EventContext::Editor3D);
    events().Register(EventType::ToggleModifyY, [this](auto) { _editorAxis.Y = !_editorAxis.Y; }, EventContext::Editor3D);
    events().Register(EventType::ToggleModifyZ, [this](auto) { _editorAxis.Z = !_editorAxis.Z; }, EventContext::Editor3D);

    for (size_t i = 0; i < 2; i++)
    {
        events().Register(renderEvents[i][0], [this, i](auto) { renderParameters[i]->mode = RenderMode::Wireframe; });
        events().Register(renderEvents[i][1], [this, i](auto) { renderParameters[i]->mode = RenderMode::Flat; });
        events().Register(renderEvents[i][2], [this, i](auto) { renderParameters[i]->mode = RenderMode::Textured; });
        events().Register(renderEvents[i][3], [this, i](auto) { renderParameters[i]->drawBackfaces = !renderParameters[i]->drawBackfaces; });
        events().Register(renderEvents[i][4], [this, i](auto) { renderParameters[i]->smoothNormals = !renderParameters[i]->smoothNormals; });
        events().Register(renderEvents[i][5], [this, i](auto) { renderParameters[i]->shaded = !renderParameters[i]->shaded; });
        events().Register(renderEvents[i][6], [this, i](auto) { renderParameters[i]->filtered = !renderParameters[i]->filtered; });
        events().Register(renderEvents[i][7], [this, i](auto) { renderParameters[i]->showOverlay = !renderParameters[i]->showOverlay; });
        events().Register(renderEvents[i][8], [this, i](auto) { renderParameters[i]->showTicks = !renderParameters[i]->showTicks; });
        events().Register(renderEvents[i][9], [this, i](auto) { renderParameters[i]->showNormals = !renderParameters[i]->showNormals; });
        events().Register(renderEvents[i][10], [this, i](auto) { renderParameters[i]->showOrigin = !renderParameters[i]->showOrigin; });
        events().Register(renderEvents[i][11], [this, i](auto) { renderParameters[i]->showGrid = !renderParameters[i]->showGrid; });
    }

    events().Register(EventType::SelectAll, [this](auto) {
        if (_editorSelectMode == SelectMode::Vertex)
            model().mutator().selectAllVertices3D();
        else
            model().mutator().selectAllTriangles3D();
    }, EventContext::Editor3D);

    events().Register(EventType::SelectInverse, [this](auto) {
        if (_editorSelectMode == SelectMode::Vertex)
            model().mutator().selectInverseVertices3D();
        else
            model().mutator().selectInverseTriangles3D();
    }, EventContext::Editor3D);

    events().Register(EventType::SelectNone, [this](auto) {
        if (_editorSelectMode == SelectMode::Vertex)
            model().mutator().selectNoneVertices3D();
        else
            model().mutator().selectNoneTriangles3D();
    }, EventContext::Editor3D);

    events().Register(EventType::SelectTouching, [this](auto) {
        if (_editorSelectMode == SelectMode::Vertex)
            model().mutator().selectTouchingVertices3D();
        else
            model().mutator().selectTouchingTriangles3D();
    }, EventContext::Editor3D);

    events().Register(EventType::SelectConnected, [this](auto) {
        if (_editorSelectMode == SelectMode::Vertex)
            model().mutator().selectConnectedVertices3D();
        else
            model().mutator().selectConnectedTriangles3D();
    }, EventContext::Editor3D);
    
    events().Register(EventType::SelectMode_Vertex, [this](auto) { _editorSelectMode = SelectMode::Vertex; renderer().markBufferDirty(); }, EventContext::Editor3D);
    events().Register(EventType::SelectMode_Face, [this](auto) { _editorSelectMode = SelectMode::Face; renderer().markBufferDirty(); }, EventContext::Editor3D);

    _renderer.initializeGL();
}

void Editor3D::Draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool render = ImGui::Begin("3D Editor", nullptr, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    ImGui::DockSpace(ImGui::GetID("3D Editor Dock"));

    if (render)
    {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_DockHierarchy | ImGuiFocusedFlags_ChildWindows))
            ui().eventContext = ui().activeEditor = EventContext::Editor3D;

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Reference"))
            {
                ImGui::MenuItem("Choose Model", "R");
                ImGui::MenuItem("Clear Model", "C");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Frames"))
            {
                ImGui::MenuItem("Add New Frame...", "A");
                ImGui::MenuItem("Delete Current Frame...", "D");
                ImGui::MenuItem("Delete Frames...", "F");
                ImGui::MenuItem("Move Frames...", "M");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Select"))
            {
                ImGui::qmdlr::MenuItemWithEvent("Select All", EventType::SelectAll, EventContext::Editor3D);
                ImGui::qmdlr::MenuItemWithEvent("Select None", EventType::SelectNone, EventContext::Editor3D);
                ImGui::qmdlr::MenuItemWithEvent("Select Inverse", EventType::SelectInverse, EventContext::Editor3D);
                ImGui::qmdlr::MenuItemWithEvent("Select Connected", EventType::SelectConnected, EventContext::Editor3D);
                ImGui::qmdlr::MenuItemWithEvent("Select Touching", EventType::SelectTouching, EventContext::Editor3D);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                constexpr const char *const viewSubMenus[] = { "2D", "3D" };

                for (size_t i = 0; i < 2; i++)
                {
                    if (ImGui::BeginMenu(viewSubMenus[i]))
                    {
                        ImGui::SeparatorText("Render Mode");
                        ImGui::qmdlr::MenuItemWithEvent("Wireframe", renderEvents[i][0], EventContext::Any, renderParameters[i]->mode == RenderMode::Wireframe);
                        ImGui::qmdlr::MenuItemWithEvent("Flat", renderEvents[i][1], EventContext::Any, renderParameters[i]->mode == RenderMode::Flat);
                        ImGui::qmdlr::MenuItemWithEvent("Textured", renderEvents[i][2], EventContext::Any, renderParameters[i]->mode == RenderMode::Textured);
                        ImGui::SeparatorText("Render Options");
                        ImGui::qmdlr::MenuItemWithEvent("Draw Backfaces", renderEvents[i][3], EventContext::Any, renderParameters[i]->drawBackfaces);
                        ImGui::qmdlr::MenuItemWithEvent("Per-Vertex Normals", renderEvents[i][4], EventContext::Any, renderParameters[i]->smoothNormals);
                        ImGui::qmdlr::MenuItemWithEvent("Shading", renderEvents[i][5], EventContext::Any, renderParameters[i]->shaded);
                        ImGui::qmdlr::MenuItemWithEvent("Filtered", renderEvents[i][6], EventContext::Any, renderParameters[i]->filtered);
                        ImGui::qmdlr::MenuItemWithEvent("Wireframe Overlay", renderEvents[i][7], EventContext::Any, renderParameters[i]->showOverlay);
                        ImGui::SeparatorText("Gadgets");
                        ImGui::qmdlr::MenuItemWithEvent("Show Vertex Ticks", renderEvents[i][8], EventContext::Any, renderParameters[i]->showTicks);
                        ImGui::qmdlr::MenuItemWithEvent("Show Normals", renderEvents[i][9], EventContext::Any, renderParameters[i]->showNormals);
                        ImGui::qmdlr::MenuItemWithEvent("Show Origin", renderEvents[i][10], EventContext::Any, renderParameters[i]->showOrigin);
                        ImGui::qmdlr::MenuItemWithEvent("Show Grid", renderEvents[i][11], EventContext::Any, renderParameters[i]->showGrid);

                        // 3d mode only stuff
                        if (i == 1)
                        {
                            ImGui::SeparatorText("3D-specific");
                            ImGui::MenuItem("Viewmodel Mode", "M", &renderer().viewWeaponMode());
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("FOV");
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(90);
                            if (ImGui::InputInt("##FOV", &renderer().fov()))
                                renderer().fov() = std::clamp(renderer().fov(), 5, 175);
                        }

                        ImGui::EndMenu();
                    }
                }
                ImGui::Separator();
                ImGui::MenuItem("Increase Grid Size", "I");
                ImGui::MenuItem("Decrease Grid Size", "O");
                ImGui::Separator();
                ImGui::MenuItem("Zoom In", "Z");
                ImGui::MenuItem("Zoom Out", "X");
                ImGui::Separator();
                ImGui::MenuItem("Hide Selected", "H");
                ImGui::MenuItem("Hide Unselected", "S");
                ImGui::MenuItem("Unhide All", "U");
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        DrawAnimationBox();
        DrawModifyBox();
        DrawFitBox();
        DrawToolBox();
        DrawVisibilityBox();
        DrawTimelineWindow();
        DrawViewport();
    }

    ImGui::End();
}

void Editor3D::DrawAnimationBox()
{
    ImGui::Begin("Animation");

    if (ImGui::qmdlr::CheckBoxButton("Play", _animation.active, ImVec2(-24, 0)))
    {
        _animation.active = !_animation.active;
        _animation.time = 0;
    }
    ImGui::SameLine();
    if (ImGui::qmdlr::CheckBoxButton("I", _animation.interpolate))
        _animation.interpolate = !_animation.interpolate;

    if (ImGui::BeginTable("Play Controls", 2))
    {
        ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        ImGui::Text("FPS");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::InputInt("##FPS", &_animation.fps);
        _animation.fps = std::clamp(_animation.fps, 1, 1000);
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("From");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::InputInt("##From", &_animation.from);
        _animation.from = std::clamp(_animation.from, 0, (int) model().model().frames.size() - 1);
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("To");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::InputInt("##To", &_animation.to);
        _animation.to = std::clamp(_animation.to, 0, (int) model().model().frames.size() - 1);
        ImGui::PopItemWidth();

        ImGui::EndTable();
    }

    ImGui::End();
}

void Editor3D::DrawModifyBox()
{
    ImGui::Begin("Modify");

    ImGui::SeparatorText("Face Tools");
    ImGui::BeginGroup();
    ImGui::Button("Flip Normals", ImVec2(-1, 0));
    ImGui::Button("Extrude", ImVec2(-1, 0));
    ImGui::EndGroup();
    ImGui::SeparatorText("Selected Tools");
    ImGui::BeginGroup();
    ImGui::Button("Mirror", ImVec2(-1, 0));
    ImGui::Button("Delete", ImVec2(-1, 0));
    ImGui::EndGroup();

    ImGui::End();
}

void Editor3D::DrawFitBox()
{
    ImGui::Begin("Fit");

    ImGui::BeginGroup();
    ImGui::Button("Fit Selected", ImVec2(-1, 0));
    ImGui::Button("Fit All", ImVec2(-1, 0));
    ImGui::EndGroup();

    ImGui::End();
}

void Editor3D::DrawToolBox()
{
    ImGui::Begin("Tools");
    
    ImGui::qmdlr::DrawToolBoxButton("Pan", EditorTool::Pan, _editorTool, toolToEvents.at(EditorTool::Pan), EventContext::Editor3D);
    ImGui::qmdlr::DrawToolBoxButton("Select", EditorTool::Select, _editorTool, toolToEvents.at(EditorTool::Select), EventContext::Editor3D);

    ImGui::Separator();
    
    ImGui::qmdlr::DrawToolBoxButton("Create Face", EditorTool::CreateFace, _editorTool, toolToEvents.at(EditorTool::CreateFace), EventContext::Editor3D);
    ImGui::qmdlr::DrawToolBoxButton("Create Vertex", EditorTool::CreateVertex, _editorTool, toolToEvents.at(EditorTool::CreateVertex), EventContext::Editor3D);

    ImGui::Separator();
    
    ImGui::qmdlr::DrawToolBoxButton("Move", EditorTool::Move, _editorTool, toolToEvents.at(EditorTool::Move), EventContext::Editor3D);
    ImGui::qmdlr::DrawToolBoxButton("Scale", EditorTool::Scale, _editorTool, toolToEvents.at(EditorTool::Scale), EventContext::Editor3D);
    ImGui::qmdlr::DrawToolBoxButton("Rotate", EditorTool::Rotate, _editorTool, toolToEvents.at(EditorTool::Rotate), EventContext::Editor3D);

    ImGui::SeparatorText("Frame Range");

    ImGui::BeginGroup();
    ImGui::Button("Affect Range", ImVec2(-1, 0));
    
    if (ImGui::BeginTable("Frame Range", 2))
    {
        ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("From");
        ImGui::TableNextColumn();
        int from = 0;
        ImGui::PushItemWidth(-1);
        ImGui::InputInt("##RangeFrom", &from);
        ImGui::PopItemWidth();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("To");
        ImGui::TableNextColumn();
        int to = 0;
        ImGui::PushItemWidth(-1);
        ImGui::InputInt("##RangeTo", &to);
        ImGui::PopItemWidth();

        ImGui::EndTable();
    }

    ImGui::EndGroup();
    
    ImGui::Separator();
    
    if (ImGui::BeginTable("Axis", 3, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::qmdlr::CheckBoxButton("X", _editorAxis.X, ImVec2(-1, 0)))
            events().Push(EventType::ToggleModifyX, EventContext::Editor3D);
        ImGui::TableNextColumn();
        if (ImGui::qmdlr::CheckBoxButton("Y", _editorAxis.Y, ImVec2(-1, 0)))
            events().Push(EventType::ToggleModifyY, EventContext::Editor3D);
        ImGui::TableNextColumn();
        if (ImGui::qmdlr::CheckBoxButton("Z", _editorAxis.Z, ImVec2(-1, 0)))
            events().Push(EventType::ToggleModifyZ, EventContext::Editor3D);

        ImGui::EndTable();
    }
    
    ImGui::Separator();
    
    if (ImGui::BeginTable("Type", 2, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::qmdlr::CheckBoxButton("Vertex", _editorSelectMode == SelectMode::Vertex, ImVec2(-1, 0)))
            events().Push(EventType::SelectMode_Vertex);
        ImGui::TableNextColumn();
        if (ImGui::qmdlr::CheckBoxButton("Face", _editorSelectMode == SelectMode::Face, ImVec2(-1, 0)))
            events().Push(EventType::SelectMode_Face);

        ImGui::EndTable();
    }
    
    ImGui::End();
}

void Editor3D::DrawVisibilityBox()
{
    ImGui::Begin("Visibility");
    
    ImGui::Button("Hide Selected", ImVec2(-1, 0));
    ImGui::Button("Hide Unselected", ImVec2(-1, 0));
    ImGui::Button("Unhide All", ImVec2(-1, 0));

    ImGui::End();
}

void Editor3D::DrawTimelineWindow()
{
    ImGui::Begin("Timeline");

    int currentFrame = model().model().selectedFrame;
    bool frameChanged = false;

    ImGui::PushItemWidth(-1);
    frameChanged = ImGui::SliderInt("##Frame", &currentFrame, 0, (int) model().model().frames.size() - 1);
    ImGui::PopItemWidth();

    ImGui::ArrowButton("<", ImGuiDir_Left);
    ImGui::SameLine();
    ImGui::PushItemWidth(80);
    frameChanged = ImGui::InputInt("##FrameNumber", &currentFrame, 0) || frameChanged;
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::ArrowButton("<", ImGuiDir_Right);

    if (frameChanged)
    {
        currentFrame = std::clamp(currentFrame, 0, (int) model().model().frames.size() - 1);
        model().mutator().setSelectedFrame(currentFrame);
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(200);
    ImGui::qmdlr::BufferedInputText("Frame Name", model().model().frames[currentFrame].name.c_str(), [](std::string &str) {
        model().mutator().setSelectedFrameName(str);
    });

    ImGui::PopItemWidth();

    ImGui::End();
}

void Editor3D::DrawViewport()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Viewport"))
    {
        ImGui::qmdlr::HandleViewport(_renderer);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
