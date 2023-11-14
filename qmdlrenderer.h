#pragma once

#include <QOpenGLWidget>
#include <qopenglfunctions_3_3_compatibility.h>
#include <QMouseEvent>
#include <QMatrix4x4>
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

struct GPUAxisData
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

class QMDLRenderer : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Compatibility
{
#ifdef RENDERDOC_SUPPORT
    RENDERDOC_API_1_1_2 *rdoc_api = NULL;
#endif

public:
    QMDLRenderer(QWidget *parent) : QOpenGLWidget(parent), QOpenGLFunctions_3_3_Compatibility()
    {
#ifdef RENDERDOC_SUPPORT
        if(HMODULE mod = GetModuleHandleA("renderdoc.dll"))
        {
            pRENDERDOC_GetAPI RENDERDOC_GetAPI =
                (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
            assert(ret == 1);
        }
#endif

        setMouseTracking(true);

        _camera.setBehavior(Camera::CameraBehavior::CAMERA_BEHAVIOR_ORBIT);
        _camera.lookAt({ 25.f, 0, 0 }, {}, { 0, 1, 0 });
		_camera.zoom(Camera::DEFAULT_ORBIT_OFFSET_DISTANCE, _camera.getOrbitMinZoom(), _camera.getOrbitMaxZoom());
    }

protected:
    inline void generateGrid(float grid_size, size_t count)
    {
        size_t point_count = ((count * 4) + 4) * 2;
        std::vector<QVector3D> points;
        points.reserve(point_count);

        // corners
        float extreme = grid_size * count;
        
        points.emplace_back(-extreme, -extreme, 0);
        points.emplace_back(extreme, -extreme, 0);
        
        points.emplace_back(-extreme, extreme, 0);
        points.emplace_back(extreme, extreme, 0);
        
        points.emplace_back(-extreme, -extreme, 0);
        points.emplace_back(-extreme, extreme, 0);
        
        points.emplace_back(extreme, extreme, 0);
        points.emplace_back(extreme, -extreme, 0);

        // lines
        for (size_t i = 0; i < count; i++)
        {
            float v = grid_size * i;

            // x
            points.emplace_back(v, -extreme, 0);
            points.emplace_back(v, extreme, 0);
            points.emplace_back(-v, -extreme, 0);
            points.emplace_back(-v, extreme, 0);

            // y
            points.emplace_back(-extreme, v,  0);
            points.emplace_back(extreme, v, 0);
            points.emplace_back(-extreme, -v, 0);
            points.emplace_back(extreme, -v, 0);
        }

        glBufferData(GL_ARRAY_BUFFER, point_count * sizeof(QVector3D), points.data(), GL_STATIC_DRAW);
        _gridSize = point_count;
    }

    inline void generateAxis()
    {
        constexpr GPUAxisData axisData[] = {
            { { 0.0f, 0.0f, 0.0f }, { 255, 0, 0, 127 } },
            { { 32.0f, 0.0f, 0.0f }, { 255, 0, 0, 127 } },

            { { 0.0f, 0.0f, 0.0f }, { 0, 255, 0, 127 } },
            { { 0.0f, 32.0f, 0.0f }, { 0, 255, 0, 127 } },

            { { 0.0f, 0.0f, 0.0f }, { 0, 0, 255, 127 } },
            { { 0.0f, 0.0f, 32.0f }, { 0, 0, 255, 127 } },
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(axisData), axisData, GL_STATIC_DRAW);
    }

    void initializeGL() override
    {
        initializeOpenGLFunctions();

        glFrontFace(GL_CW);

        glActiveTexture(GL_TEXTURE0);
        
        {
            glGenTextures(1, &_whiteTexture);
            byte textureData[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
            glBindTexture(GL_TEXTURE_2D, _whiteTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &textureData);
        }
        
        {
            glGenTextures(1, &_blackTexture);
            byte textureData[4] = { 0x00, 0x00, 0x00, 0xFF };
            glBindTexture(GL_TEXTURE_2D, _blackTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &textureData);
        }

        glGenTextures(1, &_modelTexture);

        const char *vertexShaderSource = R"SHADER(
#version 330 core

uniform mat4 u_projection;
uniform mat4 u_modelview;
 
in vec3 i_position;
in vec2 i_texcoord;
in vec4 i_color;

out vec4 v_color;
out vec2 v_texcoord;
 
void main()
{
    v_color = i_color;
    v_texcoord = i_texcoord;
    gl_Position = u_projection * u_modelview * vec4(i_position, 1.0);
}
)SHADER";
 
const char *fragmentShaderSource = R"SHADER(
#version 330 core 
 
precision mediump float;

uniform sampler2D u_texture;

in vec4 v_color;
in vec2 v_texcoord;
 
out vec4 o_color;
 
void main()
{
    o_color = texture2D(u_texture, v_texcoord) * v_color;
}
)SHADER";

        GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSource);
        GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

