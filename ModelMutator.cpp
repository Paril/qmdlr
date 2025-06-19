#include <unordered_set>
#include <sul/dynamic_bitset.hpp>
#include "UndoRedo.h"
#include "ModelLoader.h"
#include "UI.h"

#pragma region(Frame Operations)
class UndoRedoStateFrameChanged : public UndoRedoState
{
public:
    UndoRedoStateFrameChanged() = default;

	UndoRedoStateFrameChanged(int32_t from, int32_t to) :
        UndoRedoState(),
        from(from),
        to(to)
    {
    }

	void Undo(ModelData *data) override
    {
        data->selectedFrame = from;

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        data->selectedFrame = to;

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return "Frame Changed";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, from);
        serialize_read(input, to);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, from);
        serialize_write(output, to);
    }

    virtual size_t Size() const override { return sizeof(*this); }

    virtual bool AutoRedo() const override { return false; }

	SET_UNDO_REDO_ID(UndoRedoStateFrameChanged)

private:
    int32_t from, to;
};

REGISTER_UNDO_REDO_ID(UndoRedoStateFrameChanged);

void ModelMutator::setSelectedFrame(int32_t frame)
{
    static bool changeFrameHandle = false;
    undo().PushDeferred<int>(changeFrameHandle,
        [data = data]() -> auto { return data->selectedFrame; },
        [data = data](const auto &state) {
            if (state != data->selectedFrame)
                undo().Push(new UndoRedoStateFrameChanged(state, data->selectedFrame));
        }
    );
    data->selectedFrame = frame;
    ui().editor3D().renderer().markBufferDirty();
}

class UndoRedoStateFrameNameChanged : public UndoRedoState
{
public:
    UndoRedoStateFrameNameChanged() = default;

	UndoRedoStateFrameNameChanged(std::string from, std::string to) :
        UndoRedoState(),
        from(from),
        to(to)
    {
        from.shrink_to_fit();
        to.shrink_to_fit();
    }

	void Undo(ModelData *data) override
    {
        data->getSelectedFrame().name = from;
    }

	void Redo(ModelData *data) override
    {
        data->getSelectedFrame().name = to;
    }

	const char *Name() const override
    {
        return "Frame Name Changed";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, from);
        serialize_read(input, to);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, from);
        serialize_write(output, to);
    }

    virtual size_t Size() const override { return sizeof(*this) + from.size() + to.size() + 2; }

    virtual bool AutoRedo() const override { return false; }

	SET_UNDO_REDO_ID(UndoRedoStateFrameNameChanged)

private:
    std::string from, to;
};

REGISTER_UNDO_REDO_ID(UndoRedoStateFrameNameChanged);

void ModelMutator::setSelectedFrameName(std::string &str)
{
    if (data->getSelectedFrame().name == str)
        return;

    auto state = new UndoRedoStateFrameNameChanged(std::move(data->getSelectedFrame().name), str);
    data->getSelectedFrame().name = std::move(str);
    undo().Push(state);
}
#pragma endregion

#pragma region(Skins)
class UndoRedoStateSkinChanged : public UndoRedoState
{
public:
    UndoRedoStateSkinChanged() = default;

	UndoRedoStateSkinChanged(std::optional<int32_t> from, std::optional<int32_t> to) :
        UndoRedoState(),
        from(from),
        to(to)
    {
    }

	void Undo(ModelData *data) override
    {
        data->selectedSkin = from;

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        data->selectedSkin = to;

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return "Skin Changed";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, from);
        serialize_read(input, to);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, from);
        serialize_write(output, to);
    }

    virtual size_t Size() const override { return sizeof(*this); }

    virtual bool AutoRedo() const override { return false; }

	SET_UNDO_REDO_ID(UndoRedoStateSkinChanged)

private:
    std::optional<int32_t> from, to;
};

REGISTER_UNDO_REDO_ID(UndoRedoStateSkinChanged);

void ModelMutator::setSelectedSkin(std::optional<int32_t> skin)
{
    static bool changeSkinHandle = false;
    undo().PushDeferred<std::optional<int32_t>>(changeSkinHandle,
        [data = data]() -> auto { return data->selectedSkin; },
        [data = data](const auto &state) {
            if (state != data->selectedSkin)
                undo().Push(new UndoRedoStateSkinChanged(state, data->selectedSkin));
        }
    );
    data->selectedSkin = skin;
}

class UndoRedoStateSkinNameChanged : public UndoRedoState
{
public:
    UndoRedoStateSkinNameChanged() = default;

	UndoRedoStateSkinNameChanged(std::string from, std::string to) :
        UndoRedoState(),
        from(from),
        to(to)
    {
        from.shrink_to_fit();
        to.shrink_to_fit();
    }

	void Undo(ModelData *data) override
    {
        data->getSelectedSkin()->name = from;
    }

	void Redo(ModelData *data) override
    {
        data->getSelectedSkin()->name = to;
    }

	const char *Name() const override
    {
        return "Skin Name Changed";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, from);
        serialize_read(input, to);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, from);
        serialize_write(output, to);
    }

    virtual size_t Size() const override { return sizeof(*this) + from.size() + to.size() + 2; }

    virtual bool AutoRedo() const override { return false; }

	SET_UNDO_REDO_ID(UndoRedoStateSkinNameChanged)

private:
    std::string from, to;
};

REGISTER_UNDO_REDO_ID(UndoRedoStateSkinNameChanged);

void ModelMutator::setSelectedSkinName(std::string &str)
{
    if (!data->selectedSkin)
        return;

    auto skin = data->getSelectedSkin();

    if (skin->name == str)
        return;

    auto state = new UndoRedoStateSkinNameChanged(std::move(skin->name), str);
    skin->name = std::move(str);
    undo().Push(state);
}

void ModelMutator::setNextSkin()
{
    setSelectedSkin(std::min((int) data->skins.size() - 1, data->selectedSkin.value_or(0) + 1));
}

void ModelMutator::setPreviousSkin()
{
    setSelectedSkin(std::max(0, data->selectedSkin.value_or(0) - 1));
}

class UndoRedoStateAddSkin : public UndoRedoState
{
public:
    UndoRedoStateAddSkin() = default;

	void Undo(ModelData *data) override
    {
        data->skins.pop_back();

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        int w = data->skins.size() ? data->skins[0].width : 64;
        int h = data->skins.size() ? data->skins[0].height : 64;

        auto &skin = data->skins.emplace_back();
        skin.name = std::format("Skin {}", data->skins.size());
        skin.width = w;
        skin.height = h;
        skin.image = Image::create_rgba(w, h);

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return "Skin Added";
    }

	virtual void Read(read_adapter &input) override
    {
    }

	virtual void Write(write_adapter &output) const override
    {
    }

    virtual size_t Size() const override { return sizeof(*this); }

	SET_UNDO_REDO_ID(UndoRedoStateAddSkin)
};

REGISTER_UNDO_REDO_ID(UndoRedoStateAddSkin);

