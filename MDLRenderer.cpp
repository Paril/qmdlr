#include <optional>
#include <filesystem>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include "ModelLoader.h"
#include "MDLRenderer.h"
#include "Settings.h"
#include "System.h"
#include "UI.h"
#define TCPP_IMPLEMENTATION
#include <tcpp/tcppLibrary.hpp>
#include "Log.h"

enum : GLuint
{
    ATTRIB_POSITION,
    ATTRIB_TEXCOORD,
    ATTRIB_COLOR,
    ATTRIB_NORMAL,
    ATTRIB_SELECTED,
    ATTRIB_SELECTED_VERTEX,
    ATTRIB_COUNT
};

struct GPUAxisData
{
    glm::vec3   position;
    Color       color;
};

// handles lifecycle for model skins
struct MDLSkinDataHandle : public RendererSkinHandle
{
    MDLSkinDataHandle(ModelSkin &skin)
    {
        glGenTextures(1, &_id);

        Bind();

        if (skin.image.is_indexed_valid())
            skin.image.convert_to_rgba();

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, skin.width, skin.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, skin.image.rgba());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    virtual ~MDLSkinDataHandle() override
    {
        if (!_id)
            return;

        glDeleteTextures(1, &_id);
    }

    virtual void MarkDirty() override
    {
        _dirty = true;
    }

    virtual void Update(ModelSkin &skin) override
    {
        if (!_dirty)
            return;

        Bind();

        if (skin.image.is_indexed_valid())
            skin.image.convert_to_rgba();

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, skin.width, skin.height, GL_RGBA, GL_UNSIGNED_BYTE, skin.image.rgba());
        glGenerateMipmap(GL_TEXTURE_2D);
        _dirty = false;
    }

    virtual void Bind() const override
    {
        glBindTexture(GL_TEXTURE_2D, _id);
    }

    virtual ImTextureID GetTextureHandle() const override
    {
        return (ImTextureID) (ptrdiff_t) _id;
    }

private:
    GLuint  _id = 0;
    bool    _dirty = false;
};

MDLRenderer::MDLRenderer()
{
    _camera.setBehavior(Camera::CameraBehavior::CAMERA_BEHAVIOR_ORBIT);
    _camera.lookAt({ 25.f, 0, 0 }, {}, { 0, 1, 0 });
	_camera.zoom(Camera::DEFAULT_ORBIT_OFFSET_DISTANCE, _camera.getOrbitMinZoom(), _camera.getOrbitMaxZoom());

    _viewModelCamera.setBehavior(Camera::CameraBehavior::CAMERA_BEHAVIOR_ORBIT);
    _viewModelCamera.lookAt({ 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 });
	_viewModelCamera.zoom(0.f, 0.f, 0.f);
}

class STLFileStream : public tcpp::IInputStream
{
    std::ifstream file;

public:
    inline STLFileStream(const char *filename) :
        file(filename, std::ios_base::in)
    {
    }

    inline STLFileStream(const std::filesystem::path &filename) :
        file(filename, std::ios_base::in)
    {
    }

	virtual std::string ReadLine() TCPP_NOEXCEPT override
    {
        std::string s;
        std::getline(file, s);
        return s + '\n';
    }

    virtual bool HasNextLine() const TCPP_NOEXCEPT override
    {
        return file.good();
    }
};

static std::optional<std::filesystem::path> resolveShaderPath(const char *filename)
{
    std::filesystem::path shaderPath("res/shaders");

    if (!std::filesystem::exists(shaderPath))
    {
        shaderPath = "../../.." / shaderPath;

        if (!std::filesystem::exists(shaderPath))
            return std::nullopt;
    }

    std::filesystem::path file = shaderPath / filename;

    if (!std::filesystem::exists(file))
        return std::nullopt;

    return file;
}

static std::string preprocessShader(const char *filename)
{
    tcpp::Lexer lexer(std::make_unique<STLFileStream>(filename));
    tcpp::Preprocessor::TPreprocessorConfigInfo info;
    info.mSkipComments = true;
    //TInputStreamUniquePtr(const std::string&, bool)
    info.mOnIncludeCallback = [](const std::string &str, bool something) -> tcpp::TInputStreamUniquePtr
    {
        auto file = resolveShaderPath(str.c_str());

        if (file)
            return std::make_unique<STLFileStream>(file.value());

        return nullptr;
    };
    tcpp::Preprocessor p(lexer, info);
    p.AddCustomDirectiveHandler("version", [](tcpp::Preprocessor& p, tcpp::Lexer& l, const std::string& s) -> std::string {
        std::string versionString = "#version";
        size_t line = l.GetCurrLineIndex();

        while (l.HasNextToken() && l.PeekNextToken().mLineId == line)
            versionString += l.GetNextToken().mRawView;

        return versionString + "\n";
    });
    return p.Process();
}

void MDLRenderer::generateGrid(float grid_size, size_t count)
{
    size_t point_count = ((count * 4) + 4) * 2;
    std::vector<glm::vec3> points;
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

    glBufferData(GL_ARRAY_BUFFER, point_count * sizeof(glm::vec3), points.data(), GL_STATIC_DRAW);
    _gridSize = point_count;
}

void MDLRenderer::generateAxis()
{
    const Color &xColor = ui().GetColor(EditorColorId::OriginX);
    const Color &yColor = ui().GetColor(EditorColorId::OriginY);
    const Color &zColor = ui().GetColor(EditorColorId::OriginZ);

    GPUAxisData axisData[] = {
        { { 0.0f, 0.0f, 0.0f }, xColor },
        { { 32.0f, 0.0f, 0.0f }, xColor },

        { { 0.0f, 0.0f, 0.0f }, yColor },
        { { 0.0f, 32.0f, 0.0f }, yColor },

        { { 0.0f, 0.0f, 0.0f }, zColor },
        { { 0.0f, 0.0f, 32.0f }, zColor },
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(axisData), axisData, GL_STATIC_DRAW);
}

template<size_t w, size_t h, size_t N = w * h * 4>
static inline void createBuiltInTexture(GLuint &out, const std::initializer_list<uint8_t> &pixels)
{
    if (pixels.size() != N)
        throw std::runtime_error("bad pixels");

    glGenTextures(1, &out);
    glBindTexture(GL_TEXTURE_2D, out);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.begin());
}

static std::optional<std::string> loadShader(const char *filename)
{
    auto file = resolveShaderPath(filename);

    if (!file)
        return std::nullopt;

    return preprocessShader(file.value().string().c_str());
}

static void enableVertexAttribArrayBits(GLuint attributeBits)
{
    for (size_t i = 0; i < ATTRIB_COUNT; i++)
        if (attributeBits & (1 << i))
            glEnableVertexAttribArray(i);
        else
            glDisableVertexAttribArray(i);
}

template<typename ...TArgs>
static void enableVertexAttribArrays(TArgs ...attributes)
{
    enableVertexAttribArrayBits(((1 << attributes) | ...));
}

template<auto member>
static void vertexAttribPointer(GLuint attribute, GLint size, GLenum type, GLboolean normalized = false)
{
    // sadly, no constexpr offsetof :(
    using MT = typename detail::extract_class_type_t<decltype(member)>;
    MT val {};
    ptrdiff_t offset = reinterpret_cast<uintptr_t>(&(val.*member)) - reinterpret_cast<uintptr_t>(&val);
    glVertexAttribPointer(attribute, size, type, normalized, sizeof(MT), reinterpret_cast<const GLvoid *>(offset));
}

