#include "ModelLoader.h"
#include "UVRenderer.h"
#include "Settings.h"
#include "UI.h"
#include "UndoRedo.h"

// return a fixed array of texcoord positions
// that match the given transformation. it only modifies
// coordinates that are actually changed by the matrix.
static const std::vector<glm::vec2> &transformTexcoords(const ModelMesh &mesh, int32_t width, int32_t height, const glm::mat4 &matrix, SelectMode mode)
{
    static std::vector<glm::vec2> coordinates;
    coordinates.resize(mesh.texcoords.size());

    const auto &verticesSelected = model().mutator().getSelectedTextureCoordinates(mesh, mode);

    glm::vec2 scale { (float) width, (float) height };

    for (size_t i = 0; i < mesh.texcoords.size(); i++)
    {
        coordinates[i] = mesh.texcoords[i].pos;

        if (!glm::all(glm::equal(matrix, glm::identity<glm::mat4>())) && verticesSelected.contains(i))
            coordinates[i] = glm::vec2(matrix * glm::vec4(coordinates[i] * scale, 0, 1)) / scale;
    }

    return coordinates;
}

void UVRenderer::resize(int available_width, int available_height)
{
    auto &mdl = model().model();
    auto skin = mdl.getSelectedSkin();

    if (!skin)
    {
        _skinWidth = _skinHeight = 0;
        _width = available_width;
        _height = available_height;
        return;
    }
    
    _skinWidth = skin->width * ui().editorUV().scale();
    _skinHeight = skin->height * ui().editorUV().scale();
    _width = std::max(_skinWidth, available_width);
    _height = std::max(_skinHeight, available_height);
    
    if (_skinWidth >= _width)
        _skinX = 0;
    else
        _skinX = (_width / 2) - (_skinWidth / 2);

    if (_skinHeight >= _height)
        _skinY = 0;
    else
        _skinY = (_height / 2) - (_skinHeight / 2);
}

void UVRenderer::draw()
{
    auto &mdl = model().model();
    auto skin = mdl.getSelectedSkin();

    if (!skin)
        return;

    auto drawer = ImGui::GetWindowDrawList();
    auto cursor = ImGui::GetCursorScreenPos();
    cursor.x += _skinX;
    cursor.y += _skinY;
    ImGui::SetCursorScreenPos(cursor);
    drawer->AddRect({ cursor.x - 1, cursor.y - 1 }, { cursor.x + _skinWidth + 1, cursor.y + _skinHeight + 1 }, Color(192, 192, 192, 64).u32);
    ImGui::Image(skin->handle->GetTextureHandle(), { (float) _skinWidth, (float) _skinHeight });
}