void ModelMutator::addSkin()
{
    undo().BeginCombined();

    undo().Push(new UndoRedoStateAddSkin());
    undo().Push(new UndoRedoStateSkinChanged(model().model().selectedMesh, (int) (model().model().skins.size() - 1)));

    undo().EndCombined();
}

class UndoRedoStateDeleteSkin : public UndoRedoState
{
public:
    UndoRedoStateDeleteSkin() = default;

    UndoRedoStateDeleteSkin(int32_t index) :
        UndoRedoState(),
        index(index)
    {
    }

	void Undo(ModelData *data) override
    {
        // give the skin back
        auto &skins = data->skins;

        skins.insert(skins.begin() + index, std::move(skin));

        data->selectedSkin = index;

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        // steal the skin
        auto &skins = data->skins;

        skin = std::move(skins[index]);
        skin.handle.reset();

        skins.erase(skins.begin() + index);

        if (index >= skins.size())
        {
            if (skins.empty())
                data->selectedSkin = std::nullopt;
            else
                data->selectedSkin = (int) (skins.size() - 1);
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return "Skin Deleted";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, index);
        serialize_read(input, skin);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, index);
        serialize_write(output, skin);
    }

    virtual size_t Size() const override { return sizeof(*this) + skin.image.data_size(); }

	SET_UNDO_REDO_ID(UndoRedoStateDeleteSkin)

private:
    int32_t   index;
    ModelSkin skin;
};

REGISTER_UNDO_REDO_ID(UndoRedoStateDeleteSkin);

void ModelMutator::deleteSkin()
{
    if (!model().model().selectedSkin.has_value())
        return;

    undo().Push(new UndoRedoStateDeleteSkin(model().model().selectedSkin.value()));
}

class UndoRedoStateResizeSkin : public UndoRedoState
{
public:
    UndoRedoStateResizeSkin() = default;

    UndoRedoStateResizeSkin(int32_t index, int32_t width, int32_t height, bool resizeUVs, bool resizeImage) :
        UndoRedoState(),
        index(index),
        width(width),
        height(height),
        resizeUVs(resizeUVs),
        resizeImage(resizeImage)
    {
    }

	void Undo(ModelData *data) override
    {
        // swap the old skin back
        auto &newSkin = data->skins[index];

        newSkin.handle.reset();
        std::swap(newSkin, skin);

        if (resizeUVs)
        {
            glm::vec2 old_f2i { (float) 1.0f / skin.width, (float) 1.0f / skin.height };
            glm::vec2 new_f2i { (float) 1.0f / newSkin.width, (float) 1.0f / newSkin.height };

            size_t i = 0;

            for (auto &mesh : data->meshes)
                for (auto &tc : mesh.texcoords)
                    tc.pos = uvData[i++];
        }

        // destroy the resized skin; we'll recreate it on undo.
        skin = {};

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        // resize the skin, swap them
        auto &oldSkin = data->skins[index];

        ModelSkin newSkin;
        newSkin.name = oldSkin.name;
        newSkin.width = width;
        newSkin.height = height;
        newSkin.q1_data = oldSkin.q1_data;

        newSkin.image = oldSkin.image.resized(newSkin.width, newSkin.height, resizeImage);

        if (resizeUVs)
        {
            glm::vec2 old_f2i { (float) 1.0f / oldSkin.width, (float) 1.0f / oldSkin.height };
            glm::vec2 new_f2i { (float) 1.0f / width, (float) 1.0f / height };

            for (auto &mesh : data->meshes)
                for (auto &tc : mesh.texcoords)
                {
                    uvData.push_back(tc.pos);
                    tc.pos = (tc.pos / old_f2i) * new_f2i;
                }
        }
        else
        {
            // no change to UVs
        }

        oldSkin.handle.reset();
        std::swap(newSkin, oldSkin);
        skin = std::move(newSkin);

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return "Skin Resized";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, index, width, height, resizeUVs, resizeImage);
        serialize_read(input, skin);
        serialize_read(input, uvData);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, index, width, height, resizeUVs, resizeImage);
        serialize_write(output, skin);
        serialize_write(output, uvData);
    }

    virtual size_t Size() const override
    {
        return sizeof(*this) +
            skin.image.data_size() +
            vector_element_size(uvData);
    }

	SET_UNDO_REDO_ID(UndoRedoStateResizeSkin)

private:
    int32_t   index;
    int32_t   width;
    int32_t   height;
    bool      resizeUVs;
    bool      resizeImage;
    ModelSkin skin;

    std::vector<glm::vec2> uvData;
};

REGISTER_UNDO_REDO_ID(UndoRedoStateResizeSkin);

void ModelMutator::resizeSkin(int width, int height, bool resizeUVs, bool resizeImage)
{
    if (!model().model().selectedSkin.has_value())
        return;

    undo().Push(new UndoRedoStateResizeSkin(model().model().selectedSkin.value(), width, height, resizeUVs, resizeImage));
}

class UndoRedoStateMoveSkin : public UndoRedoState
{
public:
    UndoRedoStateMoveSkin() = default;

    UndoRedoStateMoveSkin(int32_t old_index, int32_t new_index, bool after) :
        UndoRedoState(),
        old_index(old_index),
        new_index(new_index),
        after(after)
    {
    }

	void Undo(ModelData *data) override
    {
        cut_paste(data->skins.begin() + new_index, data->skins.begin() + new_index + 1, data->skins.begin() + old_index + (!after ? 1 : 0));

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        cut_paste(data->skins.begin() + old_index, data->skins.begin() + old_index + 1, data->skins.begin() + new_index + (after ? 1 : 0));

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return "Skin Moved";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, old_index, new_index);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, old_index, new_index);
    }

    virtual size_t Size() const override { return sizeof(*this); }

	SET_UNDO_REDO_ID(UndoRedoStateMoveSkin)

private:
    int32_t   old_index;
    int32_t   new_index;
    bool      after;
};

REGISTER_UNDO_REDO_ID(UndoRedoStateMoveSkin);

void ModelMutator::moveSkin(int target, bool after)
{
    if (!model().model().selectedSkin.has_value())
        return;

    int old_index = model().model().selectedSkin.value();
    int new_index = target;

    undo().Push(new UndoRedoStateMoveSkin(old_index, new_index, after));
}

class UndoRedoStateImportSkin : public UndoRedoState
{
public:
    UndoRedoStateImportSkin() = default;

    UndoRedoStateImportSkin(ModelSkin &skin) :
        UndoRedoState(),
        skin(std::move(skin))
    {
    }

	void Undo(ModelData *data) override
    {
        auto &selectedSkin = *data->getSelectedSkin();
        std::swap(selectedSkin, skin);
        selectedSkin.handle.reset();
        skin.handle.reset();

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        auto &selectedSkin = *data->getSelectedSkin();
        std::swap(selectedSkin, skin);
        selectedSkin.handle.reset();
        skin.handle.reset();

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return "Skin Imported";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, skin);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, skin);
    }

    virtual size_t Size() const override { return sizeof(*this); }

	SET_UNDO_REDO_ID(UndoRedoStateImportSkin)

