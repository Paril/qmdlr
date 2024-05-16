#pragma once

#include <imgui.h>
#include <SDL2/SDL_assert.h>
#include "MDLRenderer.h"

enum class SelectMode
{
    Vertex,
    Face
};

enum class EditorTool
{
    // 3D + UV
    Select,
    Move,
    Rotate,
    Scale,

    // 3D editor only
    Pan,
    CreateVertex,
    CreateFace
};

struct ModifyAxis
{
    bool X = true;
    bool Y = true;
    bool Z = true; // 3d editor only (obviously)

    inline bool AxisEnabled(int axis) const
    {
        SDL_assert(axis >= 0 && axis <= 2);

        switch (axis)
        {
        case 0:
        default:
            return X;
        case 1:
            return Y;
        case 2:
            return Z;
        }
    }

    inline bool operator[](int axis) const { return AxisEnabled(axis); }
};

struct AnimationParams
{
    // parameters set via UI
    int fps = 10;
    int from = 0;
    int to = 0;
    bool active = false;
    bool interpolate = true;

    // internal state
    // TODO: eventually don't use doubles
    double time = 0;
};

class Editor3D
{
public:
	void Init();
    void Draw();
	
    EditorTool &editorTool() { return _editorTool; }
    ModifyAxis &editorAxis() { return _editorAxis; }
    SelectMode &editorSelectMode() { return _editorSelectMode; }
    MDLRenderer &renderer() { return _renderer; }
    AnimationParams &animation() { return _animation; }

private:
    // 3d editor
    void Draw3DEditor();

    // windows
	void DrawAnimationBox();
	void DrawModifyBox();
	void DrawFitBox();
	void DrawToolBox();
	void DrawVisibilityBox();
	void DrawViewport();
	void DrawTimelineWindow();
	void DrawConsoleWindow();

    // both state
    AnimationParams _animation;

    // 3d state
    EditorTool _editorTool = EditorTool::Pan;
    ModifyAxis _editorAxis = {};
    SelectMode _editorSelectMode = SelectMode::Vertex;

    // separate widgets
    MDLRenderer _renderer;
};