template<typename T>
static void vertexAttribPointer(GLuint attribute, GLint size, GLenum type, GLboolean normalized = false)
{
    glVertexAttribPointer(attribute, size, type, normalized, sizeof(T), nullptr);
}

template<auto member>
static void vertexAttribIPointer(GLuint attribute, GLint size, GLenum type)
{
    // sadly, no constexpr offsetof :(
    using MT = typename detail::extract_class_type_t<decltype(member)>;
    MT val {};
    ptrdiff_t offset = reinterpret_cast<uintptr_t>(&(val.*member)) - reinterpret_cast<uintptr_t>(&val);
    glVertexAttribIPointer(attribute, size, type, sizeof(MT), reinterpret_cast<const GLvoid *>(offset));
}

template<typename T>
static void vertexAttribIPointer(GLuint attribute, GLint size, GLenum type)
{
    glVertexAttribIPointer(attribute, size, type, sizeof(T), nullptr);
}

void MDLRenderer::initializeGL()
{
    glFrontFace(GL_CW);

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
        
    createBuiltInTexture<1, 1>(_whiteTexture, { 0xFF, 0xFF, 0xFF, 0xFF });
    createBuiltInTexture<1, 1>(_blackTexture, { 0x00, 0x00, 0x00, 0xFF });

    glGenSamplers(1, &_nearestSampler);
    glBindSampler(0, _nearestSampler);
    glSamplerParameteri(_nearestSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(_nearestSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(_nearestSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(_nearestSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenSamplers(1, &_filteredSampler);
    glBindSampler(0, _filteredSampler);
    glSamplerParameteri(_filteredSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(_filteredSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(_filteredSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(_filteredSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (GL_ARB_texture_filter_anisotropic)
        glSamplerParameterf(_filteredSampler, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);
    glBindSampler(0, _nearestSampler);

    auto makeProgramAndSetUniforms = [this](ProgramShared &shared, const char *vertex, const char *fragment) {
        shared.program = createProgram(
            createShader(GL_VERTEX_SHADER, loadShader(vertex)->c_str()),
            createShader(GL_FRAGMENT_SHADER, loadShader(fragment)->c_str()));
        glUseProgram(shared.program);

        shared.projectionUniformLocation = glGetUniformLocation(shared.program, "u_projection");
        shared.modelviewUniformLocation = glGetUniformLocation(shared.program, "u_modelview");
    };

    makeProgramAndSetUniforms(_modelProgram, "model.vert.glsl", "model.frag.glsl");

    _modelProgram.projectionUniformLocation = glGetUniformLocation(_modelProgram.program, "u_projection");
    _modelProgram.modelviewUniformLocation = glGetUniformLocation(_modelProgram.program, "u_modelview");

    glUniform1i(glGetUniformLocation(_modelProgram.program, "u_texture"), 0);
    glUniform1i(_modelProgram.shadedUniformLocation = glGetUniformLocation(_modelProgram.program, "u_shaded"), 1);
    _modelProgram.is2DLocation = glGetUniformLocation(_modelProgram.program, "u_2d");
    _modelProgram.isLineLocation = glGetUniformLocation(_modelProgram.program, "u_line");
    _modelProgram.face3DLocation = glGetUniformLocation(_modelProgram.program, "u_face3D");
    _modelProgram.face2DLocation = glGetUniformLocation(_modelProgram.program, "u_face2D");
    _modelProgram.line3DLocation = glGetUniformLocation(_modelProgram.program, "u_line3D");
    _modelProgram.line2DLocation = glGetUniformLocation(_modelProgram.program, "u_line2D");
    
    makeProgramAndSetUniforms(_simpleProgram, "simple.vert.glsl", "simple.frag.glsl");
    
    makeProgramAndSetUniforms(_normalProgram, "normals.vert.glsl", "simple.frag.glsl");

    glGenBuffers(1, &_gridBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _gridBuffer);
    generateGrid(8.0f, 8);

    glGenVertexArrays(1, &_gridVao);
    glBindVertexArray(_gridVao);
    enableVertexAttribArrays(ATTRIB_POSITION);
    vertexAttribPointer<glm::vec3>(ATTRIB_POSITION, 3, GL_FLOAT);
        
    glGenBuffers(1, &_axisBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _axisBuffer);
    generateAxis();

    glGenVertexArrays(1, &_axisVao);
    glBindVertexArray(_axisVao);
    enableVertexAttribArrays(ATTRIB_POSITION, ATTRIB_COLOR);
    vertexAttribPointer<&GPUAxisData::position>(ATTRIB_POSITION, 3, GL_FLOAT);
    vertexAttribPointer<&GPUAxisData::color>(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, true);

    glGenBuffers(1, &_buffer);
    
    glGenBuffers(1, &_smoothNormalBuffer);
    glGenBuffers(1, &_flatNormalBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, _buffer);

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    enableVertexAttribArrays(ATTRIB_POSITION, ATTRIB_TEXCOORD, ATTRIB_NORMAL, ATTRIB_SELECTED, ATTRIB_SELECTED_VERTEX);
    vertexAttribPointer<&GPUVertexData::position>(ATTRIB_POSITION, 3, GL_FLOAT);
    vertexAttribPointer<&GPUVertexData::texcoord>(ATTRIB_TEXCOORD, 2, GL_FLOAT);
    vertexAttribIPointer<&GPUVertexData::selected>(ATTRIB_SELECTED, 1, GL_INT);
    vertexAttribIPointer<&GPUVertexData::selectedVertex>(ATTRIB_SELECTED_VERTEX, 1, GL_INT);
    glBindBuffer(GL_ARRAY_BUFFER, _smoothNormalBuffer);
    vertexAttribPointer<glm::vec3>(ATTRIB_NORMAL, 3, GL_FLOAT);
    glBindBuffer(GL_ARRAY_BUFFER, _flatNormalBuffer);
    vertexAttribPointer<glm::vec3>(ATTRIB_NORMAL, 3, GL_FLOAT);

    glGenBuffers(1, &_pointBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _pointBuffer);
    glGenVertexArrays(1, &_pointVao);
    glBindVertexArray(_pointVao);
    enableVertexAttribArrays(ATTRIB_POSITION, ATTRIB_COLOR, ATTRIB_SELECTED);
    vertexAttribPointer<&GPUPointData::position>(ATTRIB_POSITION, 3, GL_FLOAT);
    vertexAttribPointer<&GPUPointData::color>(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, true);
    vertexAttribIPointer<&GPUPointData::selected>(ATTRIB_SELECTED, 1, GL_INT);
    
    glGenBuffers(1, &_normalsBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _normalsBuffer);
    glGenVertexArrays(1, &_normalVao);
    glBindVertexArray(_normalVao);
    enableVertexAttribArrays(ATTRIB_POSITION, ATTRIB_NORMAL, ATTRIB_SELECTED);
    vertexAttribPointer<&GPUNormalData::position>(ATTRIB_POSITION, 3, GL_FLOAT);
    vertexAttribPointer<&GPUNormalData::normal>(ATTRIB_NORMAL, 3, GL_FLOAT);
    vertexAttribIPointer<&GPUNormalData::selected>(ATTRIB_SELECTED, 1, GL_INT);

    _uboIndex = 0;
    glGenBuffers(1, &_uboObject);

    glBindBuffer(GL_UNIFORM_BUFFER, _uboObject);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GPURenderData), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    auto set_ubo = [&_uboIndex = _uboIndex, &_uboObject = _uboObject](GLuint program) {
        GLuint index = glGetUniformBlockIndex(program, "RenderData");
        glUniformBlockBinding(program, index, _uboIndex);
        glUseProgram(program);
        glBindBufferBase(GL_UNIFORM_BUFFER, _uboIndex, _uboObject); 
    };

    set_ubo(_modelProgram.program);
    set_ubo(_simpleProgram.program);
    set_ubo(_normalProgram.program);
}

void MDLRenderer::resize(int available_width, int available_height)
{
    if (_width == available_width && _height == available_height)
        return;

    _width = available_width;
    _height = available_height;

    if (!_fbo)
    {
        glGenFramebuffers(1, &_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

        glGenTextures(1, &_fboColor);
        glBindTexture(GL_TEXTURE_2D, _fboColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               _fboColor,
                               0);

        glGenRenderbuffers(1, &_fboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, _fboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                  GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER,
                                  _fboDepth);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        if (status != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("bad fbo");
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, _fboColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, _fboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MDLRenderer::draw()
{
    ImGui::Image((ImTextureID) (ptrdiff_t) getRendererTexture(), { (float) _width, (float) _height }, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
}

QuadRect MDLRenderer::getQuadrantRect(QuadrantFocus quadrant)
{
    int w = _width;//width() * devicePixelRatio();
    int h = _height;//height() * devicePixelRatio();
        
    // calculate the top-left quadrant size, which is used as the basis
    int qw = (w * settings().horizontalSplit) - 1;
    int qh = (h * settings().verticalSplit) - 1;
        
    int line_width = 2 + (w & 1);
    int line_height = 2 + (h & 1);

    if (quadrant == QuadrantFocus::TopLeft)
        return { 0, 0, qw, qh };
    else if (quadrant == QuadrantFocus::Vertical)
        return { qw, 0, line_width, h };
    else if (quadrant == QuadrantFocus::Horizontal)
        return { 0, qh, w, line_height };
    else if (quadrant == QuadrantFocus::Center)
        return { qw, qh, line_width, line_height };

    // the rest of the quadrants need the other side
    // calculated.
    int oqw = w - (qw + line_width);
    int oqh = h - (qh + line_height);

    if (quadrant == QuadrantFocus::TopRight)
        return { qw + line_width, 0, oqw, qh };
    else if (quadrant == QuadrantFocus::BottomRight)
        return { qw + line_width, qh + line_height, oqw, oqh };
    else if (quadrant == QuadrantFocus::BottomLeft)
        return { 0, qh + line_height, qw, oqh };

    throw std::runtime_error("bad quadrant");
}

QuadrantFocus MDLRenderer::getQuadrantFocus(glm::ivec2 xy)
{
    constexpr QuadrantFocus focusOrder[] = {
        QuadrantFocus::Center,
        QuadrantFocus::Horizontal,
        QuadrantFocus::Vertical,
        QuadrantFocus::TopLeft,
        QuadrantFocus::TopRight,
        QuadrantFocus::BottomRight,
        QuadrantFocus::BottomLeft
    };
    QuadRect focusRects[] = {
        getQuadrantRect(QuadrantFocus::Center),
        getQuadrantRect(QuadrantFocus::Horizontal),
        getQuadrantRect(QuadrantFocus::Vertical),
        getQuadrantRect(QuadrantFocus::TopLeft),
        getQuadrantRect(QuadrantFocus::TopRight),
        getQuadrantRect(QuadrantFocus::BottomRight),
        getQuadrantRect(QuadrantFocus::BottomLeft)
    };

    size_t i = 0;

    for (auto &quadrant : focusRects)
    {
        if (xy.x >= quadrant.x && xy.y >= quadrant.y &&
            xy.x < quadrant.x + quadrant.w && xy.y < quadrant.y + quadrant.h)
            return focusOrder[i];

        i++;
    }

    return QuadrantFocus::None;
}

bool MDLRenderer::mousePressEvent(ImGuiMouseButton buttons, ImVec2 localPos)
{
    _dragging = true;
    _dragButtons = buttons;
    _dragPos = _downPos = { localPos.x, localPos.y }; //mapFromGlobal(QCursor::pos()) * devicePixelRatio();
    _dragWorldPos = mouseToWorld(_dragPos);
    _dragDelta = {};

    if (ui().editor3D().editorTool() == EditorTool::Select)
        return false;

    return _focusedQuadrant == QuadrantFocus::TopLeft || _focusedQuadrant == QuadrantFocus::TopRight ||
        _focusedQuadrant == QuadrantFocus::BottomLeft || _focusedQuadrant == QuadrantFocus::BottomRight;
}

void MDLRenderer::rectangleSelect(aabb2 rect)
{
    auto &mdl = model().model();
    auto mutator = model().mutator();

    if (rect.mins == rect.maxs)
    {
        constexpr float w = 5;
        constexpr float h = 5;

        rect = aabb2::fromMinsMaxs(
            rect.mins - glm::vec2(w / 2, h / 2),
            rect.maxs + glm::vec2(w, h)
        );
    }

    auto r = getQuadrantRect(_focusedQuadrant);
    rect.mins -= glm::vec2 { r.x, r.y };
    rect.maxs -= glm::vec2 { r.x, r.y };
    auto matrices = getQuadrantMatrices(_focusedQuadrant);

    if (ui().editor3D().editorSelectMode() == SelectMode::Vertex)
    {
        mutator.selectRectangleVertices3D(rect, [this, &mutator, &matrices, &r](size_t mesh, size_t index) {
            return worldToMouse(mutator.data->meshes[mesh].frames[mutator.data->selectedFrame].vertices[index].position(), matrices.projection, matrices.modelview, r, true);
        });
    }
    else
    {
        mutator.selectRectangleTriangles3D(rect, [this, &mutator, &matrices, &r](size_t mesh, size_t index) {
            return worldToMouse(mutator.data->meshes[mesh].frames[mutator.data->selectedFrame].vertices[index].position(), matrices.projection, matrices.modelview, r, true);
        });
    }
}

void MDLRenderer::mouseReleaseEvent(ImVec2 localPos)
{
    if (!_dragging)
        return;
    
    if (_dragging && (_focusedQuadrant == QuadrantFocus::TopLeft || _focusedQuadrant == QuadrantFocus::TopRight ||
        _focusedQuadrant == QuadrantFocus::BottomLeft || _focusedQuadrant == QuadrantFocus::BottomRight))
    {
        if (ui().editor3D().editorTool() == EditorTool::Select)
            rectangleSelect(aabb2::fromMinsMaxs(_downPos, _dragPos).normalize());
        else
        {
            glm::mat4 drag = getDragMatrix();

            if (!glm::all(glm::equal(drag, glm::identity<glm::mat4>())))
                model().mutator().apply3DMatrix(drag, ui().editor3D().editorSelectMode());
        }
    }

    _dragging = false;

    mouseMoveEvent(localPos);
    //MainWindow::instance().updateRenders();
}

void MDLRenderer::mouseMoveEvent(ImVec2 localPos)
{
    glm::ivec2 pos { localPos.x, localPos.y };//mapFromGlobal(QCursor::pos()) * devicePixelRatio();
    //auto world = mouseToWorld(pos);

    //MainWindow::instance().setCurrentWorldPosition(world);

    if (_dragging)
    {
        glm::ivec2 delta = { (int) -localPos.x, (int) -localPos.y };

        if (!delta.x && !delta.y)
            return;
        
        auto &io = ImGui::GetIO();

        _dragDelta += delta;
        
        float xDelta = delta.x / _2dZoom;
        float yDelta = delta.y / _2dZoom;

        if (_focusedQuadrant == QuadrantFocus::Horizontal ||
            _focusedQuadrant == QuadrantFocus::Vertical ||
            _focusedQuadrant == QuadrantFocus::Center)
        {
            bool adjust_vert = _focusedQuadrant == QuadrantFocus::Horizontal || _focusedQuadrant == QuadrantFocus::Center;
            bool adjust_horz = _focusedQuadrant == QuadrantFocus::Vertical || _focusedQuadrant == QuadrantFocus::Center;
                
            if (adjust_horz)
                settings().horizontalSplit = (float) pos.x / (_width/* * devicePixelRatio()*/);
            if (adjust_vert)
                settings().verticalSplit = (float) pos.y / (_height/* * devicePixelRatio()*/);
        }
        else if (ui().editor3D().editorTool() == EditorTool::Pan)
        {
            if (!_viewWeaponMode)
            {
                if (_focusedQuadrant == QuadrantFocus::TopRight)
                {
                    if (_dragButtons & (1 << ImGuiMouseButton_Right))
                        _camera.zoom(-delta.y, _camera.getOrbitMinZoom(), _camera.getOrbitMaxZoom());
                    else if (_dragButtons & (1 << ImGuiMouseButton_Left))
                        _camera.rotate(delta.y, delta.x, 0);
                }
                else if (_focusedQuadrant == QuadrantFocus::TopLeft ||
                            _focusedQuadrant == QuadrantFocus::BottomLeft ||
                            _focusedQuadrant == QuadrantFocus::BottomRight)
                {
                    if (_dragButtons & (1 << ImGuiMouseButton_Right))
                    {
                        _2dZoom += (delta.y * 0.01f) * _2dZoom;
                    }
                    else if (_dragButtons & (1 << ImGuiMouseButton_Left))
                    {
                        if (_focusedQuadrant == QuadrantFocus::TopLeft)
                            _2dOffset += glm::vec3(-xDelta, yDelta, 0);
                        else if (_focusedQuadrant == QuadrantFocus::BottomLeft)
                            _2dOffset += glm::vec3(-xDelta, 0, yDelta);
                        else if (_focusedQuadrant == QuadrantFocus::BottomRight)
                            _2dOffset += glm::vec3(0, -xDelta, yDelta);
                    }
                }
            }
        }
        
        //update();

        if (ui().editor3D().editorTool() == EditorTool::Select)
            _dragPos = pos;
        
        if (_focusedQuadrant == QuadrantFocus::Center)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        else if (_focusedQuadrant == QuadrantFocus::Horizontal)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        else if (_focusedQuadrant == QuadrantFocus::Vertical)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        else if (ui().editor3D().editorTool() == EditorTool::Pan)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        else
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

        return;
    }

    _focusedQuadrant = getQuadrantFocus(pos);
        
    int w = _width;//this->width() * devicePixelRatio();
    int h = _height;//this->height() * devicePixelRatio();
        
    int quadrant_w = (w / 2) - 1;
    int quadrant_h = (h / 2) - 1;
        
    int line_width = 2 + (w & 1);
    int line_height = 2 + (h & 1);
        
    bool within_x = pos.x >= quadrant_w && pos.x <= quadrant_w + line_width;
    bool within_y = pos.y >= quadrant_h && pos.y <= quadrant_h + line_height;

    // the separators always take priority
    if (_focusedQuadrant == QuadrantFocus::Center)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    else if (_focusedQuadrant == QuadrantFocus::Horizontal)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    else if (_focusedQuadrant == QuadrantFocus::Vertical)
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    else if (ui().editor3D().editorTool() == EditorTool::Pan)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    else
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
}

void MDLRenderer::mouseWheelEvent(int delta)
{
    if (_viewWeaponMode)
        return;

    delta *= 16.0f;

    if (_focusedQuadrant == QuadrantFocus::TopRight)
        _camera.zoom(-delta, _camera.getOrbitMinZoom(), _camera.getOrbitMaxZoom());
    else if (_focusedQuadrant == QuadrantFocus::TopLeft ||
                _focusedQuadrant == QuadrantFocus::BottomLeft ||
                _focusedQuadrant == QuadrantFocus::BottomRight)
    {
        _2dZoom += (delta * 0.01f) * _2dZoom;
    }
}

glm::mat4 MDLRenderer::getDragMatrix()
{
    glm::mat4 matrix = glm::identity<glm::mat4>();
    
    if (!_dragging || _focusedQuadrant == QuadrantFocus::None)
        return matrix;
        
    float xDelta = _dragDelta.x / _2dZoom;
    float yDelta = _dragDelta.y / _2dZoom;

    if (ui().editor3D().editorTool() == EditorTool::Move)
    {
        glm::vec3 translate;

        if (_focusedQuadrant == QuadrantFocus::TopLeft)
            translate = { -yDelta, -xDelta, 0 };
        else if (_focusedQuadrant == QuadrantFocus::BottomLeft)
            translate = { 0, -xDelta, yDelta };
        else if (_focusedQuadrant == QuadrantFocus::BottomRight)
            translate = { xDelta, 0, yDelta };
        
        if (!ui().editor3D().editorAxis().X)
            translate.x = 0;
        if (!ui().editor3D().editorAxis().Y)
            translate.z = 0;
        if (!ui().editor3D().editorAxis().Z)
            translate.y = 0;

        matrix = glm::translate(matrix, translate);
    }
    else if (ui().editor3D().editorTool() == EditorTool::Scale)
    {
        float s = 1.0f + (_dragDelta.y * 0.01f) / _2dZoom;
        matrix = glm::translate(matrix, _dragWorldPos);
        matrix = glm::scale(matrix, glm::vec3(
            ui().editor3D().editorAxis().X ? s : 1,
            ui().editor3D().editorAxis().Z ? s : 1,
            ui().editor3D().editorAxis().Y ? s : 1
        ));
        matrix = glm::translate(matrix, -_dragWorldPos);
    }
    else if (ui().editor3D().editorTool() == EditorTool::Rotate)
    {
        QuadRect rect = getQuadrantRect(_focusedQuadrant);
        float r = glm::radians(360.f * (_dragDelta.y / (float) rect.h));
        matrix = glm::translate(matrix, _dragWorldPos);
        if (_focusedQuadrant == QuadrantFocus::TopLeft)
            matrix = glm::rotate(matrix, r, glm::vec3(0, 0, -1));
        else if (_focusedQuadrant == QuadrantFocus::BottomLeft)
            matrix = glm::rotate(matrix, r, glm::vec3(1, 0, 0));
        else if (_focusedQuadrant == QuadrantFocus::BottomRight)
            matrix = glm::rotate(matrix, r, glm::vec3(0, 1, 0));
        matrix = glm::translate(matrix, -_dragWorldPos);
    }

    return matrix;
}

void MDLRenderer::focusLost()
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
    MDLRenderer::mouseReleaseEvent(&ev);
#endif
}

#if 0
void MDLRenderer::leaveEvent(QEvent *event)
{
    _focusedQuadrant = QuadrantFocus::None;
}
#endif

void MDLRenderer::scissorQuadrant(QuadRect rect)
{
    // flip Y to match GL orientation
    int ry = (_height/*(height() * devicePixelRatio())*/ - rect.y) - rect.h;
    glScissor(rect.x, ry, rect.w, rect.h);
}

void MDLRenderer::clearQuadrant(QuadRect rect, Color color)
{
    // flip Y to match GL orientation
    int ry = (_height/*(height() * devicePixelRatio())*/ - rect.y) - rect.h;

    glViewport(rect.x, ry, rect.w, rect.h);
    scissorQuadrant(rect);
    glClearColor(color[0] / 255.f, color[1] / 255.f, color[2] / 255.f, color[3] / 255.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MDLRenderer::drawModels(QuadrantFocus quadrant, bool is_2d)
{
    RenderParameters &params = (is_2d ? settings().renderParams2D : settings().renderParams3D);

    auto [ projection, modelview ] = getQuadrantMatrices(quadrant);

    glUseProgram(_simpleProgram.program);
    glUniformMatrix4fv(_simpleProgram.projectionUniformLocation, 1, false, glm::value_ptr(projection));
    glUniformMatrix4fv(_simpleProgram.modelviewUniformLocation, 1, false, glm::value_ptr(modelview));

    glDisable(GL_CULL_FACE);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindTexture(GL_TEXTURE_2D, _whiteTexture);

    if (params.showGrid)
    {
        glBindVertexArray(_gridVao);
        glBindBuffer(GL_ARRAY_BUFFER, _gridBuffer);
        const Color &gridColor = ui().GetColor(EditorColorId::Grid);
        glVertexAttribI1i(ATTRIB_SELECTED, 0);
        glVertexAttrib2f(ATTRIB_TEXCOORD, 1.0f, 1.0f);
        glVertexAttrib4f(ATTRIB_COLOR, gridColor.r / 255.f, gridColor.g / 255.f, gridColor.b / 255.f, gridColor.a / 255.f);
        glm::mat4 gridMatrix = glm::translate(modelview, { 0, 0, _gridZ });
        glUniformMatrix4fv(_simpleProgram.modelviewUniformLocation, 1, false, glm::value_ptr(gridMatrix));
        glDrawArrays(GL_LINES, 0, (GLsizei) _gridSize);
        glUniformMatrix4fv(_simpleProgram.modelviewUniformLocation, 1, false, glm::value_ptr(modelview));
    }
    
    if (params.showOrigin)
    {
        glDisable(GL_DEPTH_TEST);

        glBindVertexArray(_axisVao);
        glBindBuffer(GL_ARRAY_BUFFER, _axisBuffer);
        glVertexAttribI1i(ATTRIB_SELECTED, 0);
        glDrawArrays(GL_LINES, 0, 6);

        glEnable(GL_DEPTH_TEST);
    }

    glUseProgram(_modelProgram.program);
    glUniform1i(_modelProgram.is2DLocation, is_2d);
    auto face3DUnselected = ui().GetColor(EditorColorId::FaceUnselected3D).as_vec4();
    auto face3DSelected = ui().GetColor(EditorColorId::FaceSelected3D).as_vec4();
    glUniform4fv(_modelProgram.face3DLocation + 0, 1, &(face3DUnselected.x));
    glUniform4fv(_modelProgram.face3DLocation + 1, 1, &(face3DSelected.x));
    auto face2DUnselected = ui().GetColor(EditorColorId::FaceUnselected2D).as_vec4();
    auto face2DSelected = ui().GetColor(EditorColorId::FaceSelected2D).as_vec4();
    glUniform4fv(_modelProgram.face2DLocation + 0, 1, &(face2DUnselected.x));
    glUniform4fv(_modelProgram.face2DLocation + 1, 1, &(face2DSelected.x));
    auto line3DUnselected = ui().GetColor(EditorColorId::FaceLineUnselected3D).as_vec4();
    auto line3DSelected = ui().GetColor(EditorColorId::FaceLineSelected3D).as_vec4();
    glUniform4fv(_modelProgram.line3DLocation + 0, 1, &(line3DUnselected.x));
    glUniform4fv(_modelProgram.line3DLocation + 1, 1, &(line3DSelected.x));
    auto line2DUnselected = ui().GetColor(EditorColorId::FaceLineUnselected2D).as_vec4();
    auto line2DSelected = ui().GetColor(EditorColorId::FaceLineSelected2D).as_vec4();
    glUniform4fv(_modelProgram.line2DLocation + 0, 1, &(line2DUnselected.x));
    glUniform4fv(_modelProgram.line2DLocation + 1, 1, &(line2DSelected.x));
    glUniformMatrix4fv(_modelProgram.projectionUniformLocation, 1, false, glm::value_ptr(projection));
    glUniformMatrix4fv(_modelProgram.modelviewUniformLocation, 1, false, glm::value_ptr(modelview));

    // model
    glDisable(GL_BLEND);
    glBindVertexArray(_vao);
    glVertexAttrib4f(ATTRIB_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);

    if (params.smoothNormals)
        glBindBuffer(GL_ARRAY_BUFFER, _smoothNormalBuffer);
    else
        glBindBuffer(GL_ARRAY_BUFFER, _flatNormalBuffer);

    vertexAttribPointer<glm::vec3>(ATTRIB_NORMAL, 3, GL_FLOAT);

    if (!params.drawBackfaces)
        glEnable(GL_CULL_FACE);

    if (params.mode == RenderMode::Wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUniform1i(_modelProgram.shadedUniformLocation, false);
    }
    else
        glUniform1i(_modelProgram.shadedUniformLocation, params.shaded);

    GLint offset = 0;
    
    if (params.filtered)
        glBindSampler(0, _filteredSampler);

    glUniform1i(_modelProgram.isLineLocation, params.mode == RenderMode::Wireframe);

    for (auto &mesh : model().model().meshes)
    {
        GLsizei count = mesh.triangles.size() * 3;

        if (params.mode == RenderMode::Textured)
        {
            auto skin = mesh.assigned_skin;
            
            if (!skin)
                skin = model().model().selectedSkin;

            if (skin)
                model().model().skins[skin.value()].handle->Bind();
        }
        
        glDrawArrays(GL_TRIANGLES, offset, count);
        offset += count;
    }

    if (params.mode != RenderMode::Wireframe && params.showOverlay)
    {
        offset = 0;
        glUniform1i(_modelProgram.isLineLocation, 1);
        glBindTexture(GL_TEXTURE_2D, _whiteTexture);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glPolygonOffset(-1.0f, 0.0f);
        glEnable(GL_POLYGON_OFFSET_LINE);

        for (auto &mesh : model().model().meshes)
        {
            GLsizei count = mesh.triangles.size() * 3;
            glDrawArrays(GL_TRIANGLES, offset, count);
            offset += count;
        }

        glDisable(GL_POLYGON_OFFSET_LINE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (params.filtered)
        glBindSampler(0, _nearestSampler);

    glEnable(GL_BLEND);

    glVertexAttrib4f(ATTRIB_COLOR, 1.0f, 1.0f, 1.0f, 1.0f);

    if (params.showTicks || params.showNormals)
    {
        // this is super ugly but it works ok enough for now.
        glDepthFunc(GL_LEQUAL);
	
        Matrix4 depthProj = projection;
        if (!depthProj[3][3])
        {
            constexpr float n = 0.1f;
            constexpr float f = 1024;
            constexpr float delta = 0.25f;
            constexpr float pz = 8.5f;
	        constexpr float epsilon = -2.0f * f * n * delta / ((f + n) * pz * (pz + delta));

    	    depthProj[3][3] = -epsilon;
        }
        else
            glm::scale(depthProj, glm::vec3(1.0f, 1.0f, 0.98f));
        
        if (params.showNormals)
        {
            glUseProgram(_normalProgram.program);
            glUniformMatrix4fv(_simpleProgram.projectionUniformLocation, 1, false, glm::value_ptr(depthProj));
            glUniformMatrix4fv(_simpleProgram.modelviewUniformLocation, 1, false, glm::value_ptr(modelview));
        }

        glUseProgram(_simpleProgram.program);
        glUniformMatrix4fv(_simpleProgram.projectionUniformLocation, 1, false, glm::value_ptr(depthProj));

        glBindTexture(GL_TEXTURE_2D, _whiteTexture);
    }
    
    if (params.showTicks)
    {
        // points
        glPointSize(3.0f);
        glDisable(GL_DEPTH_TEST);
    
        glBindVertexArray(_pointVao);
        glBindBuffer(GL_ARRAY_BUFFER, _pointBuffer);
        glDrawArrays(GL_POINTS, 0, (GLsizei) _pointData.size());

        glEnable(GL_DEPTH_TEST);
    }

    if (params.showNormals)
    {
        glUseProgram(_normalProgram.program);
        glBindVertexArray(_normalVao);
        glBindBuffer(GL_ARRAY_BUFFER, _normalsBuffer);
        glDrawArrays(GL_LINES, 0, (GLsizei) _normalsData.size());
    }
    
    if (params.showTicks || params.showNormals)
    {
        glDepthFunc(GL_LESS);
    }
}

QuadrantMatrices MDLRenderer::getQuadrantMatrices(QuadrantFocus quadrant)
{
    Matrix4 projection, modelview;
    QuadRect rect = getQuadrantRect(quadrant);

    if (quadrant == QuadrantFocus::TopRight)
    {
        if (!_viewWeaponMode)
        {
            _camera.perspective(fov(), (float) rect.w / rect.h, 0.1f, 1024.f);
            projection = _camera.getProjectionMatrix();

            modelview = _camera.getViewMatrix();
            modelview = glm::rotate(modelview, glm::radians(-90.f), { 1.f, 0.f, 0.f });
            modelview = glm::translate(modelview, { -_2dOffset.y, _2dOffset.x, _2dOffset.z });
        }
        else
        {
            _viewModelCamera.perspective(fov(), (float) rect.w / rect.h, 0.01f, 128.f, true);
            projection = _viewModelCamera.getProjectionMatrix();

            modelview = _viewModelCamera.getViewMatrix();
            modelview = glm::rotate(modelview, glm::radians(-90.f), { 1.f, 0.f, 0.f });
        }
    }
    else
    {
        float hw = (rect.w / 2.0f);
        float hh = (rect.h / 2.0f);
        projection = glm::ortho(-hw, hw, -hh, hh, -8192.f, 8192.f);

        modelview = glm::identity<glm::mat4>();
        modelview = glm::scale(modelview, glm::vec3 { _2dZoom });

        if (quadrant == QuadrantFocus::TopLeft)
        {
            modelview = glm::translate(modelview, glm::vec3 { _2dOffset.x, _2dOffset.y, 0.f });
            modelview = glm::rotate(modelview, glm::radians(-90.f), { 0.0f, 0.0f, 1.0f });
        }
        else if (quadrant == QuadrantFocus::BottomLeft)
        {
            modelview = glm::translate(modelview, glm::vec3 { _2dOffset.x, _2dOffset.z, 0.f });
            modelview = glm::rotate(modelview, glm::radians(-90.f), { 0.0f, 0.0f, 1.0f });
            modelview = glm::rotate(modelview, glm::radians(-90.f), { 0.0f, 1.0f, 0.0f });
        }
        else if (quadrant == QuadrantFocus::BottomRight)
        {
            modelview = glm::translate(modelview, glm::vec3 { _2dOffset.y, _2dOffset.z, 0.f });
            modelview = glm::rotate(modelview, glm::radians(-90.f), { 0.0f, 0.0f, 1.0f });
            modelview = glm::rotate(modelview, glm::radians(-90.f), { 0.0f, 1.0f, 0.0f });
            modelview = glm::rotate(modelview, glm::radians(-90.f), { 0.0f, 0.0f, 1.0f });
        }
    }

    return { projection, modelview };
}

void MDLRenderer::draw2D(Orientation2D orientation, QuadrantFocus quadrant)
{
    clearQuadrant(getQuadrantRect(quadrant), { 102, 102, 102, 255 });
    drawModels(quadrant, true);
}

void MDLRenderer::draw3D(QuadrantFocus quadrant)
{
    clearQuadrant(getQuadrantRect(quadrant), { 102, 102, 102, 255 });
    drawModels(quadrant, false);
}

glm::vec3 MDLRenderer::mouseToWorld(glm::ivec2 pos)
{
    if (_focusedQuadrant == QuadrantFocus::None ||
        _focusedQuadrant == QuadrantFocus::Vertical ||
        _focusedQuadrant == QuadrantFocus::Horizontal ||
        _focusedQuadrant == QuadrantFocus::Center)
        return {};

    auto r = getQuadrantRect(_focusedQuadrant);
    auto [ projection, modelview ] = getQuadrantMatrices(_focusedQuadrant);

    // put pos into local quadrant space
    pos.x = pos.x - r.x;
    pos.y = pos.y - r.y;
    // flip Y to match GL orientation
    pos.y = r.h - pos.y;

    glm::vec3 position = glm::vec3(pos, 0.0f);

    if (_focusedQuadrant == QuadrantFocus::TopRight)
        position.z = 1.0f;
    else
        position.z = 0.5f;

    return glm::unProject(position, modelview, projection, glm::ivec4{ 0, 0, r.w, r.h });
}

glm::vec2 MDLRenderer::worldToMouse(const glm::vec3 &pos, const Matrix4 &projection, const Matrix4 &modelview, const QuadRect &viewport, bool local)
{
    if (_focusedQuadrant == QuadrantFocus::None ||
        _focusedQuadrant == QuadrantFocus::Vertical ||
        _focusedQuadrant == QuadrantFocus::Horizontal ||
        _focusedQuadrant == QuadrantFocus::Center)
        return {};

    auto pt = glm::project(pos, modelview, projection, glm::ivec4{ 0, 0, viewport.w, viewport.h });
    
    // flip Y to match GL orientation
    pt.y = viewport.h - pt.y;

    if (!local)
    {
        // put pos into local quadrant space
        pt.x = pt.x + viewport.x;
        pt.y = pt.y + viewport.y;
    }

    return pt;
}

void MDLRenderer::updateTextures()
{
    for (auto &skin : model().mutator().data->skins)
    {
        if (!skin.handle)
            skin.handle = std::make_unique<MDLSkinDataHandle>(skin);

        static_cast<MDLSkinDataHandle *>(skin.handle.get())->Update(skin);
    }
}

void MDLRenderer::paint()
{
    glBindBuffer(GL_UNIFORM_BUFFER, _uboObject);
    _uboData.flags = 0;

    if (ui().editor3D().editorSelectMode() == SelectMode::Face)
        _uboData.flags |= GPURenderData::FLAG_FACE_MODE;
    
    if (auto matrix3D = getDragMatrix(); !glm::all(glm::equal(matrix3D, glm::identity<glm::mat4>())))
    {
        _uboData.flags |= GPURenderData::FLAG_DRAG_SELECTED;
        _uboData.drag3DMatrix = matrix3D;
    }

    if (auto matrixUV = ui().editorUV().renderer().getDragMatrix(); !glm::all(glm::equal(matrixUV, glm::identity<glm::mat4>())))
    {
        _uboData.flags |= GPURenderData::FLAG_UV_SELECTED;
        _uboData.drag3DMatrix = matrixUV;
    }
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(_uboData), &_uboData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);  

    glBindSampler(0, _nearestSampler);

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    this->rebuildBuffer();

#ifdef RENDERDOC_SUPPORT
    if (_doRenderDoc && rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);
#endif

    clearQuadrant({ 0, 0, static_cast<int>(/*width() * devicePixelRatio()*/_width), static_cast<int>(/*height() * devicePixelRatio()*/_height) }, { 0, 0, 0, 255 });
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    draw2D(Orientation2D::XY, QuadrantFocus::TopLeft);
    draw2D(Orientation2D::ZY, QuadrantFocus::BottomLeft);
    draw2D(Orientation2D::XZ, QuadrantFocus::BottomRight);

    draw3D(QuadrantFocus::TopRight);

    if (_dragging && ui().editor3D().editorTool() == EditorTool::Select)
    {
        QuadRect clamp = getQuadrantRect(_focusedQuadrant);

        aabb<2, int> quadrantBounds = aabb<2, int>::fromMinsMaxs({ clamp.x, clamp.y }, { clamp.x + clamp.w, clamp.y + clamp.h });
        aabb<2, int> selectBounds = aabb<2, int>::fromMinsMaxs(_downPos, _dragPos).normalize();
        
        if (selectBounds.mins.x < quadrantBounds.mins.x)
            selectBounds.mins.x = quadrantBounds.mins.x;
        if (selectBounds.mins.y < quadrantBounds.mins.y)
            selectBounds.mins.y = quadrantBounds.mins.y;
        if (selectBounds.maxs.x > quadrantBounds.maxs.x)
            selectBounds.maxs.x = quadrantBounds.maxs.x;
        if (selectBounds.maxs.y > quadrantBounds.maxs.y)
            selectBounds.maxs.y = quadrantBounds.maxs.y;

        const Color &selectBox = ui().GetColor(EditorColorId::SelectBox);
        
        int w = selectBounds.maxs.x - selectBounds.mins.x;
        int h = selectBounds.maxs.y - selectBounds.mins.y;

        clearQuadrant({
            selectBounds.mins.x, selectBounds.mins.y,
            w, 1
        }, selectBox);

        clearQuadrant({
            selectBounds.mins.x, selectBounds.maxs.y,
            w, 1
        }, selectBox);

        clearQuadrant({
            selectBounds.mins.x, selectBounds.mins.y,
            1, h
        }, selectBox);

        clearQuadrant({
            selectBounds.maxs.x, selectBounds.mins.y,
            1, h
        }, selectBox);
    }
        
#ifdef RENDERDOC_SUPPORT
    if (_doRenderDoc && rdoc_api) rdoc_api->EndFrameCapture(NULL, NULL);

    _doRenderDoc = false;
#endif

    // run animations
    auto &anim = ui().editor3D().animation();

    if (anim.active)
    {
        anim.time += ImGui::GetIO().DeltaTime;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindSampler(0, 0);
}

GLuint MDLRenderer::createShader(GLenum type, const char *source)
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

GLuint MDLRenderer::createProgram(GLuint vertexShader, GLuint fragmentShader)
{
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
    
    glBindAttribLocation(program, ATTRIB_POSITION, "i_position");
    glBindAttribLocation(program, ATTRIB_TEXCOORD, "i_texcoord");
    glBindAttribLocation(program, ATTRIB_COLOR, "i_color");
    glBindAttribLocation(program, ATTRIB_NORMAL, "i_normal");
    glBindAttribLocation(program, ATTRIB_SELECTED, "i_selected");
    glBindAttribLocation(program, ATTRIB_SELECTED_VERTEX, "i_selected_vertex");

	glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

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

template<typename T>
static void uploadToBuffer(GLuint buffer, bool full_upload, const std::vector<T> &data)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    if (full_upload)
        glBufferData(GL_ARRAY_BUFFER, sizeof(T) * data.size(), data.data(), GL_DYNAMIC_DRAW);
    else
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(T) * data.size(), data.data());
}

void MDLRenderer::rebuildBuffer()
{
    auto &anim = ui().editor3D().animation();

    if (!_bufferDirty && !anim.active)
        return;

    auto &mdl = model().model();

    size_t num_tri_verts = 0;

    for (auto &mesh : mdl.meshes)
        num_tri_verts += mesh.triangles.size() * 3;

    bool full_tri_upload = _bufferData.size() < num_tri_verts;
    _bufferData.resize(num_tri_verts);
    _smoothNormalData.resize(num_tri_verts);
    _flatNormalData.resize(num_tri_verts);

    size_t num_loose_verts = 0;

    for (auto &mesh : mdl.meshes)
        num_loose_verts += mesh.vertices.size();
    
    bool full_verts_upload = _pointData.size() < num_loose_verts;
    _pointData.resize(num_loose_verts);
    _normalsData.resize(num_loose_verts * 2);

    size_t n = 0, l = 0;
    int cur_frame = mdl.selectedFrame, next_frame = mdl.selectedFrame;
    float frac = 0.0f;

    if (anim.active)
    {
        float frame_time = anim.time * anim.fps;
        int frame_offset = (int) frame_time;

        if (anim.interpolate)
            frac = frame_time - frame_offset;
        
        const int &start = anim.from;
        const int &end = anim.to;

        // TODO: support backwards
        if (end > start)
        {
            cur_frame = start + (frame_offset % (end - start));
            next_frame = start + ((frame_offset + 1) % (end - start));
        }

        sys().WantsRedraw();
    }

    // TODO moved to cached state
    static std::unordered_set<size_t> selectedVerticesFromTriangles;
    
    for (auto &mesh : mdl.meshes)
    {
        selectedVerticesFromTriangles.clear();

        for (auto &tri : mesh.triangles)
            if (tri.selectedFace)
                selectedVerticesFromTriangles.insert({ tri.vertices[0], tri.vertices[1], tri.vertices[2] });

        for (auto &tri : mesh.triangles)
        {
            auto &from = mesh.frames[cur_frame];
            auto &to = mesh.frames[next_frame];
        
            auto &gv0 = mesh.vertices[tri.vertices[0]];
            auto &gv1 = mesh.vertices[tri.vertices[1]];
            auto &gv2 = mesh.vertices[tri.vertices[2]];

            auto &cv0 = from.vertices[tri.vertices[0]];
            auto &cv1 = from.vertices[tri.vertices[1]];
            auto &cv2 = from.vertices[tri.vertices[2]];

            auto &nv0 = to.vertices[tri.vertices[0]];
            auto &nv1 = to.vertices[tri.vertices[1]];
            auto &nv2 = to.vertices[tri.vertices[2]];
        
            std::array<MeshFrameVertex, 3> verts;
            
            verts[0] = MeshFrameVertex::lerp(cv0.vertex(), nv0.vertex(), frac);
            verts[1] = MeshFrameVertex::lerp(cv1.vertex(), nv1.vertex(), frac);
            verts[2] = MeshFrameVertex::lerp(cv2.vertex(), nv2.vertex(), frac);
            
            auto &st0 = mesh.texcoords[tri.texcoords[0]];
            auto &st1 = mesh.texcoords[tri.texcoords[1]];
            auto &st2 = mesh.texcoords[tri.texcoords[2]];

            {
                auto &ov0 = _bufferData[n + 0];
                auto &ov1 = _bufferData[n + 1];
                auto &ov2 = _bufferData[n + 2];
            
                ov0.position = verts[0].position;
                ov1.position = verts[1].position;
                ov2.position = verts[2].position;
            
                auto &nv0 = _smoothNormalData[n + 0];
                auto &nv1 = _smoothNormalData[n + 1];
                auto &nv2 = _smoothNormalData[n + 2];

                nv0 = verts[0].normal;
                nv1 = verts[1].normal;
                nv2 = verts[2].normal;
            
                _flatNormalData[n + 0] =
                _flatNormalData[n + 1] =
                _flatNormalData[n + 2] = (nv0 + nv1 + nv2) / 3.0f;

                ov0.texcoord = st0.pos;
                ov1.texcoord = st1.pos;
                ov2.texcoord = st2.pos;

                if (ui().editor3D().editorSelectMode() == SelectMode::Face)
                {
                    ov0.selected = ov1.selected = ov2.selected = tri.selectedFace;

                    ov0.selectedVertex = selectedVerticesFromTriangles.contains(tri.vertices[0]);
                    ov1.selectedVertex = selectedVerticesFromTriangles.contains(tri.vertices[1]);
                    ov2.selectedVertex = selectedVerticesFromTriangles.contains(tri.vertices[2]);
                }
                else
                {
                    ov0.selected = ov0.selectedVertex;
                    ov1.selected = ov1.selectedVertex;
                    ov2.selected = ov2.selectedVertex;

                    ov0.selectedVertex = mesh.vertices[tri.vertices[0]].selected;
                    ov1.selectedVertex = mesh.vertices[tri.vertices[1]].selected;
                    ov2.selectedVertex = mesh.vertices[tri.vertices[2]].selected;
                }
            }

            n += 3;
        }

        for (size_t i = 0; i < mesh.vertices.size(); i++)
        {
            auto &from = mesh.frames[cur_frame];
            auto &to = mesh.frames[next_frame];
        
            auto &cv = from.vertices[i];

            if (cv.is_tag())
                continue;

            auto &nv = to.vertices[i];
            auto &gv = mesh.vertices[i];
        
            MeshFrameVertex vert = MeshFrameVertex::lerp(cv.vertex(), nv.vertex(), frac);

            {
                auto &ov = _pointData[l];
            
                ov.position = vert.position;

                if (ui().editor3D().editorSelectMode() == SelectMode::Vertex)
                    ov.color = ui().GetColor(gv.selected ? EditorColorId::VertexTickSelected3D : EditorColorId::VertexTickUnselected3D);
                else
                    ov.color = ui().GetColor(EditorColorId::VertexTickUnselected3D);
                
                if (ui().editor3D().editorSelectMode() == SelectMode::Face)
                    ov.selected = selectedVerticesFromTriangles.contains(i);
                else
                    ov.selected = mesh.vertices[i].selected;
            }

            {
                auto &ov0 = _normalsData[(l * 2) + 0];
                auto &ov1 = _normalsData[(l * 2) + 1];
            
                ov0.position = vert.position;
                ov1.position = vert.position + (vert.normal * 4.0f);
                
                ov0.normal =
                ov1.normal = vert.normal;
                /* {
                    (uint8_t) (((vert.normal.x * 0.5f) + 0.5f) * 255),
                    (uint8_t) (((vert.normal.y * 0.5f) + 0.5f) * 255),
                    (uint8_t) (((vert.normal.z * 0.5f) + 0.5f) * 255),
                    (uint8_t) 255 };
                */

                if (ui().editor3D().editorSelectMode() == SelectMode::Face)
                    ov0.selected = ov1.selected = selectedVerticesFromTriangles.contains(i);
                else
                    ov0.selected = ov1.selected = mesh.vertices[i].selected;
            }

            l++;
        }
    }

    uploadToBuffer(_buffer, full_tri_upload, _bufferData);
    uploadToBuffer(_smoothNormalBuffer, full_tri_upload, _smoothNormalData);
    uploadToBuffer(_flatNormalBuffer, full_tri_upload, _flatNormalData);

    uploadToBuffer(_pointBuffer, full_verts_upload, _pointData);
    uploadToBuffer(_normalsBuffer, full_verts_upload, _normalsData);

    _bufferDirty = false;
}

void MDLRenderer::selectedSkinChanged()
{
#if 0
    makeCurrent();
    MainWindow::instance().updateRenders();
#endif
}

void MDLRenderer::modelLoaded()
{
    selectedSkinChanged();
    _2dOffset = model().model().boundsOfFrame(0).centroid();
    // FIXME: reorganize so that this is applied in model space.
    _2dOffset = { -_2dOffset.y, _2dOffset.x, -_2dOffset.z };
    _2dZoom = 1.0f;

    _gridZ = 0;

    for (auto &mesh : model().model().meshes)
        for (auto &v : mesh.frames[0].vertices)
            _gridZ = std::min(_gridZ, v.position().z);

    _bufferDirty = true;
}

void MDLRenderer::captureRenderDoc(bool)
{
#ifdef RENDERDOC_SUPPORT
    if (!rdoc_api)
    {
        if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
        {
            pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
            RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
        }
    }

    this->_doRenderDoc = true;
    this->update();
#endif
}

GLuint MDLRenderer::getRendererTexture()
{
    return _fboColor;
}

void MDLRenderer::colorsChanged()
{
    glBindBuffer(GL_ARRAY_BUFFER, _axisBuffer);
    generateAxis();

    glBindBuffer(GL_ARRAY_BUFFER, _gridBuffer);
    generateGrid(8.0f, 8);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

int &MDLRenderer::fov()
{
    return _viewWeaponMode ? settings().weaponFov : settings().viewerFov;
}