private:
    ModelSkin     skin;
};

REGISTER_UNDO_REDO_ID(UndoRedoStateImportSkin);

void ModelMutator::importSkin(Image &image)
{
    undo().BeginCombined();
    
    // if we don't have a skin, make one
    if (!data->selectedSkin.has_value())
        addSkin();
    
    ModelSkin skin;
    skin.name = "Imported Skin";
    skin.width = image.width;
    skin.height = image.height;
    skin.image = std::move(image);

    undo().Push(new UndoRedoStateImportSkin(skin));

    undo().EndCombined();
}
#pragma endregion

#pragma region(Selected Vertices Shared)
template<template_string text, template_string id, auto TVertsMember, auto TVertMember>
class UndoRedoVerticesSelected : public UndoRedoState
{
public:
	UndoRedoVerticesSelected() :
        UndoRedoState()
    {
    }

	void Undo(ModelData *data) override
    {
        for (size_t i = 0, tc = 0; i < mesh_vertices.size(); i++)
        {
            size_t mesh_id = mesh_vertices[i++];
            size_t vertex_count = mesh_vertices[i++];

            for (size_t v = 0; v < vertex_count; v++)
            {
                ((data->meshes[mesh_id].*TVertsMember)[mesh_vertices[i++]].*TVertMember) = selection_states[tc++];
            }
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        for (size_t i = 0, tc = 0; i < mesh_vertices.size(); i++)
        {
            size_t mesh_id = mesh_vertices[i++];
            size_t vertex_count = mesh_vertices[i++];

            for (size_t v = 0; v < vertex_count; v++)
            {
                ((data->meshes[mesh_id].*TVertsMember)[mesh_vertices[i++]].*TVertMember) = !selection_states[tc++];
            }
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return text.value;
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, mesh_vertices, selection_states);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, mesh_vertices, selection_states);
    }

    virtual size_t Size() const override
    {
        return sizeof(*this)
            + vector_element_size(mesh_vertices)
            + (selection_states.num_blocks() * (selection_states.bits_per_block / 8));
    }
    
    virtual const char *Id() const override { return id.value; }

private:
    std::vector<size_t> mesh_vertices;
    sul::dynamic_bitset<> selection_states;

    friend class ModelMutator;

    using TCoordType = typename detail::extract_class_type<decltype(TVertMember)>::type;

    void SelectInternal(ModelMutator &mutator, int32_t mesh,
                        std::function<std::optional<bool>(const TCoordType &, size_t index)> change)
    {
        std::optional<size_t> first = std::nullopt;
        auto data = mutator.data;

        for (size_t i = 0; i < (data->meshes[mesh].*TVertsMember).size(); i++)
        {
            auto &tc = (data->meshes[mesh].*TVertsMember)[i];
            auto new_state = change(tc, i);

            if (!new_state.has_value())
                continue;
            else if (new_state.value() == (tc.*TVertMember))
                continue;

            if (!first.has_value())
            {
                mesh_vertices.push_back(mesh);
                mesh_vertices.push_back(0);
                first = mesh_vertices.size() - 1;
            }

            mesh_vertices[first.value()]++;
            mesh_vertices.push_back(i);
            selection_states.push_back((tc.*TVertMember));
        }

        mesh_vertices.shrink_to_fit();
        selection_states.shrink_to_fit();
    }

    static void SelectAll(ModelMutator &mutator)
    {
        auto state = std::make_unique<UndoRedoVerticesSelected>();
        auto data = mutator.data;

        for (size_t m = 0; m < data->meshes.size(); m++)
        {
            if (data->selectedMesh.has_value() && data->selectedMesh != m)
                continue;

            state->SelectInternal(mutator, m, [](const TCoordType &tc, size_t _) -> std::optional<bool> {
                if ((tc.*TVertMember) == true)
                    return std::nullopt;

                return true;
            });
        }

        if (!state->mesh_vertices.empty())
        {
            undo().Push(std::move(state));
        }
    }
    
    static void SelectNone(ModelMutator &mutator)
    {
        auto state = std::make_unique<UndoRedoVerticesSelected>();
        auto data = mutator.data;

        for (size_t m = 0; m < data->meshes.size(); m++)
        {
            if (data->selectedMesh.has_value() && data->selectedMesh != m)
                continue;

            state->SelectInternal(mutator, m, [](const TCoordType &tc, size_t _) -> std::optional<bool> {
                if ((tc.*TVertMember) == false)
                    return std::nullopt;

                return false;
            });
        }

        if (!state->mesh_vertices.empty())
        {
            undo().Push(std::move(state));
        }
    }
    
    static void SelectInverse(ModelMutator &mutator)
    {
        auto state = std::make_unique<UndoRedoVerticesSelected>();
        auto data = mutator.data;

        for (size_t m = 0; m < data->meshes.size(); m++)
        {
            if (data->selectedMesh.has_value() && data->selectedMesh != m)
                continue;

            state->SelectInternal(mutator, m, [](const TCoordType &tc, size_t _) -> std::optional<bool> {
                return !(tc.*TVertMember);
            });
        }

        if (!state->mesh_vertices.empty())
        {
            undo().Push(std::move(state));
        }
    }

    template<auto TTriangleType>
    static bool SelectTouching(ModelMutator &mutator)
    {
        auto state = std::make_unique<UndoRedoVerticesSelected>();
        auto data = mutator.data;
        std::unordered_set<size_t> selected;
    
        for (size_t m = 0; m < data->meshes.size(); m++)
        {
            if (data->selectedMesh.has_value() && data->selectedMesh != m)
                continue;

            selected.clear();

            for (size_t i = 0; i < (data->meshes[m].*TVertsMember).size(); i++)
                if ((data->meshes[m].*TVertsMember)[i].*TVertMember)
                    selected.insert(i);

            std::unordered_set<size_t> changed;

            for (auto &tri : data->meshes[m].triangles)
            {
                if (selected.contains((tri.*TTriangleType)[0]) ||
                    selected.contains((tri.*TTriangleType)[1]) ||
                    selected.contains((tri.*TTriangleType)[2]))
                    changed.insert({ (tri.*TTriangleType)[0], (tri.*TTriangleType)[1], (tri.*TTriangleType)[2] });
            }

            state->SelectInternal(mutator, m, [&changed](const TCoordType &tc, size_t index) -> std::optional<bool> {
                if ((tc.*TVertMember) || !changed.contains(index))
                    return std::nullopt;

                return true;
            });
        }

        if (!state->mesh_vertices.empty())
        {
            undo().Push(std::move(state));
            return true;
        }

        return false;
    }

