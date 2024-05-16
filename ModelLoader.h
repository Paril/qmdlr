#pragma once

#include <filesystem>

#include "ModelData.h"
#include "ModelMutator.h"

class ModelLoader
{
public:
	bool Load(const std::filesystem::path &file);
	void Save(const std::filesystem::path &file) const;

	const ModelData &model() const;
	ModelMutator mutator();

private:
	std::unique_ptr<ModelData> _model = std::make_unique<ModelData>(ModelData::blankModel());
};

ModelLoader &model();