void UVRenderer::paint()
{
    auto &mdl = model().model();
    auto skin = mdl.getSelectedSkin();

    if (!skin || !skin->handle)
        return;

    ImVec2 topLeft = ImGui::GetCursorScreenPos();

    auto drawer = ImGui::GetWindowDrawList();
    ImDrawListFlags oldFlags = drawer->Flags;
    drawer->Flags = ImDrawListFlags_None;
    glm::vec2 drawOffset { _skinX + topLeft.x, _skinY + topLeft.y };
    glm::mat4 drag = getDragMatrix();
    glm::vec2 tcScale { (float) _skinWidth, (float) _skinHeight };

    for (auto &mesh : mdl.meshes)
    {
        if (mesh.assigned_skin.has_value() && mesh.assigned_skin.value() != mdl.selectedSkin.value())
            continue;

        const auto &verticesSelected = model().mutator().getSelectedTextureCoordinates(mesh, ui().editorUV().selectMode());
        const auto &coordinates = transformTexcoords(mesh, skin->width, skin->height, drag, ui().editorUV().selectMode());

        if (ui().editorUV().lineMode() == LineDisplayMode::Simple)
        {
            for (auto &v : mesh.triangles)
            {
                if (ui().editorUV().selectMode() == SelectMode::Face && v.selectedUV)
                    continue;

                glm::vec2 points[] = {
                    drawOffset + (coordinates[v.texcoords[0]] * tcScale),
                    drawOffset + (coordinates[v.texcoords[1]] * tcScale),
                    drawOffset + (coordinates[v.texcoords[2]] * tcScale),
                    drawOffset + (coordinates[v.texcoords[0]] * tcScale)
                };

                drawer->AddConvexPolyFilled((ImVec2 *) points, std::size(points), ui().GetColor(EditorColorId::FaceUnselectedUV).u32);
                drawer->AddPolyline((ImVec2 *) points, std::size(points), ui().GetColor(EditorColorId::FaceLineUnselectedUV).u32, ImDrawListFlags_None, 1.0f);
            }

            if (ui().editorUV().selectMode() == SelectMode::Face)
            {
                for (auto &v : mesh.triangles)
                {
                    if (!v.selectedUV)
                        continue;

                    glm::vec2 points[] = {
                        drawOffset + (coordinates[v.texcoords[0]] * tcScale),
                        drawOffset + (coordinates[v.texcoords[1]] * tcScale),
                        drawOffset + (coordinates[v.texcoords[2]] * tcScale),
                        drawOffset + (coordinates[v.texcoords[0]] * tcScale)
                    };

                    drawer->AddConvexPolyFilled((ImVec2 *) points, std::size(points), ui().GetColor(EditorColorId::FaceSelectedUV).u32);
                    drawer->AddPolyline((ImVec2 *) points, std::size(points), ui().GetColor(EditorColorId::FaceLineSelectedUV).u32, ImDrawListFlags_None, 1.0f);
                }
            }
        }

        if (ui().editorUV().vertexMode() != VertexDisplayMode::None)
        {
            for (size_t i = 0; i < mesh.texcoords.size(); i++)
            {
                glm::vec2 pos = coordinates[i] * tcScale;

                const Color &color = ui().GetColor(verticesSelected.contains(i) ?
                    EditorColorId::VertexTickSelectedUV : EditorColorId::VertexTickUnselectedUV);

                if (ui().editorUV().vertexMode() == VertexDisplayMode::Circles)
                    drawer->AddCircleFilled({ drawOffset.x + pos.x, drawOffset.y + pos.y }, 2.0f, color.u32);
                else
                    drawer->AddRectFilled({ drawOffset.x + pos.x, drawOffset.y + pos.y }, { drawOffset.x + pos.x + 1, drawOffset.y + pos.y + 1 }, color.u32);
            }
        }
    }

    if (_dragging && ui().editorUV().tool() == EditorTool::Select)
    {
        aabb2 bounds = aabb2::fromMinsMaxs(_dragWorldPos, glm::vec2(_dragPos)).normalize();
        drawer->AddRect({ drawOffset.x + bounds.mins.x - _skinX, drawOffset.y + bounds.mins.y - _skinY },
            { (float) drawOffset.x + bounds.maxs.x - _skinX, (float) drawOffset.y + bounds.maxs.y - _skinY }, ui().GetColor(EditorColorId::SelectBox).u32);
    }

    drawer->Flags = oldFlags;
}

bool UVRenderer::mousePressEvent(ImGuiMouseButton buttons, ImVec2 localPos)
{
    _dragging = true;
    _dragPos = _downPos = { localPos.x, localPos.y }; //mapFromGlobal(QCursor::pos()) * devicePixelRatio();
    _dragWorldPos = _dragPos;
    _dragDelta = {};

    return ui().editorUV().tool() != EditorTool::Select;
}

void UVRenderer::rectangleSelect(aabb2 rect, glm::vec2 tcScale)
{
    auto &mdl = model().model();
    auto mutator = model().mutator();

    if (rect.mins == rect.maxs)
    {
        float w = 5 / tcScale.x;
        float h = 5 / tcScale.y;

        rect = aabb2::fromMinsMaxs(
            rect.mins - glm::vec2(w / 2, h / 2),
            rect.maxs + glm::vec2(w, h)
        );
    }

    if (ui().editorUV().selectMode() == SelectMode::Vertex)
    {
        mutator.selectRectangleVerticesUV(rect);
    }
    else
    {
        mutator.selectRectangleTrianglesUV(rect);
    }
}