    static void SelectRectangle(ModelMutator &mutator, const aabb2 &rect, const std::function<glm::vec2(size_t mesh, size_t coord_index)> &transformer)
    {
        auto &io = ImGui::GetIO();
        bool new_state = !io.KeyAlt;
        auto state = std::make_unique<UndoRedoVerticesSelected>();
        auto data = mutator.data;

        for (size_t m = 0; m < data->meshes.size(); m++)
        {
            if (data->selectedMesh.has_value() && data->selectedMesh != m)
                continue;

            state->SelectInternal(mutator, m, [&m, &new_state, &rect, &transformer](const TCoordType &tc, size_t index) -> std::optional<bool> {
                if (!rect.contains(transformer(m, index)))
                    return std::nullopt;
                else if (tc.*TVertMember == new_state)
                    return std::nullopt;

                return new_state;
            });
        }

        if (!state->mesh_vertices.empty())
        {
            undo().Push(std::move(state));
        }
    }
};
#pragma endregion

#pragma region(Select UV Vertices)
using UndoRedoUVVerticesSelected = UndoRedoVerticesSelected<"UV Vertices Selected", "UndoRedoUVVerticesSelected", &ModelMesh::texcoords, &ModelTexCoord::selected>;

REGISTER_UNDO_REDO_ID(UndoRedoUVVerticesSelected);

void ModelMutator::selectRectangleVerticesUV(const aabb2 &rect)
{
    UndoRedoUVVerticesSelected::SelectRectangle(*this, rect, [this](size_t mesh, size_t coord_index) {
        return data->meshes[mesh].texcoords[coord_index].pos;
    });
}

void ModelMutator::selectAllVerticesUV()
{
    UndoRedoUVVerticesSelected::SelectAll(*this);
}

void ModelMutator::selectNoneVerticesUV()
{
    UndoRedoUVVerticesSelected::SelectNone(*this);
}

void ModelMutator::selectInverseVerticesUV()
{
    UndoRedoUVVerticesSelected::SelectInverse(*this);
}

void ModelMutator::selectTouchingVerticesUV()
{
    UndoRedoUVVerticesSelected::SelectTouching<&ModelTriangle::texcoords>(*this);
}

void ModelMutator::selectConnectedVerticesUV()
{
    // FIXME optimize
    undo().BeginCombined();
    while (UndoRedoUVVerticesSelected::SelectTouching<&ModelTriangle::texcoords>(*this)) ;
    undo().EndCombined();
}
#pragma endregion

#pragma region(Select 3D Vertices)
using UndoRedo3DVerticesSelected = UndoRedoVerticesSelected<"3D Vertices Selected", "UndoRedo3DVerticesSelected", &ModelMesh::vertices, &ModelVertex::selected>;

REGISTER_UNDO_REDO_ID(UndoRedo3DVerticesSelected);

void ModelMutator::selectRectangleVertices3D(const aabb2 &rect, const std::function<glm::vec2(size_t mesh, size_t coord_index)> &transformer)
{
    UndoRedo3DVerticesSelected::SelectRectangle(*this, rect, transformer);
}

void ModelMutator::selectAllVertices3D()
{
    UndoRedo3DVerticesSelected::SelectAll(*this);
}

void ModelMutator::selectNoneVertices3D()
{
    UndoRedo3DVerticesSelected::SelectNone(*this);
}

void ModelMutator::selectInverseVertices3D()
{
    UndoRedo3DVerticesSelected::SelectInverse(*this);
}

void ModelMutator::selectTouchingVertices3D()
{
    UndoRedo3DVerticesSelected::SelectTouching<&ModelTriangle::vertices>(*this);
}

void ModelMutator::selectConnectedVertices3D()
{
    // FIXME optimize
    undo().BeginCombined();
    while (UndoRedo3DVerticesSelected::SelectTouching<&ModelTriangle::vertices>(*this)) ;
    undo().EndCombined();
}
#pragma endregion

#pragma region(Selected Triangles Shared)
template<template_string text, template_string id, auto TTriSelectedMember>
class UndoRedoTrianglesSelected : public UndoRedoState
{
public:
	UndoRedoTrianglesSelected() :
        UndoRedoState()
    {
    }

	void Undo(ModelData *data) override
    {
        for (size_t i = 0, tc = 0; i < mesh_triangles.size(); i++)
        {
            size_t mesh_id = mesh_triangles[i++];
            size_t tri_count = mesh_triangles[i++];

            for (size_t t = 0; t < tri_count; t++)
            {
                (data->meshes[mesh_id].triangles[mesh_triangles[i++]].*TTriSelectedMember) = selection_states[tc++];
            }
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        auto mutator = model().mutator();

        for (size_t i = 0, tc = 0; i < mesh_triangles.size(); i++)
        {
            size_t mesh_id = mesh_triangles[i++];
            size_t tri_count = mesh_triangles[i++];

            for (size_t t = 0; t < tri_count; t++)
            {
                (data->meshes[mesh_id].triangles[mesh_triangles[i++]].*TTriSelectedMember) = !selection_states[tc++];
            }
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return text.value;
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, mesh_triangles, selection_states);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, mesh_triangles, selection_states);
    }

    virtual size_t Size() const override
    {
        return sizeof(*this)
            + vector_element_size(mesh_triangles)
            + (selection_states.num_blocks() * (selection_states.bits_per_block / 8));
    }
    
    virtual const char *Id() const override { return id.value; }

private:
    std::vector<size_t> mesh_triangles;
    sul::dynamic_bitset<> selection_states;

    friend class ModelMutator;

    void SelectInternal(ModelMutator &mutator, int32_t mesh,
                        std::function<std::optional<bool>(const ModelTriangle &, size_t index)> change)
    {
        std::optional<size_t> first = std::nullopt;
        auto data = mutator.data;

        for (size_t i = 0; i < data->meshes[mesh].triangles.size(); i++)
        {
            auto &tri = data->meshes[mesh].triangles[i];
            auto new_state = change(tri, i);

            if (!new_state.has_value())
                continue;
            else if (new_state.value() == tri.*TTriSelectedMember)
                continue;

            if (!first.has_value())
            {
                mesh_triangles.push_back(mesh);
                mesh_triangles.push_back(0);
                first = mesh_triangles.size() - 1;
            }

            mesh_triangles[first.value()]++;
            mesh_triangles.push_back(i);
            selection_states.push_back(tri.*TTriSelectedMember);
        }

        mesh_triangles.shrink_to_fit();
        selection_states.shrink_to_fit();
    }

    static void SelectAll(ModelMutator &mutator)
    {
        auto state = std::make_unique<UndoRedoTrianglesSelected>();
        auto data = mutator.data;

        for (size_t m = 0; m < data->meshes.size(); m++)
        {
            if (data->selectedMesh.has_value() && data->selectedMesh != m)
                continue;

            state->SelectInternal(mutator, m, [](const ModelTriangle &tri, size_t _) -> std::optional<bool> {
                if (tri.*TTriSelectedMember == true)
                    return std::nullopt;

                return true;
            });
        }

        if (!state->mesh_triangles.empty())
        {
            undo().Push(std::move(state));
        }
    }

