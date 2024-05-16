#pragma once

#include "Math.h"
#include "Camera.h"
#ifdef RENDERDOC_SUPPORT
#include "renderdoc_app.h"
#endif
#include "ModelData.h"
#include <KHR/khrplatform.h>
#include <glad/glad.h>
#include <imgui.h>

struct GPURenderData
{
    enum {
        FLAG_DRAG_SELECTED = 1,
        FLAG_UV_SELECTED   = 2,
        FLAG_FACE_MODE     = 4,
    };

    glm::mat4 drag3DMatrix;
    glm::mat4 dragUVMatrix;
    int       flags;
    int       padding[3];
};

struct GPUVertexData
{
    glm::vec3 position;
    glm::vec2 texcoord;
    int       selected;
    int       selectedVertex;
};

struct GPUPointData
{
    glm::vec3   position;
    Color       color;
    int         selected;
};

struct GPUNormalData
{
    glm::vec3   position;
    glm::vec3   normal;
    int         selected;
};

enum class QuadrantFocus
{
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Horizontal,
    Vertical,
    Center,

    Quadrants = 4,
    None = -1
};

enum class Orientation2D
{
    XY,
    ZY,
    XZ
};

struct QuadRect { int x, y, w, h; };

struct ProgramShared
{
    GLuint program;
    GLint projectionUniformLocation,
          modelviewUniformLocation,
          shadedUniformLocation,
          is2DLocation,
          isLineLocation,
          face3DLocation,
          face2DLocation,
          line3DLocation,
          line2DLocation;
};

struct QuadrantMatrices
{
    Matrix4 projection;
    Matrix4 modelview;
};

class MDLRenderer
{
#ifdef RENDERDOC_SUPPORT
    RENDERDOC_API_1_1_2 *rdoc_api = NULL;
#endif

public:
    MDLRenderer();

    void captureRenderDoc(bool);
    void focusLost();
    void modelLoaded();
    void selectedSkinChanged();

//protected:
    void initializeGL();

    void resize(int available_width, int available_height);
    void draw();
    
    const int &width() { return _width; }
    const int &height() { return _height; }
    bool &viewWeaponMode() { return _viewWeaponMode; }
    int &fov();

    bool mousePressEvent(ImGuiMouseButton buttons, ImVec2 localPos);
    void mouseReleaseEvent(ImVec2 localPos);
    void mouseMoveEvent(ImVec2 localPos);
    void mouseWheelEvent(int delta);
#if 0
    void leaveEvent();
#endif

    void paint();
    void updateTextures();

    GLuint getRendererTexture();

    void colorsChanged();
    void markBufferDirty() { _bufferDirty = true; }

private:
    void generateGrid(float grid_size, size_t count);
    void generateAxis();

    QuadRect getQuadrantRect(QuadrantFocus quadrant);
    QuadrantFocus getQuadrantFocus(glm::ivec2 xy);
    QuadrantMatrices getQuadrantMatrices(QuadrantFocus quadrant);
    void clearQuadrant(QuadRect rect, Color color);
    void scissorQuadrant(QuadRect rect);
    void drawModels(QuadrantFocus quadrant, bool is_2d);
    void draw2D(Orientation2D orientation, QuadrantFocus quadrant);
    void draw3D(QuadrantFocus quadrant);
    glm::mat4 getDragMatrix();
    glm::vec3 mouseToWorld(glm::ivec2 pos);
    glm::vec2 worldToMouse(const glm::vec3 &pos, const Matrix4 &projection, const Matrix4 &modelview, const QuadRect &viewport, bool local);
    void rectangleSelect(aabb2 rect);
    
#if 0
    std::unique_ptr<QOpenGLDebugLogger> _logger;
    QElapsedTimer _animationTimer;
#endif
    QuadrantFocus _focusedQuadrant = QuadrantFocus::None;
    bool _dragging = false;
    glm::vec3 _dragWorldPos {};
    glm::ivec2 _dragPos {}, _downPos {}, _dragDelta {};
    ImGuiMouseButton _dragButtons = 0;
    ProgramShared _modelProgram {}, _simpleProgram {}, _normalProgram {};
    GLuint _nearestSampler = 0, _filteredSampler = 0;

    GLuint _fbo = 0;
    GLuint _fboColor = 0, _fboDepth = 0;

    GLuint _buffer = 0, _pointBuffer = 0, _smoothNormalBuffer = 0, _flatNormalBuffer = 0, _axisBuffer = 0, _gridBuffer = 0, _normalsBuffer = 0;
    GLuint _whiteTexture = 0, _blackTexture = 0;
    size_t _gridSize = 0;
    GLuint _uboIndex = 0, _uboObject = 0;
    GPURenderData _uboData;
    std::vector<GPUVertexData> _bufferData;
    std::vector<GPUPointData> _pointData;
    std::vector<GPUNormalData> _normalsData;
    std::vector<glm::vec3> _smoothNormalData, _flatNormalData;
    GLuint _vao = 0, _pointVao = 0, _axisVao = 0, _gridVao = 0, _normalVao;
    float _2dZoom = 1.0f;
    glm::vec3 _2dOffset = {};
    float _gridZ = 0.0f;
    Camera _camera, _viewModelCamera;
#ifdef RENDERDOC_SUPPORT
    bool _doRenderDoc;
#endif
    int _width = 0, _height = 0;
    bool _viewWeaponMode = false;
    bool _bufferDirty = true;

	GLuint createShader(GLenum type, const char *source);
	GLuint createProgram(GLuint vertexShader, GLuint fragmentShader);
    void rebuildBuffer();
};