void UVRenderer::mouseReleaseEvent(ImVec2 localPos)
{
    if (!_dragging)
        return;

    auto &mdl = model().model();
    auto skin = mdl.getSelectedSkin();

    if (skin)
    {
        int scale = ui().editorUV().scale();
        glm::vec2 tcScale{(float) skin->width * scale, (float) skin->height * scale};

        if (_dragging)
        {
            if (ui().editorUV().tool() == EditorTool::Select)
                rectangleSelect(aabb2::fromMinsMaxs(glm::vec2 { _dragWorldPos.x - _skinX, _dragWorldPos.y - _skinY } / tcScale, glm::vec2(_dragPos.x - _skinX, _dragPos.y - _skinY) / tcScale).normalize(), tcScale);
            else
            {
                glm::mat4 drag = getDragMatrix();

                if (!glm::all(glm::equal(drag, glm::identity<glm::mat4>())))
                    model().mutator().applyUVMatrix(drag, ui().editorUV().selectMode());
            }
        }
    }

    _dragging = false;

    //MainWindow::instance().updateRenders();
}

glm::mat4 UVRenderer::getDragMatrix()
{
    glm::mat4 matrix = glm::identity<glm::mat4>();
    
    if (!_dragging || (!_dragDelta.x && !_dragDelta.y))
        return matrix;

    auto &mdl = model().model();
    auto skin = mdl.getSelectedSkin();

    if (!skin)
        return matrix;

    float zoom = (float) ui().editorUV().scale();

    if (ui().editorUV().tool() == EditorTool::Move)
    {
        if (ui().editorUV().axis().X)
            matrix = glm::translate(matrix, { -_dragDelta.x / zoom, 0.0f, 0.0f });
        if (ui().editorUV().axis().Y)
            matrix = glm::translate(matrix, { 0.0f, -_dragDelta.y / zoom, 0.0f });
    }
    else if (ui().editorUV().tool() == EditorTool::Scale)
    {
        float s = 1.0f + (_dragDelta.y * 0.01f) / zoom;
        matrix = glm::translate(matrix, { (_dragWorldPos.x - _skinX) / zoom, (_dragWorldPos.y - _skinY) / zoom, 0.f });
        if (ui().editorUV().axis().X)
            matrix = glm::scale(matrix, glm::vec3(s, 1.0f, 1.0f));
        if (ui().editorUV().axis().Y)
            matrix = glm::scale(matrix, glm::vec3(1.0f, s, 1.0f));
        matrix = glm::translate(matrix, { -(_dragWorldPos.x - _skinX) / zoom, -(_dragWorldPos.y - _skinY) / zoom, 0.f });
    }
    else if (ui().editorUV().tool() == EditorTool::Rotate)
    {
        float r = 360.f * (_dragDelta.y / (float) _height);
        matrix = glm::translate(matrix, { (_dragWorldPos.x - _skinX) / zoom, (_dragWorldPos.y - _skinY) / zoom, 1.f });
        matrix = glm::rotate(matrix, glm::radians(r), { 0.f, 0.f, -1.f });
        matrix = glm::translate(matrix, { -(_dragWorldPos.x - _skinX) / zoom, -(_dragWorldPos.y - _skinY) / zoom, 1.f });
    }

    return matrix;
}

void UVRenderer::focusLost()
{
#if 0
    QMouseEvent ev {
        QEvent::Type::MouseButtonRelease,
        mapFromGlobal(QCursor::pos()) * devicePixelRatio(),
        QCursor::pos(),
        Qt::MouseButton::NoButton,
        {},
        {}
    };
    QUVPainter::mouseReleaseEvent(&ev);
#endif
}

void UVRenderer::mouseMoveEvent(ImVec2 localPos)
{
    glm::ivec2 pos { localPos.x, localPos.y };//auto pos = mapFromGlobal(QCursor::pos()) * devicePixelRatio();
    float zoom = ui().editorUV().scale();

    if (_dragging)
    {
        glm::ivec2 delta = { (int) -localPos.x, (int) -localPos.y };

        if (!delta.x && !delta.y)
            return;

        _dragDelta += delta;
        
        float xDelta = delta.x / zoom;
        float yDelta = delta.y / zoom;
        
        if (ui().editorUV().tool() == EditorTool::Select)
            _dragPos = pos;
        
        //MainWindow::instance().updateRenders();
        return;
    }
}

void UVRenderer::mouseWheelEvent(int delta)
{
    if (ImGui::GetIO().KeyCtrl)
        ui().editorUV().scale() = std::max(1, ui().editorUV().scale() + delta);
}