        _program = createProgram(vertexShader, fragmentShader);
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        _projectionUniformLocation = glGetUniformLocation(_program, "u_projection");
        _modelviewUniformLocation = glGetUniformLocation(_program, "u_modelview");

        glUniform1i(glGetUniformLocation(_program, "u_texture"), 0);
        
        _positionAttributeLocation = glGetAttribLocation(_program, "i_position");
        _texcoordAttributeLocation = glGetAttribLocation(_program, "i_texcoord");
        _colorAttributeLocation = glGetAttribLocation(_program, "i_color");

        glGenBuffers(1, &_gridBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, _gridBuffer);
        generateGrid(8.0f, 8);

        glGenVertexArrays(1, &_gridVao);
        glBindVertexArray(_gridVao);
        glEnableVertexAttribArray(_positionAttributeLocation);
        glDisableVertexAttribArray(_texcoordAttributeLocation);
        glDisableVertexAttribArray(_colorAttributeLocation);
        glVertexAttribPointer(_positionAttributeLocation, 3, GL_FLOAT, false, sizeof(QVector3D), nullptr);
        
        glGenBuffers(1, &_axisBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, _axisBuffer);
        generateAxis();

        glGenVertexArrays(1, &_axisVao);
        glBindVertexArray(_axisVao);
        glEnableVertexAttribArray(_positionAttributeLocation);
        glDisableVertexAttribArray(_texcoordAttributeLocation);
        glEnableVertexAttribArray(_colorAttributeLocation);
        glVertexAttribPointer(_positionAttributeLocation, 3, GL_FLOAT, false, sizeof(GPUAxisData), reinterpret_cast<const GLvoid *>(offsetof(GPUAxisData, position)));
        glVertexAttribPointer(_colorAttributeLocation, 4, GL_UNSIGNED_BYTE, true, sizeof(GPUAxisData), reinterpret_cast<const GLvoid *>(offsetof(GPUAxisData, color)));

        glGenBuffers(1, &_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, _buffer);

