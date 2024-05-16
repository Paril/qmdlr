#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <optional>
#include <vector>
#include <unordered_set>
#include <imgui.h>
#include <iostream>
#include <variant>

#include "Stream.h"
#include "Math.h"
#include "Images.h"

struct ModelTriangle
{
	std::array<uint32_t, 3>	vertices;
	std::array<uint32_t, 3>	texcoords;
    bool                    selectedFace = false;
    bool                    selectedUV = false;
    
    auto stream_data()
    {
        return std::tie(vertices, texcoords, selectedFace, selectedUV);
    }
};

struct Q1GroupData
{
    int32_t     group;
    float       interval;
    
    auto stream_data()
    {
        return std::tie(group, interval);
    }
};

struct ModelFrame
{
    std::string					  name;
    std::optional<Q1GroupData>    q1_data;
    
    auto stream_data()
    {
        return std::tie(name, q1_data);
    }
};

// a handle that is meant to be implemented
// by the renderer to handle maintaining
// renderer-specific skin stuff
struct RendererSkinHandle
{
    virtual ~RendererSkinHandle() { }

    // something has changed in the image, re-upload it
    virtual void MarkDirty() = 0;

    // potentially update texture
    virtual void Update(struct ModelSkin &skin) = 0;

    // bind
    virtual void Bind() const = 0;

    // imgui handle
    virtual ImTextureID GetTextureHandle() const = 0;

protected:
    RendererSkinHandle() = default;
};

struct ModelSkin
{
    std::string                    name;
    int32_t                        width, height; // nb: these are separate because in theory you can use a different
                                                  // skin size in the MD2 than the PCX files, or an image might not
                                                  // currently exist but we still need a valid size for it.
    Image                          image;

    std::optional<Q1GroupData>     q1_data;

    // private to renderer
    std::unique_ptr<RendererSkinHandle> handle;
    
    auto stream_data()
    {
        return std::tie(name, width, height, image, q1_data);
    }
};

struct ModelVertex
{
    bool selected = false;

    auto stream_data()
    {
        return std::tie(selected);
    }
};

struct ModelTexCoord
{
    glm::vec2 pos;
    bool      selected = false;
    
    auto stream_data()
    {
        return std::tie(pos, selected);
    }
};

struct MeshFrameVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    
    auto stream_data()
    {
        return std::tie(position, normal);
    }

    MeshFrameVertex transform(const glm::mat4 &m, const glm::mat3 &n)
    {
        return { m * glm::vec4(position, 1.0f), glm::normalize(n * normal) };
    }

    static MeshFrameVertex lerp(const MeshFrameVertex &a, const MeshFrameVertex &b, const float &frac)
    {
        if (frac == 0.0f)
        {
            return a;
        }
        else if (frac == 1.0f)
        {
            return b;
        }
        else
        {
            return { glm::mix(a.position, b.position, frac), glm::mix(a.normal, b.normal, frac) };
        }
    }
};

#include <glm/gtc/quaternion.hpp>

struct MeshFrameTag
{
    glm::vec3 position;
    glm::quat orientation;   
    
    auto stream_data()
    {
        return std::tie(position, orientation);
    }

    MeshFrameTag transform(const glm::mat4 &m, const glm::mat3 &n)
    {
        return { m * glm::vec4(position, 1.0f), glm::quat_cast(n * glm::mat3_cast(orientation)) };
    }

    static MeshFrameTag lerp(const MeshFrameTag &a, const MeshFrameTag &b, const float &frac)
    {
        if (frac == 0.0f)
        {
            return a;
        }
        else if (frac == 1.0f)
        {
            return b;
        }
        else
        {
            return { glm::mix(a.position, b.position, frac), glm::slerp(a.orientation, b.orientation, frac) };
        }
    }
};

struct MeshFrameVertTag
{
    using type = std::variant<MeshFrameVertex, MeshFrameTag>;
    type data;
    
    bool is_vertex() const { return std::holds_alternative<MeshFrameVertex>(data); }
    bool is_tag() const { return std::holds_alternative<MeshFrameTag>(data); }
    
    MeshFrameVertex &vertex() { return std::get<MeshFrameVertex>(data); }
    MeshFrameTag &tag() { return std::get<MeshFrameTag>(data); }
    
    const MeshFrameVertex &vertex() const { return std::get<MeshFrameVertex>(data); }
    const MeshFrameTag &tag() const { return std::get<MeshFrameTag>(data); }

    const glm::vec3 &position() const
    {
        if (is_vertex())
            return vertex().position;
        else
            return tag().position;
    }

    glm::vec3 &position()
    {
        if (is_vertex())
            return vertex().position;
        else
            return tag().position;
    }

    MeshFrameVertTag transform(const glm::mat4 &m, const glm::mat3 &n)
    {
        if (is_vertex())
            return { vertex().transform(m, n) };
        else
            return { tag().transform(m, n) };
    }

