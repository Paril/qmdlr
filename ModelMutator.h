#pragma once

#include <functional>

#include "Types.h"
#include "ModelData.h"
#include "Editor3D.h"

class ModelMutator
{
public:
    ModelData   *data;

    constexpr ModelMutator(ModelData *data) :
        data(data)
    {
    }

    constexpr bool isValid() const { return !!data; }

    constexpr explicit operator bool() const { return isValid(); }

#pragma region(Frame Operations)
    void setSelectedFrame(int32_t frame);

    void setSelectedFrameName(std::string &str);
#pragma endregion

#pragma region(Skins)
    void setSelectedSkin(std::optional<int32_t> skin);
    void setSelectedSkinName(std::string &str);
    void setNextSkin();
    void setPreviousSkin();

    void addSkin();
    void deleteSkin();
    void resizeSkin(int width, int height, bool resizeUVs, bool resizeImage);
    void moveSkin(int target, bool after);

    // `image` is stolen by this function.
    void importSkin(Image &image);
#pragma endregion

#pragma region(Select UV Vertices)
    void selectRectangleVerticesUV(const aabb2 &rect);
    void selectAllVerticesUV();
    void selectNoneVerticesUV();
    void selectInverseVerticesUV();
    void selectTouchingVerticesUV();
    void selectConnectedVerticesUV();
#pragma endregion
    
#pragma region(Select 3D Vertices)
    void selectRectangleVertices3D(const aabb2 &rect, const std::function<glm::vec2(size_t mesh, size_t coord_index)> &transformer);
    void selectAllVertices3D();
    void selectNoneVertices3D();
    void selectInverseVertices3D();
    void selectTouchingVertices3D();
    void selectConnectedVertices3D();
#pragma endregion
    
#pragma region(Select UV Triangles)
    void selectRectangleTrianglesUV(const aabb2 &rect);
    void selectAllTrianglesUV();
    void selectNoneTrianglesUV();
    void selectInverseTrianglesUV();
    void selectTouchingTrianglesUV();
    void selectConnectedTrianglesUV();
#pragma endregion
    
#pragma region(Select 3D Triangles)
    void selectRectangleTriangles3D(const aabb2 &rect, const std::function<glm::vec2(size_t mesh, size_t vertex)> &transformer);
    void selectAllTriangles3D();
    void selectNoneTriangles3D();
    void selectInverseTriangles3D();
    void selectTouchingTriangles3D();
    void selectConnectedTriangles3D();
#pragma endregion
    
#pragma region(Sync)
    void syncSelectionUV();
    void syncSelection3D();
#pragma endregion
    
#pragma region(UV Matrix Apply)
    void applyUVMatrix(const glm::mat4 &matrix, SelectMode mode);
#pragma endregion
    
#pragma region(3D Matrix Apply)
    void apply3DMatrix(const glm::mat4 &matrix, SelectMode mode);
#pragma endregion

    // return a fixed set of texture coordinate indices that are
    // currently considered "selected" - that is to say, they will
    // be adjusted if an operation occurs.
    // FIXME in the future this should be cached state
    const std::unordered_set<size_t> &getSelectedTextureCoordinates(const ModelMesh &mesh, SelectMode mode);
    const std::unordered_set<size_t> &getSelectedVertices(const ModelMesh &mesh, SelectMode mode);
};