    static void SelectNone(ModelMutator &mutator)
    {
        auto state = std::make_unique<UndoRedoTrianglesSelected>();
        auto data = mutator.data;

        for (size_t m = 0; m < data->meshes.size(); m++)
        {
            if (data->selectedMesh.has_value() && data->selectedMesh != m)
                continue;

            state->SelectInternal(mutator, m, [](const ModelTriangle &tri, size_t _) -> std::optional<bool> {
                if (tri.*TTriSelectedMember == false)
                    return std::nullopt;

                return false;
            });
        }

        if (!state->mesh_triangles.empty())
        {
            undo().Push(std::move(state));
        }
    }

    static void SelectInverse(ModelMutator &mutator)
    {
        auto state = std::make_unique<UndoRedoTrianglesSelected>();
        auto data = mutator.data;

        for (size_t m = 0; m < data->meshes.size(); m++)
        {
            if (data->selectedMesh.has_value() && data->selectedMesh != m)
                continue;

            state->SelectInternal(mutator, m, [](const ModelTriangle &tri, size_t _) -> std::optional<bool> {
                return !(tri.*TTriSelectedMember);
            });
        }

        if (!state->mesh_triangles.empty())
        {
            undo().Push(std::move(state));
        }
    }

    template<auto TCoordinatesMember>
    static bool SelectTouching(ModelMutator &mutator)
    {
        auto state = std::make_unique<UndoRedoTrianglesSelected>();
        auto data = mutator.data;
        std::unordered_set<size_t> selected;
    
        for (size_t m = 0; m < data->meshes.size(); m++)
        {
            if (data->selectedMesh.has_value() && data->selectedMesh != m)
                continue;

            selected.clear();

            for (auto &tri : data->meshes[m].triangles)
                if (tri.*TTriSelectedMember)
                    for (auto &tc : tri.*TCoordinatesMember)
                        selected.insert(tc);

            state->SelectInternal(mutator, m, [&selected](const ModelTriangle &tri, size_t index) -> std::optional<bool> {
                if (tri.*TTriSelectedMember)
                    return std::nullopt;
            
                if (selected.contains((tri.*TCoordinatesMember)[0]) ||
                    selected.contains((tri.*TCoordinatesMember)[1]) ||
                    selected.contains((tri.*TCoordinatesMember)[2]))
                    return true;

                return false;
            });
        }

        if (!state->mesh_triangles.empty())
        {
            undo().Push(std::move(state));
            return true;
        }

        return false;
    }
    
    template<auto TCoordinatesMember>
    static void SelectRectangle(ModelMutator &mutator, const aabb2 &rect, const std::function<glm::vec2(size_t, size_t)> &pos_getter)
    {
        auto &io = ImGui::GetIO();
        bool new_state = !io.KeyAlt;
        auto state = std::make_unique<UndoRedoTrianglesSelected>();
        auto data = mutator.data;

        for (size_t m = 0; m < data->meshes.size(); m++)
        {
            if (data->selectedMesh.has_value() && data->selectedMesh != m)
                continue;

            state->SelectInternal(mutator, m, [m, data = data, &new_state, &rect, &pos_getter](const ModelTriangle &tri, size_t index) -> std::optional<bool> {
                if (tri.*TTriSelectedMember == new_state)
                    return std::nullopt;

                for (auto &tci : tri.*TCoordinatesMember)
                    if (rect.contains(pos_getter(m, tci)))
                        return new_state;

                return std::nullopt;
            });
        }

        if (!state->mesh_triangles.empty())
        {
            undo().Push(std::move(state));
        }
    }

    template<auto TOtherSelectMember>
    static void Sync(ModelMutator &mutator)
    {
        auto state = std::make_unique<UndoRedoTrianglesSelected>();
        auto data = mutator.data;

        for (size_t m = 0; m < data->meshes.size(); m++)
        {
            if (data->selectedMesh.has_value() && data->selectedMesh != m)
                continue;

            state->SelectInternal(mutator, m, [](const ModelTriangle &tri, size_t _) -> std::optional<bool> {
                return tri.*TOtherSelectMember;
            });
        }

        if (!state->mesh_triangles.empty())
        {
            undo().Push(std::move(state));
        }
    }
};
#pragma endregion

#pragma region(Select UV Triangles)
using UndoRedoUVTrianglesSelected = UndoRedoTrianglesSelected<"UV Triangles Selected", "UndoRedoUVTrianglesSelected", &ModelTriangle::selectedUV>;

REGISTER_UNDO_REDO_ID(UndoRedoUVTrianglesSelected);

void ModelMutator::selectRectangleTrianglesUV(const aabb2 &rect)
{
    if (ui().syncSelection)
        undo().BeginCombined();

    UndoRedoUVTrianglesSelected::SelectRectangle<&ModelTriangle::texcoords>(*this, rect, [data = data](size_t mesh, size_t element) {
        return data->meshes[mesh].texcoords[element].pos;
    });

    if (ui().syncSelection)
    {
        syncSelectionUV();
        undo().EndCombined();
    }
}

void ModelMutator::selectAllTrianglesUV()
{
    if (ui().syncSelection)
        undo().BeginCombined();

    UndoRedoUVTrianglesSelected::SelectAll(*this);

    if (ui().syncSelection)
    {
        syncSelectionUV();
        undo().EndCombined();
    }
}

void ModelMutator::selectNoneTrianglesUV()
{
    if (ui().syncSelection)
        undo().BeginCombined();

    UndoRedoUVTrianglesSelected::SelectNone(*this);

    if (ui().syncSelection)
    {
        syncSelectionUV();
        undo().EndCombined();
    }
}

void ModelMutator::selectInverseTrianglesUV()
{
    if (ui().syncSelection)
        undo().BeginCombined();

    UndoRedoUVTrianglesSelected::SelectInverse(*this);

    if (ui().syncSelection)
    {
        syncSelectionUV();
        undo().EndCombined();
    }
}

void ModelMutator::selectTouchingTrianglesUV()
{
    if (ui().syncSelection)
        undo().BeginCombined();

    UndoRedoUVTrianglesSelected::SelectTouching<&ModelTriangle::texcoords>(*this);

    if (ui().syncSelection)
    {
        syncSelectionUV();
        undo().EndCombined();
    }
}

void ModelMutator::selectConnectedTrianglesUV()
{
    // FIXME optimize
    undo().BeginCombined();
    while (UndoRedoUVTrianglesSelected::SelectTouching<&ModelTriangle::texcoords>(*this)) ;
    if (ui().syncSelection)
        syncSelectionUV();
    undo().EndCombined();
}
#pragma endregion

#pragma region(Select 3D Triangles)
using UndoRedo3DTrianglesSelected = UndoRedoTrianglesSelected<"3D Triangles Selected", "UndoRedo3DTrianglesSelected", &ModelTriangle::selectedFace>;

REGISTER_UNDO_REDO_ID(UndoRedo3DTrianglesSelected);

