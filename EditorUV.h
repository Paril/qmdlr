#pragma once

#include <imgui.h>
#include <SDL2/SDL_assert.h>

#include "Editor3D.h"
#include "UVRenderer.h"

// UV editor stuff
enum class LineDisplayMode
{
    None,
    Simple
};

enum class VertexDisplayMode
{
    None,
    Pixels,
    Circles
};

class EditorUV
{
public:
	void Init();
    void Draw();

    UVRenderer &renderer() { return _renderer; }
    
    EditorTool &tool() { return _uvTool; }
    ModifyAxis &axis() { return _uvAxis; }
    SelectMode &selectMode() { return _uvSelectMode; }
    LineDisplayMode &lineMode() { return _uvLineMode; }
    VertexDisplayMode &vertexMode() { return _uvVertexMode; }
    int &scale() { return _scale; }

private:
    // windows
    void DrawUVToolBox();
    void DrawUVModify();
	void DrawUVViewport();
    void DrawUVSkinSelector();

    // dialogs
    void DrawResize();
    void DrawMove();

    // uv state
    EditorTool _uvTool = EditorTool::Select;
    ModifyAxis _uvAxis = {};
    SelectMode _uvSelectMode = SelectMode::Vertex;
    LineDisplayMode _uvLineMode = LineDisplayMode::Simple;
    VertexDisplayMode _uvVertexMode = VertexDisplayMode::Circles;
    int _scale = 1;
    ImGuiMouseButton _uvMouseToViewport = 0;

    UVRenderer _renderer;

    bool _showResize = false;
    int32_t _resizeWidth = 0, _resizeHeight = 0;
    bool _resizeUVs = false, _resizeImage = false;
    bool _resizeConstrain = true;
    float _resizeWHRatio = 0, _resizeHWRatio = 0;

    bool _showMove = false;
    int _moveTarget = 0;
    int _moveDir = 0;
};