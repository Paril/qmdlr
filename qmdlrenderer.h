#pragma once

#define RENDERDOC_SUPPORT

#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QElapsedTimer>
#include "camera.h"
#ifdef RENDERDOC_SUPPORT
#include "renderdoc_app.h"
#endif
#include "modeldata.h"

struct GPUVertexData
{
    QVector3D   position;
    QVector2D   texcoord;
};

struct GPUPointData
{
    QVector3D               position;
    std::array<uint8_t, 4>  color;
};

enum class QuadrantFocus
{
    None,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Horizontal,
    Vertical,
    Center
};

enum class Orientation2D
{
    XY,
    ZY,
    XZ
};

struct QuadRect { int x, y, w, h; };

class QMDLRenderer : public QOpenGLWidget
{
#ifdef RENDERDOC_SUPPORT
    RENDERDOC_API_1_1_2 *rdoc_api = NULL;
#endif

public:
    QMDLRenderer(QWidget *parent);

    void setModel(const ModelData *model);
    void captureRenderDoc(bool);
    void resetAnimation();
    void setAnimated(bool animating);

protected:
    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

    void paintGL() override;

private:
    void generateGrid(float grid_size, size_t count);
    void generateAxis();

    QuadRect getQuadrantRect(QuadrantFocus quadrant);
    QuadrantFocus getQuadrantFocus(QPoint xy);

    void clearQuadrant(QuadRect rect, QVector4D color);
    void setupWireframe();
    void setupTextured();
    void drawModels(const Matrix4 &projection, bool is_2d);
    void draw2D(Orientation2D orientation, QuadrantFocus quadrant);
    void draw3D(QuadrantFocus quadrant);
    
    QElapsedTimer _animationTimer;
    QuadrantFocus _focusedQuadrant = QuadrantFocus::None;
    bool _dragging = false;
    QPoint _dragPos;
    Qt::MouseButton _dragButton;
    float _horizontalSplit, _verticalSplit;
    GLuint _program;
    GLint _positionAttributeLocation, _texcoordAttributeLocation, _colorAttributeLocation;
    GLint _projectionUniformLocation, _modelviewUniformLocation;
    GLuint _buffer, _pointBuffer, _axisBuffer, _gridBuffer;
    GLuint _whiteTexture, _blackTexture, _modelTexture;
    size_t _gridSize;
    std::vector<GPUVertexData> _bufferData;
    std::vector<GPUPointData> _pointData;
    GLuint _vao, _pointVao, _axisVao, _gridVao;
    const ModelData *_model = nullptr;
    float _2dZoom = 1.0f;
    QVector3D _2dOffset = {};
    Camera _camera;
#ifdef RENDERDOC_SUPPORT
    bool _doRenderDoc;
#endif

	GLuint createShader(GLenum type, const char *source);
	GLuint createProgram(GLuint vertexShader, GLuint fragmentShader);
    void rebuildBuffer();
};