void ModelMutator::selectRectangleTriangles3D(const aabb2 &rect, const std::function<glm::vec2(size_t mesh, size_t element)> &transformer)
{
    if (ui().syncSelection)
        undo().BeginCombined();

    UndoRedo3DTrianglesSelected::SelectRectangle<&ModelTriangle::vertices>(*this, rect, transformer);

    if (ui().syncSelection)
    {
        syncSelection3D();
        undo().EndCombined();
    }
}

void ModelMutator::selectAllTriangles3D()
{
    if (ui().syncSelection)
        undo().BeginCombined();

    UndoRedo3DTrianglesSelected::SelectAll(*this);

    if (ui().syncSelection)
    {
        syncSelection3D();
        undo().EndCombined();
    }
}

void ModelMutator::selectNoneTriangles3D()
{
    if (ui().syncSelection)
        undo().BeginCombined();

    UndoRedo3DTrianglesSelected::SelectNone(*this);

    if (ui().syncSelection)
    {
        syncSelection3D();
        undo().EndCombined();
    }
}

void ModelMutator::selectInverseTriangles3D()
{
    if (ui().syncSelection)
        undo().BeginCombined();

    UndoRedo3DTrianglesSelected::SelectInverse(*this);

    if (ui().syncSelection)
    {
        syncSelection3D();
        undo().EndCombined();
    }
}

void ModelMutator::selectTouchingTriangles3D()
{
    if (ui().syncSelection)
        undo().BeginCombined();

    UndoRedo3DTrianglesSelected::SelectTouching<&ModelTriangle::vertices>(*this);

    if (ui().syncSelection)
    {
        syncSelection3D();
        undo().EndCombined();
    }
}

void ModelMutator::selectConnectedTriangles3D()
{
    // FIXME optimize
    undo().BeginCombined();
    while (UndoRedo3DTrianglesSelected::SelectTouching<&ModelTriangle::vertices>(*this));
    if (ui().syncSelection)
        syncSelection3D();
    undo().EndCombined();
}
#pragma endregion

#pragma region(Sync)
void ModelMutator::syncSelectionUV()
{
    UndoRedo3DTrianglesSelected::Sync<&ModelTriangle::selectedUV>(*this);
}

void ModelMutator::syncSelection3D()
{
    UndoRedoUVTrianglesSelected::Sync<&ModelTriangle::selectedFace>(*this);
}
#pragma endregion

#pragma region(UV Matrix Apply)
class UndoRedoUVCoordinatesTransformed : public UndoRedoState
{
public:
    UndoRedoUVCoordinatesTransformed() = default;

	UndoRedoUVCoordinatesTransformed(const glm::mat4 &matrix) :
        UndoRedoState(),
        matrix(matrix)
    {
    }

	void Undo(ModelData *data) override
    {
        for (size_t i = 0, tc = 0; i < mesh_vertices.size(); i++)
        {
            size_t mesh_id = mesh_vertices[i++];
            size_t vertex_count = mesh_vertices[i++];

            for (size_t v = 0; v < vertex_count; v++)
            {
                data->meshes[mesh_id].texcoords[mesh_vertices[i++]].pos = uv_positions[tc++];
            }
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        for (size_t i = 0, tc = 0; i < mesh_vertices.size(); i++)
        {
            size_t mesh_id = mesh_vertices[i++];
            size_t vertex_count = mesh_vertices[i++];

            for (size_t v = 0; v < vertex_count; v++)
            {
                auto &p = data->meshes[mesh_id].texcoords[mesh_vertices[i++]].pos;
                p = glm::vec2(matrix * glm::vec4(p, 0.f, 1.f));
            }
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return "UV Coords Transformed";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, matrix, mesh_vertices, uv_positions);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, matrix, mesh_vertices, uv_positions);
    }

    virtual size_t Size() const override
    {
        return sizeof(*this)
            + vector_element_size(mesh_vertices)
            + vector_element_size(uv_positions);
    }

	SET_UNDO_REDO_ID(UndoRedoUVCoordinatesTransformed)

private:
    glm::mat4              matrix;
    std::vector<size_t>    mesh_vertices;
    std::vector<glm::vec2> uv_positions;

    friend class ModelMutator;
};

REGISTER_UNDO_REDO_ID(UndoRedoUVCoordinatesTransformed);

// apply matrix to selected UV coords
void ModelMutator::applyUVMatrix(const glm::mat4 &matrix, SelectMode mode)
{
    const ModelSkin *skin = data->getSelectedSkin();

    if (!skin)
        return;

    int scale = ui().editorUV().scale();

    auto state = std::make_unique<UndoRedoUVCoordinatesTransformed>(matrix);

    for (size_t i = 0; i < data->meshes.size(); i++)
    {
        if (data->selectedMesh.has_value() && data->selectedMesh != i)
            continue;

        const auto &coords = getSelectedTextureCoordinates(data->meshes[i], mode);

        if (coords.empty())
            continue;

        state->mesh_vertices.push_back(i);
        state->mesh_vertices.push_back(coords.size());

        for (auto &v : coords)
        {
            state->mesh_vertices.push_back(v);
            state->uv_positions.push_back(data->meshes[i].texcoords[v].pos);
        }

        state->mesh_vertices.shrink_to_fit();
        state->uv_positions.shrink_to_fit();
    }

    if (!state->mesh_vertices.empty())
    {
        undo().Push(std::move(state));
    }
}
#pragma endregion

#pragma region(3D Matrix Apply)
class UndoRedo3DCoordinatesTransformed : public UndoRedoState
{
public:
    UndoRedo3DCoordinatesTransformed() = default;

	UndoRedo3DCoordinatesTransformed(const glm::mat4 &matrix) :
        UndoRedoState(),
        matrix(matrix)
    {
    }

	void Undo(ModelData *data) override
    {
        for (size_t i = 0, vt = 0; i < mesh_vertices.size(); i++)
        {
            size_t mesh_id = mesh_vertices[i++];
            size_t vertex_count = mesh_vertices[i++];

            for (size_t v = 0; v < vertex_count; v++)
                data->meshes[mesh_id].frames[data->selectedFrame].vertices[mesh_vertices[i++]] = vertice_data[vt++];
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        glm::mat3 normal = matrix;

        for (size_t i = 0, tc = 0; i < mesh_vertices.size(); i++)
        {
            size_t mesh_id = mesh_vertices[i++];
            size_t vertex_count = mesh_vertices[i++];

            for (size_t v = 0; v < vertex_count; v++)
            {
                auto &p = data->meshes[mesh_id].frames[data->selectedFrame].vertices[mesh_vertices[i++]];
                p = p.transform(matrix, normal);
            }
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return "3D Coords Transformed";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, matrix, mesh_vertices, vertice_data);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, matrix, mesh_vertices, vertice_data);
    }

    virtual size_t Size() const override
    {
        return sizeof(*this)
            + vector_element_size(mesh_vertices)
            + vector_element_size(vertice_data);
    }

	SET_UNDO_REDO_ID(UndoRedo3DCoordinatesTransformed)

private:
    glm::mat4                     matrix;
    std::vector<size_t>           mesh_vertices;
    std::vector<MeshFrameVertTag> vertice_data;