    void stream_write(std::ostream &s) const
    {
        s <= is_vertex();

        if (is_vertex())
            s <= vertex();
        else
            s <= tag();
    }

    void stream_read(std::istream &s)
    {
        bool is_vertex;
        s >= is_vertex;

        if (is_vertex)
        {
            MeshFrameVertex v;
            s >= v;
            data = v;
        }
        else
        {
            MeshFrameTag t;
            s >= t;
            data = t;
        }
    }

    static MeshFrameVertTag lerp(const MeshFrameVertTag &a, const MeshFrameVertTag &b, const float &frac)
    {
        if (a.is_vertex())
            return { MeshFrameVertex::lerp(a.vertex(), b.vertex(), frac) };
        else
            return { MeshFrameTag::lerp(a.tag(), b.tag(), frac) };
    }
};

struct MeshFrame
{
    std::vector<MeshFrameVertTag> vertices;

    inline aabb3 bounds() const
    {
        aabb3 bounds;

        for (auto &vert : vertices)
            if (vert.is_vertex())
                bounds.add(vert.vertex().position);

        if (bounds.empty())
            return aabb3(0);

        return bounds;
    }
    
    auto stream_data()
    {
        return std::tie(vertices);
    }
};

struct ModelMesh
{
    // these can all be empty for a valid model
    std::vector<ModelTexCoord>  texcoords;
    std::vector<ModelTriangle>  triangles;
    std::vector<ModelVertex>    vertices;
    std::vector<MeshFrame>      frames;
    // assigned skin for this model; if set, it will
    // always use this texture and not the selected skin.
    std::optional<int32_t>      assigned_skin = std::nullopt;
    std::string				    name;
    
    auto stream_data()
    {
        return std::tie(texcoords, triangles, vertices, frames, assigned_skin, name);
    }
};

// model data contains all of the data 'saved' with
// a model. It can not be mutated directly.
struct ModelData
{
private:
    // no copy constructor, to prevent errors
    ModelData(const ModelData &) = delete;

public:
    ModelData(ModelData &&) = default;
    ModelData() = default;

    // model data
    // nb: a model will always have at least one frame.
    std::vector<ModelFrame>     frames;
    // nb: a model will always have at least one mesh.
    std::vector<ModelMesh>      meshes;
    std::vector<ModelSkin>      skins;

    // state data
    // selected animation frame
    int32_t                 selectedFrame = 0;
    // an unselected skin is valid
    std::optional<int32_t>  selectedSkin = std::nullopt;
    // if true, each mesh is assigned a matching skin
    // instead of all meshes using the same current skin.
    bool                    skinPerObject = false;

    // an unselected mesh means all meshes
    // are being used for operations
    std::optional<int32_t>  selectedMesh = std::nullopt;

    constexpr ModelSkin *getSelectedSkin()
    {
        if (!selectedSkin.has_value())
            return nullptr;
        return &skins[selectedSkin.value()];
    }

    constexpr ModelMesh *getSelectedMesh()
    {
        if (!selectedMesh.has_value())
            return nullptr;
        return &meshes[selectedMesh.value()];
    }

    ModelFrame &getSelectedFrame() { return frames[selectedFrame]; }

    constexpr const ModelSkin *getSelectedSkin() const
    {
        if (!selectedSkin.has_value())
            return nullptr;
        return &skins[selectedSkin.value()];
    }

    constexpr const ModelMesh *getSelectedMesh() const
    {
        if (!selectedMesh.has_value())
            return nullptr;
        return &meshes[selectedMesh.value()];
    }

    const ModelFrame &getSelectedFrame() const { return frames[selectedFrame]; }

    inline aabb3 boundsOfFrame(int32_t frame) const
    {
        aabb3 bounds;

        for (auto &mesh : meshes)
            for (auto &vert : mesh.frames[frame].vertices)
                if (vert.is_vertex())
                    bounds.add(vert.vertex().position);

        if (bounds.empty())
            return aabb3(0);

        return bounds;
    }

    inline aabb3 boundsOfAllFrames() const
    {
        aabb3 bounds;

        for (size_t i = 0; i < frames.size(); i++)
        {
            auto fb = boundsOfFrame(i);
            bounds.add(fb.mins);
            bounds.add(fb.maxs);
        }

        if (bounds.empty())
            return aabb3(0);

        return bounds;
    }

    static ModelData blankModel()
    {
        ModelData data;
	    data.frames.emplace_back("Frame 1");
	    data.meshes.emplace_back();
	    data.meshes[0].frames.emplace_back();
        return data;
    }
    
    auto stream_data()
    {
        return std::tie(frames, meshes, skins, selectedFrame, selectedSkin, skinPerObject, selectedMesh);
    }

private:
    friend class ModelMutator;
};