        glGenVertexArrays(1, &_vao);
        glBindVertexArray(_vao);
        glEnableVertexAttribArray(_positionAttributeLocation);
        glEnableVertexAttribArray(_texcoordAttributeLocation);
        glDisableVertexAttribArray(_colorAttributeLocation);
        glVertexAttribPointer(_positionAttributeLocation, 3, GL_FLOAT, false, sizeof(GPUVertexData), reinterpret_cast<const GLvoid *>(offsetof(GPUVertexData, position)));
        glVertexAttribPointer(_texcoordAttributeLocation, 2, GL_FLOAT, false, sizeof(GPUVertexData), reinterpret_cast<const GLvoid *>(offsetof(GPUVertexData, texcoord)));
    }

    void resizeGL(int w, int h) override
    {
    }

    inline QuadrantFocus getQuadrantFocus(int x, int y)
    {
        int w = this->width();
        int h = this->height();
        
        int quadrant_w = (w / 2) - 1;
        int quadrant_h = (h / 2) - 1;
        
        int line_width = 2 + (w & 1);
        int line_height = 2 + (h & 1);
        
        bool within_x = x >= quadrant_w && x <= quadrant_w + line_width;
        bool within_y = y >= quadrant_h && y <= quadrant_h + line_height;
        
        if (within_x && within_y)
            return QuadrantFocus::Center;
        else if (within_x)
            return QuadrantFocus::Horizontal;
        else if (within_y)
            return QuadrantFocus::Vertical;

        if (x < quadrant_w && y < quadrant_h)
            return QuadrantFocus::TopLeft;
        else if (x > quadrant_w + line_width && y < quadrant_h)
            return QuadrantFocus::TopRight;
        else if (x > quadrant_w + line_width && y > quadrant_h + line_height)
            return QuadrantFocus::BottomRight;
        else
            return QuadrantFocus::BottomLeft;
    }

    QuadrantFocus _focusedQuadrant = QuadrantFocus::None;
    bool _dragging = false;
    QPoint _dragPos;
    Qt::MouseButton _dragButton;

    void mousePressEvent(QMouseEvent *e) override
    {
        _dragging = true;
        _dragButton = e->button();
        _dragPos = e->pos();
    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        _dragging = false;
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (_dragging)
        {
            auto delta = _dragPos - event->pos();

            if (_focusedQuadrant == QuadrantFocus::TopRight)
            {
#if 0
                _3dAngles = QQuaternion::fromAxisAndAngle({ 0, -1, 0 }, delta.x()) * _3dAngles;

                QVector3D axis;
                float angle;
			    _3dAngles.getAxisAndAngle(&axis, &angle);
                axis.normalize();

                auto m_xAxis = QVector3D::crossProduct({ 0, -1, 0 }, axis);
			    m_xAxis.normalize();

			    auto m_yAxis = QVector3D::crossProduct(axis, m_xAxis);
			    m_yAxis.normalize();

                _3dAngles = QQuaternion::fromAxisAndAngle(m_yAxis, delta.y()) * _3dAngles;
#endif
                if (_dragButton == Qt::MouseButton::RightButton)
                    _camera.zoom(-delta.y(), _camera.getOrbitMinZoom(), _camera.getOrbitMaxZoom());
                else if (_dragButton == Qt::MouseButton::LeftButton)
                    _camera.rotate(delta.y(), delta.x(), 0);
		        this->update();
            }
            else if (_focusedQuadrant == QuadrantFocus::TopLeft ||
                     _focusedQuadrant == QuadrantFocus::BottomLeft ||
                     _focusedQuadrant == QuadrantFocus::BottomRight)
            {
                if (_dragButton == Qt::MouseButton::RightButton)
                {
                    _2dZoom += delta.y() * 0.01f;
		            this->update();
                }
                else if (_dragButton == Qt::MouseButton::LeftButton)
                {
                    float xDelta = delta.x() / _2dZoom;
                    float yDelta = delta.y() / _2dZoom;

                    if (_focusedQuadrant == QuadrantFocus::TopLeft)
                        _2dOffset += QVector3D(-xDelta, yDelta, 0);
                    else if (_focusedQuadrant == QuadrantFocus::BottomLeft)
                        _2dOffset += QVector3D(-xDelta, 0, yDelta);
                    else
                        _2dOffset += QVector3D(0, -xDelta, yDelta);
		            this->update();
                }
            }

            _dragPos = event->pos();
            return;
        }

        _focusedQuadrant = getQuadrantFocus(event->pos().x(), event->pos().y());
        
        int w = this->width();
        int h = this->height();
        
        int quadrant_w = (w / 2) - 1;
        int quadrant_h = (h / 2) - 1;
        
        int line_width = 2 + (w & 1);
        int line_height = 2 + (h & 1);
        
        bool within_x = event->pos().x() >= quadrant_w && event->pos().x() <= quadrant_w + line_width;
        bool within_y = event->pos().y() >= quadrant_h && event->pos().y() <= quadrant_h + line_height;
        
        if (_focusedQuadrant == QuadrantFocus::Center)
            setCursor(Qt::SizeAllCursor);
        else if (_focusedQuadrant == QuadrantFocus::Horizontal)
            setCursor(Qt::SizeHorCursor);
        else if (_focusedQuadrant == QuadrantFocus::Vertical)
            setCursor(Qt::SizeVerCursor);
        else
            setCursor(Qt::OpenHandCursor);
    }

    void leaveEvent(QEvent *event) override
    {
        _focusedQuadrant = QuadrantFocus::None;
    }

    inline void clearQuadrant(int x, int y, int w, int h, QVector4D color)
    {
        glViewport(x, y, w, h);
        glScissor(x, y, w, h);
        glClearColor(color[0], color[1], color[2], color[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    inline void setupWireframe()
    {
        glBindTexture(GL_TEXTURE_2D, _blackTexture);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    inline void setupTextured()
    {
        glBindTexture(GL_TEXTURE_2D, _modelTexture);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    inline void drawModels(bool is_2d)
    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        setupTextured();
        glBindTexture(GL_TEXTURE_2D, _whiteTexture);

        glBindVertexArray(_gridVao);
        glBindBuffer(GL_ARRAY_BUFFER, _gridBuffer);
        glVertexAttrib4f(_colorAttributeLocation, 1.0f, 0.5f, 0.0f, 0.50f);
        glVertexAttrib2f(_texcoordAttributeLocation, 1.0f, 1.0f);
        glDrawArrays(GL_LINES, 0, _gridSize);

        glBindVertexArray(_axisVao);
        glBindBuffer(GL_ARRAY_BUFFER, _axisBuffer);
        glDrawArrays(GL_LINES, 0, 6);

        glEnable(GL_DEPTH_TEST);

        glBindVertexArray(_vao);
        glBindBuffer(GL_ARRAY_BUFFER, _buffer);
        glVertexAttrib4f(_colorAttributeLocation, 1.0f, 1.0f, 1.0f, 1.0f);

        if (is_2d)
            setupWireframe();
        else
        {
            glEnable(GL_CULL_FACE);
            setupTextured();
        }

        glDrawArrays(GL_TRIANGLES, 0, _bufferData.size());
    }
    
    void paintGL() override
    {
#ifdef RENDERDOC_SUPPORT
        if(rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);
#endif

        int w = this->width();
        int h = this->height();

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        int quadrant_w = (w / 2) - 1;
        int quadrant_h = (h / 2) - 1;
        
        int line_width = 2 + (w & 1);
        int line_height = 2 + (h & 1);

        glUseProgram(_program);

        glEnable(GL_SCISSOR_TEST);

        QMatrix4x4 projection;
        float hw = (quadrant_w / 2.0f);
        float hh = (quadrant_h / 2.0f);

        projection.ortho(-hw, hw, -hh, hh, -4096.f, 4096.f);
        glUniformMatrix4fv(_projectionUniformLocation, 1, false, projection.data());

        QMatrix4x4 modelview;
        modelview.setToIdentity();
        modelview.scale(_2dZoom);
        modelview.translate(_2dOffset.x(), _2dOffset.y());
        modelview.rotate(-90, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(_modelviewUniformLocation, 1, false, modelview.data());
        
        // top-left
        clearQuadrant(0, quadrant_h + line_height, quadrant_w, quadrant_h, { 0.4f, 0.4f, 0.4f, 1.0f });

        drawModels(true);

        modelview.setToIdentity();
        modelview.scale(_2dZoom);
        modelview.translate(_2dOffset.x(), _2dOffset.z());
        modelview.rotate(-90, 0.0f, 0.0f, 1.0f);
        modelview.rotate(-90, 0.0f, 1.0f, 0.0f);
        glUniformMatrix4fv(_modelviewUniformLocation, 1, false, modelview.data());

        // bottom-left
        clearQuadrant(0, 0, quadrant_w, quadrant_h, { 0.4f, 0.4f, 0.4f, 1.0f });

        drawModels(true);

        modelview.setToIdentity();
        modelview.scale(_2dZoom);
        modelview.translate(_2dOffset.y(), _2dOffset.z());
        modelview.rotate(-90, 0.0f, 0.0f, 1.0f);
        modelview.rotate(-90, 0.0f, 1.0f, 0.0f);
        modelview.rotate(-90, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(_modelviewUniformLocation, 1, false, modelview.data());

        // bottom-right
        clearQuadrant(quadrant_w + line_width, 0, quadrant_w, quadrant_h, { 0.4f, 0.4f, 0.4f, 1.0f });

        drawModels(true);
        
        // top-right
        _camera.perspective(45, (float) quadrant_w / quadrant_h, 0.1f, 1024.f);
        projection = _camera.getProjectionMatrix();
        glUniformMatrix4fv(_projectionUniformLocation, 1, false, projection.data());
        
        modelview = _camera.getViewMatrix();
        modelview.rotate(-90, { 1, 0, 0 });
        modelview.translate(_2dOffset);
        glUniformMatrix4fv(_modelviewUniformLocation, 1, false, modelview.data());

        clearQuadrant(quadrant_w + line_width, quadrant_h + line_height, quadrant_w, quadrant_h, { 0.4f, 0.4f, 0.4f, 1.0f });

        drawModels(false);

        glDisable(GL_SCISSOR_TEST);
        
#ifdef RENDERDOC_SUPPORT
        if(rdoc_api) rdoc_api->EndFrameCapture(NULL, NULL);
#endif
    }

private:
    GLuint _program;
    GLint _positionAttributeLocation, _texcoordAttributeLocation, _colorAttributeLocation;
    GLint _projectionUniformLocation, _modelviewUniformLocation;
    GLuint _buffer, _axisBuffer, _gridBuffer;
    GLuint _whiteTexture, _blackTexture, _modelTexture;
    size_t _gridSize;
    std::vector<GPUVertexData> _bufferData;
    GLuint _vao, _axisVao, _gridVao;
    const ModelData *_model;
    float _2dZoom = 1.0f;
    QVector3D _2dOffset = {};
    Camera _camera;

	GLuint createShader(GLenum type, const char *source)
	{
		GLuint shader = glCreateShader(type);
        const char *sources[] = { source };
        GLint lengths[] = { (GLint) strlen(source) };

		glShaderSource(shader, 1, sources, lengths);
		glCompileShader(shader);

        GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

		if (status == GL_TRUE)
			return shader;

        static char infoLog[1024];
        static GLsizei length;
        glGetShaderInfoLog(shader, sizeof(infoLog), &length, infoLog);
		glDeleteShader(shader);
        throw std::runtime_error(infoLog);
	}

	GLuint createProgram(GLuint vertexShader, GLuint fragmentShader)
    {
		GLuint program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);

        GLint status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);

        if (status == GL_TRUE)
			return program;
        
        static char infoLog[1024];
        static GLsizei length;
        glGetProgramInfoLog(program, sizeof(infoLog), &length, infoLog);
		glDeleteProgram(program);
        throw std::runtime_error(infoLog);
	}

    void rebuildBuffer()
    {
        size_t full_size = sizeof(GPUVertexData) * _model->triangles.size() * 3;
        bool full_upload = _bufferData.size() < (_model->triangles.size() * 3);
        _bufferData.resize(_model->triangles.size() * 3);
        size_t n = 0;

        for (auto &tri : this->_model->triangles)
        {
            auto &v0 = this->_model->frames[0].vertices[tri.vertices[0]];
            auto &v1 = this->_model->frames[0].vertices[tri.vertices[1]];
            auto &v2 = this->_model->frames[0].vertices[tri.vertices[2]];
            
            auto &st0 = this->_model->texcoords[tri.texcoords[0]];
            auto &st1 = this->_model->texcoords[tri.texcoords[1]];
            auto &st2 = this->_model->texcoords[tri.texcoords[2]];
            
            auto &ov0 = _bufferData[n++];
            auto &ov1 = _bufferData[n++];
            auto &ov2 = _bufferData[n++];
            
            ov0.position = v0.position;
            ov1.position = v1.position;
            ov2.position = v2.position;
            
            ov0.texcoord = st0;
            ov1.texcoord = st1;
            ov2.texcoord = st2;
        }

        glBindBuffer(GL_ARRAY_BUFFER, _buffer);

        if (full_upload)
            glBufferData(GL_ARRAY_BUFFER, full_size, _bufferData.data(), GL_DYNAMIC_DRAW);
        else
            glBufferSubData(GL_ARRAY_BUFFER, 0, full_size, _bufferData.data());
    }

public:
    void setModel(const ModelData *model)
    {
        this->_model = model;
        this->rebuildBuffer();

        glBindTexture(GL_TEXTURE_2D, _modelTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, model->skins[0].width, model->skins[0].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, model->skins[0].data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //glGenerateMipmap(GL_TEXTURE_2D);

		this->update();
    }
};