    friend class ModelMutator;
};

REGISTER_UNDO_REDO_ID(UndoRedo3DCoordinatesTransformed);

// apply matrix to selected UV coords
void ModelMutator::apply3DMatrix(const glm::mat4 &matrix, SelectMode mode)
{
    auto state = std::make_unique<UndoRedo3DCoordinatesTransformed>(matrix);

    for (size_t i = 0; i < data->meshes.size(); i++)
    {
        if (data->selectedMesh.has_value() && data->selectedMesh != i)
            continue;

        const auto &coords = getSelectedVertices(data->meshes[i], mode);

        if (coords.empty())
            continue;

        state->mesh_vertices.push_back(i);
        state->mesh_vertices.push_back(coords.size());

        for (auto &v : coords)
        {
            state->mesh_vertices.push_back(v);
            state->vertice_data.push_back(data->meshes[i].frames[data->selectedFrame].vertices[v]);
        }

        state->mesh_vertices.shrink_to_fit();
        state->vertice_data.shrink_to_fit();
    }

    if (!state->mesh_vertices.empty())
    {
        undo().Push(std::move(state));
    }
}
#pragma endregion

#pragma region(Delete UVs)
class UndoRedoUVFacesRemoved : public UndoRedoState
{
public:
    UndoRedoUVFacesRemoved() = default;

	void Undo(ModelData *data) override
    {
        vector_read_adapter stream(command_buffer);

        while (true)
        {
            size_t mesh_index;
            serialize_read(stream, mesh_index);
            
            if (mesh_index == (size_t) -1)
                break;

            auto &mesh = data->meshes[mesh_index];

            while (true)
            {
                size_t triangle_index;
                serialize_read(stream, triangle_index);

                if (triangle_index == (size_t) -1)
                    break;

                auto &tri = mesh.triangles[triangle_index];
                serialize_read(stream, tri.texcoords, tri.selectedUV);
            }
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        vector_read_adapter stream(command_buffer);

        while (true)
        {
            size_t mesh_index;
            serialize_read(stream, mesh_index);
            
            if (mesh_index == (size_t) -1)
                break;

            auto &mesh = data->meshes[mesh_index];

            while (true)
            {
                size_t triangle_index;
                serialize_read(stream, triangle_index);

                if (triangle_index == (size_t) -1)
                    break;

                auto &tri = mesh.triangles[triangle_index];
                tri.texcoords = {};
                tri.selectedUV = false;

                stream.skip(sizeof(tri.texcoords) + sizeof(tri.selectedUV));
            }
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return "UV Faces Removed";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, command_buffer);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, command_buffer);
    }

    virtual size_t Size() const override { return sizeof(*this) + command_buffer.size(); }

	SET_UNDO_REDO_ID(UndoRedoUVFacesRemoved)

private:
    std::vector<uint8_t>       command_buffer;

    void InitFromTriangles(const ModelData *data, std::function<bool(const ModelMesh &mesh, const ModelTriangle &tri)> func)
    {
        vector_write_adapter stream;

        for (auto &mesh : data->meshes)
        {
            size_t meshIndex = (size_t) (&mesh - data->meshes.data());

            if (data->selectedMesh.has_value() && data->selectedMesh != meshIndex)
                continue;

            bool meshWritten = false;

            for (auto &triangle : mesh.triangles)
            {
                if (!func(mesh, triangle))
                    continue;

                if (!meshWritten)
                {
                    meshWritten = true;
                    serialize_write(stream, meshIndex);
                }

                serialize_write(stream, (size_t) (&triangle - mesh.triangles.data()));
                serialize_write(stream, triangle.texcoords, triangle.selectedUV);
            }

            if (meshWritten)
                serialize_write(stream, (size_t) -1);
        }

        serialize_write(stream, (size_t) -1);

        command_buffer = std::move(stream.data);
        command_buffer.shrink_to_fit();
    }

    friend class ModelMutator;
};

REGISTER_UNDO_REDO_ID(UndoRedoUVFacesRemoved);

class UndoRedoUVVerticesRemoved : public UndoRedoState
{
    struct ModifiedTexCoord
    {
        uint32_t                index;
        std::array<uint32_t, 3> texcoords;
        bool                    selected;

        auto stream_data()
        {
            return std::tie(index, texcoords, selected);
        }
    };

public:
    UndoRedoUVVerticesRemoved() = default;

