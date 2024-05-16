#pragma once

#include "Math.h"
#include <imgui.h>

class UVRenderer
{
public:
    void focusLost();
    glm::mat4 getDragMatrix();

    void paint();
    
    bool mousePressEvent(ImGuiMouseButton buttons, ImVec2 localPos);
    void mouseReleaseEvent(ImVec2 localPos);
    void mouseMoveEvent(ImVec2 localPos);
    void mouseWheelEvent(int delta);

    void resize(int w, int h);
    void draw();
    
    const int &width() { return _width; }
    const int &height() { return _height; }

private:
    void rectangleSelect(aabb2 rect, glm::vec2 tcScale);

    bool _dragging = false;
    glm::ivec2 _dragPos, _downPos, _dragDelta;
    glm::vec2 _dragWorldPos;
    int _width, _height;
    int _skinWidth, _skinHeight;
    int _skinX, _skinY;
};
