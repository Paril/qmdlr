#pragma once

#include <QVector2D>
#include <QVector3D>
#include <array>
#include <vector>
#include <string>

struct ModelFrameVertex
{
    QVector3D   position;
    QVector3D   normal;
};

struct ModelTriangle
{
	std::array<uint32_t, 3>		vertices;
	std::array<uint32_t, 3>		texcoords;
    bool                        selected = false;
};

struct ModelFrame
{
    std::string					    name;
    std::vector<ModelFrameVertex>	vertices;
};

struct ModelSkin
{
    std::vector<uint8_t>        data;
    int                         width, height;
};

struct ModelVertex
{
    bool                        selected;
};

struct ModelData
{
    std::vector<ModelFrame>     frames;
    std::vector<QVector2D>		texcoords;
    std::vector<ModelTriangle>  triangles;
    std::vector<ModelSkin>      skins;
    std::vector<ModelVertex>    vertices;
};