	void Undo(ModelData *data) override
    {
        vector_read_adapter stream(command_buffer);

        std::vector<uint32_t> removedIDs;
        std::vector<ModifiedTexCoord> changedFaces;

        while (true)
        {
            size_t mesh_index;
            serialize_read(stream, mesh_index);
            
            if (mesh_index == (size_t) -1)
                break;

            auto &mesh = data->meshes[mesh_index];
            
            removedIDs.clear();
            serialize_read(stream, removedIDs);

            for (auto it : removedIDs)
            {
                ModelTexCoord tc;
                serialize_read(stream, tc);
                mesh.texcoords.insert(mesh.texcoords.begin() + it, tc);
            }

            changedFaces.clear();
            serialize_read(stream, changedFaces);

            for (auto &face : changedFaces)
            {
                mesh.triangles[face.index].texcoords = face.texcoords;
                mesh.triangles[face.index].selectedUV = face.selected;
            }
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	void Redo(ModelData *data) override
    {
        // for redo, we just need to delete the entries in reverse and
        // then update all of the texcoord IDs backwards.
        vector_read_adapter stream(command_buffer);

        std::vector<uint32_t> removedIDs;
        std::vector<ModifiedTexCoord> changedFaces;

        while (true)
        {
            size_t mesh_index;
            serialize_read(stream, mesh_index);
            
            if (mesh_index == (size_t) -1)
                break;

            auto &mesh = data->meshes[mesh_index];
            
            removedIDs.clear();
            serialize_read(stream, removedIDs);

            // skip texcoord data, we don't need it
            stream.skip((sizeof(glm::vec2) + sizeof(bool)) * removedIDs.size());

            changedFaces.clear();
            serialize_read(stream, changedFaces);

            for (auto it = removedIDs.rbegin(); it != removedIDs.rend(); it++)
            {
                mesh.texcoords.erase(mesh.texcoords.begin() + *it);

                for (auto &triangle : changedFaces)
                    for (auto &tc : mesh.triangles[triangle.index].texcoords)
                        if (tc >= *it && tc)
                            tc--;
            }
        }

        ui().editor3D().renderer().markBufferDirty();
    }

	const char *Name() const override
    {
        return "UV Vertices Removed";
    }

	virtual void Read(read_adapter &input) override
    {
        serialize_read(input, command_buffer);
    }

	virtual void Write(write_adapter &output) const override
    {
        serialize_write(output, command_buffer);
    }

    virtual size_t Size() const override { return sizeof(*this) + command_buffer.size(); }

	SET_UNDO_REDO_ID(UndoRedoUVVerticesRemoved)

private:
    std::vector<uint8_t>       command_buffer;

    void InitFromSelected(const ModelData *data, SelectMode mode)
    {
        vector_write_adapter adapter;
        std::unordered_set<uint32_t> removedIDs;
        std::vector<uint32_t> flatRemovedIDs;
        std::vector<ModifiedTexCoord> changedFaces;

        for (auto &mesh : data->meshes)
        {
            size_t meshIndex = (size_t) (&mesh - data->meshes.data());

            if (data->selectedMesh.has_value() && data->selectedMesh != meshIndex)
                continue;

            removedIDs.clear();

            if (mode == SelectMode::Vertex)
            {
                for (auto &tc : mesh.texcoords)
                    if (tc.selected)
                        removedIDs.insert(&tc - mesh.texcoords.data());
            }
            else
            {
                for (auto &tc : mesh.triangles)
                    if (tc.selectedUV)
                        removedIDs.insert({ tc.texcoords[0], tc.texcoords[1], tc.texcoords[2] });
                for (auto &tc : mesh.triangles)
                    if (!tc.selectedUV)
                    {
                        removedIDs.erase(tc.texcoords[0]);
                        removedIDs.erase(tc.texcoords[1]);
                        removedIDs.erase(tc.texcoords[2]);
                    }
            }

            if (removedIDs.empty())
                continue;

            flatRemovedIDs.clear();
            std::copy(removedIDs.begin(), removedIDs.end(), std::back_inserter(flatRemovedIDs));
            std::sort(flatRemovedIDs.begin(), flatRemovedIDs.end());

            for (auto &tri : mesh.triangles)
            {
                if (tri.texcoords[0] >= flatRemovedIDs.front() ||
                    tri.texcoords[1] >= flatRemovedIDs.front() ||
                    tri.texcoords[2] >= flatRemovedIDs.front())
                    changedFaces.emplace_back((uint32_t) (&tri - mesh.triangles.data()), tri.texcoords);
            }

            serialize_write(adapter, meshIndex);
            serialize_write(adapter, flatRemovedIDs);

            for (auto &tc : flatRemovedIDs)
                serialize_write(adapter, mesh.texcoords[tc]);

            serialize_write(adapter, changedFaces);
        }

        serialize_write(adapter, (size_t) -1);

        command_buffer = std::move(adapter.data);
        command_buffer.shrink_to_fit();
    }

    void InitFromCleanup(const ModelData *data)
    {
        vector_write_adapter adapter;
        std::vector<uint32_t> removedIDs;
        std::unordered_set<uint32_t> removableIDs;

        for (auto &mesh : data->meshes)
        {
            size_t meshIndex = (size_t) (&mesh - data->meshes.data());

            if (data->selectedMesh.has_value() && data->selectedMesh != meshIndex)
                continue;

            removableIDs.clear();

            removableIDs.reserve(mesh.texcoords.size());

            for (size_t i = 0; i < mesh.texcoords.size(); i++)
                removableIDs.insert(i);

            for (auto &tc : mesh.triangles)
                for (auto &i : tc.texcoords)
                    removableIDs.erase(i);

            if (removableIDs.empty())
                continue;

            removedIDs.clear();
            removedIDs.reserve(removedIDs.size());

            std::copy(removableIDs.begin(), removableIDs.end(), std::back_inserter(removedIDs));

            std::sort(removedIDs.begin(), removedIDs.end());

            serialize_write(adapter, meshIndex);
            serialize_write(adapter, removedIDs);

            for (size_t i = 0; i < removedIDs.size(); i++)
                serialize_write(adapter, mesh.texcoords[removedIDs[i]]);
        }

        serialize_write(adapter, (size_t) -1);

        command_buffer = std::move(adapter.data);
        command_buffer.shrink_to_fit();
    }

    friend class ModelMutator;
};

REGISTER_UNDO_REDO_ID(UndoRedoUVVerticesRemoved);

void ModelMutator::cleanupUVVertices(bool onlySelected, SelectMode selectMode)
{
    auto state = std::make_unique<UndoRedoUVVerticesRemoved>();

    if (onlySelected)
        state->InitFromSelected(data, selectMode);
    else
        state->InitFromCleanup(data);

    if (!state->command_buffer.empty())
        undo().Push(std::move(state));
}

void ModelMutator::deleteSelectedUVVertices()
{
    undo().BeginCombined();

    {
        auto state = std::make_unique<UndoRedoUVFacesRemoved>();

        state->InitFromTriangles(data, [](const ModelMesh &mesh, const ModelTriangle &tri) {
            return mesh.texcoords[tri.texcoords[0]].selected ||
                   mesh.texcoords[tri.texcoords[1]].selected ||
                   mesh.texcoords[tri.texcoords[2]].selected;
        });

        if (!state->command_buffer.empty())
        {
            undo().Push(std::move(state));
        }
    }

    cleanupUVVertices(true, SelectMode::Vertex);

    undo().EndCombined();
}

void ModelMutator::cleanUVVertices()
{
    cleanupUVVertices(false, SelectMode::Vertex);
    undo().List().back()->Redo(data);
}

void ModelMutator::deleteSelectedUVFaces(bool alsoDeleteVertices)
{
    undo().BeginCombined();
    
    auto state = std::make_unique<UndoRedoUVFacesRemoved>();

    state->InitFromTriangles(data, [](const ModelMesh &mesh, const ModelTriangle &tri) { return tri.selectedUV; });

    if (!state->command_buffer.empty())
    {
        undo().Push(std::move(state));
    }
    
    if (alsoDeleteVertices)
        cleanupUVVertices(true, SelectMode::Face);
    
    undo().EndCombined();
}
#pragma endregion

const std::unordered_set<size_t> &ModelMutator::getSelectedTextureCoordinates(const ModelMesh &mesh, SelectMode mode)
{
    static std::unordered_set<size_t> verticesSelected;
    verticesSelected.clear();

    if (mode == SelectMode::Face)
    {
        for (auto &triangle : mesh.triangles)
        {
            if (triangle.selectedUV)
                for (auto &tc : triangle.texcoords)
                    verticesSelected.insert(tc);
        }
    }
    else
    {
        for (size_t i = 0; i < mesh.texcoords.size(); i++)
            if (mesh.texcoords[i].selected)
                verticesSelected.insert(i);
    }

    return verticesSelected;
}

const std::unordered_set<size_t> &ModelMutator::getSelectedVertices(const ModelMesh &mesh, SelectMode mode)
{
    static std::unordered_set<size_t> verticesSelected;
    verticesSelected.clear();

    if (mode == SelectMode::Face)
    {
        for (auto &triangle : mesh.triangles)
        {
            if (triangle.selectedFace)
                for (auto &tc : triangle.vertices)
                    verticesSelected.insert(tc);
        }
    }
    else
    {
        for (size_t i = 0; i < mesh.vertices.size(); i++)
            if (mesh.vertices[i].selected)
                verticesSelected.insert(i);
    }

    return verticesSelected;
}