#include <fstream>

#include <zstd.h>

#include "ModelLoader.h"
#include "Stream.h"
#include "MDLRenderer.h"
#include "UI.h"
#include "UndoRedo.h"
#include "Log.h"

constexpr int32_t QIM_MAGIC = 'QMOD';
constexpr int32_t QIM_VERSION = 1;

constexpr int32_t QIM_CHUNK_MODEL = 'MODL';
constexpr int32_t QIM_CHUNK_UNDO = 'UNDO';

namespace detail
{
inline int32_t qim_version_i()
{
    static int32_t i = std::ios_base::xalloc();
    return i;
}
} // namespace detail

inline int32_t qim_version(std::ios_base &os)
{
    return static_cast<int32_t>(os.iword(detail::qim_version_i()));
}

inline std::ios_base &set_qim_version(std::ios_base &os, int32_t s)
{
    os.iword(detail::qim_version_i()) = static_cast<int32_t>(s);
    return os;
}

enum qim_flags_e
{
	QIM_FLAG_COMPRESSED = (1 << 0)
};

struct qim_chunk_t
{
	int32_t	id;
	size_t	size;
	int8_t  flags;

	auto stream_data() { return std::tie(id, size, flags); }
};

#define CHECK_ZSTD(x) x
#define CHECK(x)

void WriteQIMChunk(std::ostream &s, bool compressed, int32_t chunk_id, std::function<void(std::ostream &)> write_chunk)
{
	std::streamoff chunk_header_offset = s.tellp();
	qim_chunk_t chunk { chunk_id, 0, compressed ? QIM_FLAG_COMPRESSED : 0 };
	s <= chunk;
	if (!compressed)
		write_chunk(s);
	else
	{
		std::stringstream c;
		write_chunk(c);

		c.seekg(0);

		std::string chunkData = c.str();
		size_t outBufferSize = ZSTD_CStreamOutSize();
		auto outBuffer = std::make_unique<uint8_t[]>(outBufferSize);
		
		ZSTD_CCtx *const cctx = ZSTD_createCCtx();
		CHECK_ZSTD( ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, ZSTD_btultra2) );
		CHECK_ZSTD( ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1) );
		ZSTD_inBuffer input = { chunkData.c_str(), chunkData.size(), 0 };
        int finished;
        do {
            ZSTD_outBuffer output = { outBuffer.get(), outBufferSize, 0 };
            size_t remaining = ZSTD_compressStream2(cctx, &output , &input, ZSTD_e_end);
            CHECK_ZSTD(remaining);
            s.write((const char *) outBuffer.get(), output.pos);
            finished = (remaining == 0);
        } while (!finished);
		ZSTD_freeCCtx(cctx);
	}
	std::streamoff p = s.tellp();
	chunk.size = p - (chunk_header_offset + sizeof(int32_t) + sizeof(size_t));
	s.seekp(chunk_header_offset);
	s <= chunk;
	s.seekp(p);
}

static void SaveQIM(const ModelData &model, const std::filesystem::path &file)
{
	std::ofstream stream(file, std::ios_base::binary | std::ios_base::out);

	if (!stream.good())
		throw std::runtime_error("can't open file for writing");

	stream << endianness<std::endian::little>;

	set_qim_version(stream, QIM_VERSION);
	
	stream <= QIM_MAGIC;
	stream <= QIM_VERSION;

	WriteQIMChunk(stream, true, QIM_CHUNK_MODEL, [&model](std::ostream &stream) {
		stream <= model;
	});

	WriteQIMChunk(stream, true, QIM_CHUNK_UNDO, [](std::ostream &stream) {
		undo().Write(stream);
	});
}

static void LoadQIMChunk(const qim_chunk_t &chunk_header, ModelData &data, std::istream &stream)
{
	if (chunk_header.id == QIM_CHUNK_MODEL)
		stream >= data;
	else if (chunk_header.id == QIM_CHUNK_UNDO)
		undo().Read(stream);
}

std::unique_ptr<ModelData> LoadQIM(const std::filesystem::path &file)
{
	if (!std::filesystem::exists(file))
        throw std::runtime_error("non-existent file");

	std::ifstream stream(file.string(), std::ios_base::binary | std::ios_base::in);

    if (!stream.good())
        throw std::runtime_error("can't open file for reading");

	stream >> endianness<std::endian::little>;

	int32_t v;

	stream >= v;
	stream >= v;

	set_qim_version(stream, v);

	ModelData data;
	
	undo().Clear();

	while (stream)
	{
		qim_chunk_t chunk_header;

		stream >= chunk_header;

		if (!stream)
			break;

		// unknown chunk
		if (chunk_header.id != QIM_CHUNK_MODEL && 
			chunk_header.id != QIM_CHUNK_UNDO)
		{
			stream.seekg(chunk_header.size, std::ios_base::cur);
			continue;
		}

		// valid chunk, decompress if necessary
		if (!(chunk_header.flags & QIM_FLAG_COMPRESSED))
			LoadQIMChunk(chunk_header, data, stream);
		else
		{
			std::stringstream decompressed;
			size_t inBufferSize = ZSTD_DStreamInSize();
			auto inBuffer = std::make_unique<uint8_t[]>(inBufferSize);
			size_t outBufferSize = ZSTD_DStreamOutSize();
			auto outBuffer = std::make_unique<uint8_t[]>(outBufferSize);
			
			ZSTD_DCtx *const dctx = ZSTD_createDCtx();

			size_t remaining = chunk_header.size;

			while ( remaining && stream.read((char *) inBuffer.get(), std::min(remaining, inBufferSize)))
			{
				size_t read = stream.gcount();
				remaining -= read;

				ZSTD_inBuffer input = { inBuffer.get(), read, 0 };
				while (input.pos < input.size) {
					ZSTD_outBuffer output = { outBuffer.get(), outBufferSize, 0 };
					size_t ret = ZSTD_decompressStream(dctx, &output , &input);
					decompressed.write((const char *) outBuffer.get(), output.pos);
				}
			}

			ZSTD_freeDCtx(dctx);

			decompressed.seekg(0);
			LoadQIMChunk(chunk_header, data, decompressed);
		}
	}

	return std::make_unique<ModelData>(std::move(data));
}

// Shared model stuff

constexpr glm::vec3 anorms[] = {
	{-0.525731f, 0.000000f, 0.850651f}, 
	{-0.442863f, 0.238856f, 0.864188f}, 
	{-0.295242f, 0.000000f, 0.955423f}, 
	{-0.309017f, 0.500000f, 0.809017f}, 
	{-0.162460f, 0.262866f, 0.951056f}, 
	{0.000000f, 0.000000f, 1.000000f}, 
	{0.000000f, 0.850651f, 0.525731f}, 
	{-0.147621f, 0.716567f, 0.681718f}, 
	{0.147621f, 0.716567f, 0.681718f}, 
	{0.000000f, 0.525731f, 0.850651f}, 
	{0.309017f, 0.500000f, 0.809017f}, 
	{0.525731f, 0.000000f, 0.850651f}, 
	{0.295242f, 0.000000f, 0.955423f}, 
	{0.442863f, 0.238856f, 0.864188f}, 
	{0.162460f, 0.262866f, 0.951056f}, 
	{-0.681718f, 0.147621f, 0.716567f}, 
	{-0.809017f, 0.309017f, 0.500000f}, 
	{-0.587785f, 0.425325f, 0.688191f}, 
	{-0.850651f, 0.525731f, 0.000000f}, 
	{-0.864188f, 0.442863f, 0.238856f}, 
	{-0.716567f, 0.681718f, 0.147621f}, 
	{-0.688191f, 0.587785f, 0.425325f}, 
	{-0.500000f, 0.809017f, 0.309017f}, 
	{-0.238856f, 0.864188f, 0.442863f}, 
	{-0.425325f, 0.688191f, 0.587785f}, 
	{-0.716567f, 0.681718f, -0.147621f}, 
	{-0.500000f, 0.809017f, -0.309017f}, 
	{-0.525731f, 0.850651f, 0.000000f}, 
	{0.000000f, 0.850651f, -0.525731f}, 
	{-0.238856f, 0.864188f, -0.442863f}, 
	{0.000000f, 0.955423f, -0.295242f}, 
	{-0.262866f, 0.951056f, -0.162460f}, 
	{0.000000f, 1.000000f, 0.000000f}, 
	{0.000000f, 0.955423f, 0.295242f}, 
	{-0.262866f, 0.951056f, 0.162460f}, 
	{0.238856f, 0.864188f, 0.442863f}, 
	{0.262866f, 0.951056f, 0.162460f}, 
	{0.500000f, 0.809017f, 0.309017f}, 
	{0.238856f, 0.864188f, -0.442863f}, 
	{0.262866f, 0.951056f, -0.162460f}, 
	{0.500000f, 0.809017f, -0.309017f}, 
	{0.850651f, 0.525731f, 0.000000f}, 
	{0.716567f, 0.681718f, 0.147621f}, 
	{0.716567f, 0.681718f, -0.147621f}, 
	{0.525731f, 0.850651f, 0.000000f}, 
	{0.425325f, 0.688191f, 0.587785f}, 
	{0.864188f, 0.442863f, 0.238856f}, 
	{0.688191f, 0.587785f, 0.425325f}, 
	{0.809017f, 0.309017f, 0.500000f}, 
	{0.681718f, 0.147621f, 0.716567f}, 
	{0.587785f, 0.425325f, 0.688191f}, 
	{0.955423f, 0.295242f, 0.000000f}, 
	{1.000000f, 0.000000f, 0.000000f}, 
	{0.951056f, 0.162460f, 0.262866f}, 
	{0.850651f, -0.525731f, 0.000000f}, 
	{0.955423f, -0.295242f, 0.000000f}, 
	{0.864188f, -0.442863f, 0.238856f}, 
	{0.951056f, -0.162460f, 0.262866f}, 
	{0.809017f, -0.309017f, 0.500000f}, 
	{0.681718f, -0.147621f, 0.716567f}, 
	{0.850651f, 0.000000f, 0.525731f}, 
	{0.864188f, 0.442863f, -0.238856f}, 
	{0.809017f, 0.309017f, -0.500000f}, 
	{0.951056f, 0.162460f, -0.262866f}, 
	{0.525731f, 0.000000f, -0.850651f}, 
	{0.681718f, 0.147621f, -0.716567f}, 
	{0.681718f, -0.147621f, -0.716567f}, 
	{0.850651f, 0.000000f, -0.525731f}, 
	{0.809017f, -0.309017f, -0.500000f}, 
	{0.864188f, -0.442863f, -0.238856f}, 
	{0.951056f, -0.162460f, -0.262866f}, 
	{0.147621f, 0.716567f, -0.681718f}, 
	{0.309017f, 0.500000f, -0.809017f}, 
	{0.425325f, 0.688191f, -0.587785f}, 
	{0.442863f, 0.238856f, -0.864188f}, 
	{0.587785f, 0.425325f, -0.688191f}, 
	{0.688191f, 0.587785f, -0.425325f}, 
	{-0.147621f, 0.716567f, -0.681718f}, 
	{-0.309017f, 0.500000f, -0.809017f}, 
	{0.000000f, 0.525731f, -0.850651f}, 
	{-0.525731f, 0.000000f, -0.850651f}, 
	{-0.442863f, 0.238856f, -0.864188f}, 
	{-0.295242f, 0.000000f, -0.955423f}, 
	{-0.162460f, 0.262866f, -0.951056f}, 
	{0.000000f, 0.000000f, -1.000000f}, 
	{0.295242f, 0.000000f, -0.955423f}, 
	{0.162460f, 0.262866f, -0.951056f}, 
	{-0.442863f, -0.238856f, -0.864188f}, 
	{-0.309017f, -0.500000f, -0.809017f}, 
	{-0.162460f, -0.262866f, -0.951056f}, 
	{0.000000f, -0.850651f, -0.525731f}, 
	{-0.147621f, -0.716567f, -0.681718f}, 
	{0.147621f, -0.716567f, -0.681718f}, 
	{0.000000f, -0.525731f, -0.850651f}, 
	{0.309017f, -0.500000f, -0.809017f}, 
	{0.442863f, -0.238856f, -0.864188f}, 
	{0.162460f, -0.262866f, -0.951056f}, 
	{0.238856f, -0.864188f, -0.442863f}, 
	{0.500000f, -0.809017f, -0.309017f}, 
	{0.425325f, -0.688191f, -0.587785f}, 
	{0.716567f, -0.681718f, -0.147621f}, 
	{0.688191f, -0.587785f, -0.425325f}, 
	{0.587785f, -0.425325f, -0.688191f}, 
	{0.000000f, -0.955423f, -0.295242f}, 
	{0.000000f, -1.000000f, 0.000000f}, 
	{0.262866f, -0.951056f, -0.162460f}, 
	{0.000000f, -0.850651f, 0.525731f}, 
	{0.000000f, -0.955423f, 0.295242f}, 
	{0.238856f, -0.864188f, 0.442863f}, 
	{0.262866f, -0.951056f, 0.162460f}, 
	{0.500000f, -0.809017f, 0.309017f}, 
	{0.716567f, -0.681718f, 0.147621f}, 
	{0.525731f, -0.850651f, 0.000000f}, 
	{-0.238856f, -0.864188f, -0.442863f}, 
	{-0.500000f, -0.809017f, -0.309017f}, 
	{-0.262866f, -0.951056f, -0.162460f}, 
	{-0.850651f, -0.525731f, 0.000000f}, 
	{-0.716567f, -0.681718f, -0.147621f}, 
	{-0.716567f, -0.681718f, 0.147621f}, 
	{-0.525731f, -0.850651f, 0.000000f}, 
	{-0.500000f, -0.809017f, 0.309017f}, 
	{-0.238856f, -0.864188f, 0.442863f}, 
	{-0.262866f, -0.951056f, 0.162460f}, 
	{-0.864188f, -0.442863f, 0.238856f}, 
	{-0.809017f, -0.309017f, 0.500000f}, 
	{-0.688191f, -0.587785f, 0.425325f}, 
	{-0.681718f, -0.147621f, 0.716567f}, 
	{-0.442863f, -0.238856f, 0.864188f}, 
	{-0.587785f, -0.425325f, 0.688191f}, 
	{-0.309017f, -0.500000f, 0.809017f}, 
	{-0.147621f, -0.716567f, 0.681718f}, 
	{-0.425325f, -0.688191f, 0.587785f}, 
	{-0.162460f, -0.262866f, 0.951056f}, 
	{0.442863f, -0.238856f, 0.864188f}, 
	{0.162460f, -0.262866f, 0.951056f}, 
	{0.309017f, -0.500000f, 0.809017f}, 
	{0.147621f, -0.716567f, 0.681718f}, 
	{0.000000f, -0.525731f, 0.850651f}, 
	{0.425325f, -0.688191f, 0.587785f}, 
	{0.587785f, -0.425325f, 0.688191f}, 
	{0.688191f, -0.587785f, 0.425325f}, 
	{-0.955423f, 0.295242f, 0.000000f}, 
	{-0.951056f, 0.162460f, 0.262866f}, 
	{-1.000000f, 0.000000f, 0.000000f}, 
	{-0.850651f, 0.000000f, 0.525731f}, 
	{-0.955423f, -0.295242f, 0.000000f}, 
	{-0.951056f, -0.162460f, 0.262866f}, 
	{-0.864188f, 0.442863f, -0.238856f}, 
	{-0.951056f, 0.162460f, -0.262866f}, 
	{-0.809017f, 0.309017f, -0.500000f}, 
	{-0.864188f, -0.442863f, -0.238856f}, 
	{-0.951056f, -0.162460f, -0.262866f}, 
	{-0.809017f, -0.309017f, -0.500000f}, 
	{-0.681718f, 0.147621f, -0.716567f}, 
	{-0.681718f, -0.147621f, -0.716567f}, 
	{-0.850651f, 0.000000f, -0.525731f}, 
	{-0.688191f, 0.587785f, -0.425325f}, 
	{-0.587785f, 0.425325f, -0.688191f}, 
	{-0.425325f, 0.688191f, -0.587785f}, 
	{-0.425325f, -0.688191f, -0.587785f}, 
	{-0.587785f, -0.425325f, -0.688191f}, 
	{-0.688191f, -0.587785f, -0.425325f}
};

constexpr uint8_t CompressNormal(const glm::vec3 &v)
{
	float bestdot = 0;
	const glm::vec3 *bestnorm = nullptr;

	for (auto &norm : anorms)
	{
		float dot = glm::dot(v, norm);

		if (bestnorm && dot <= bestdot)
			continue;

		bestdot = dot;
		bestnorm = &norm;
	}

	if (!bestnorm)
		throw std::runtime_error("fatal");

	return bestnorm - anorms;
}

/*
========================================================================

.MD2 triangle model file format

========================================================================
*/
constexpr int32_t MD2_MAGIC = (('2'<<24)+('P'<<16)+('D'<<8)+'I');
constexpr int32_t MD2_VERSION = 8;

constexpr size_t MD2_MAX_SKINNAME = 64;
constexpr size_t MD2_MAX_FRAMENAME = 16;

struct dstvert_t
{
	int16_t	s;
	int16_t	t;

	auto stream_data()
	{
		return std::tie(s, t);
	}
};

struct dtriangle_t
{
	std::array<int16_t, 3> index_xyz;
	std::array<int16_t, 3> index_st;

	auto stream_data()
	{
		return std::tie(index_xyz, index_st);
	}
};

struct dtrivertx_t
{
	std::array<uint8_t, 3> v;			// scaled byte to fit in frame mins/maxs
	uint8_t				   lightnormalindex;

	auto stream_data()
	{
		return std::tie(v, lightnormalindex);
	}
};

struct daliasframe_t
{
	glm::vec3                    scale;	// multiply byte verts by this
	glm::vec3                    translate;	// then add this
	cstring_t<MD2_MAX_FRAMENAME> name;	// frame name from grabbing

	auto stream_data()
	{
		return std::tie(scale, translate, name);
	}
};

struct dmdl_t
{
	int32_t	ident;
	int32_t	version;

	int32_t	skinwidth;
	int32_t	skinheight;
	int32_t	framesize;		// byte size of each frame

	int32_t	num_skins;
	int32_t	num_xyz;
	int32_t	num_st;			// greater than num_xyz for seams
	int32_t	num_tris;
	int32_t	num_glcmds;		// dwords in strip/fan command list
	int32_t	num_frames;

	int32_t	ofs_skins;		// each skin is a MAX_SKINNAME string
	int32_t	ofs_st;			// byte offset from start for stverts
	int32_t	ofs_tris;		// offset for dtriangles
	int32_t	ofs_frames;		// offset for first frame
	int32_t	ofs_glcmds;	
	int32_t	ofs_end;		// end of file

	auto stream_data()
	{
		return std::tie(ident, version,
			skinwidth, skinheight, framesize,
			num_skins, num_xyz, num_st, num_tris, num_glcmds, num_frames,
			ofs_skins, ofs_st, ofs_tris, ofs_frames, ofs_glcmds, ofs_end);
	}
};

static std::unique_ptr<ModelData> LoadMD2(const std::filesystem::path &file)
{
	if (!std::filesystem::exists(file))
        throw std::runtime_error("non-existent file");

	std::ifstream stream(file, std::ios_base::binary | std::ios_base::in);

    if (!stream.good())
        throw std::runtime_error("can't open file for reading");

	stream >> endianness<std::endian::little>;

	dmdl_t header;
	stream >= header;

	ModelData data;

	data.frames.resize(header.num_frames);

	auto &mesh = data.meshes.emplace_back();

	data.frames.resize(header.num_frames);
	mesh.frames.resize(header.num_frames);

	for (auto &frame : mesh.frames)
		frame.vertices.resize(header.num_xyz);

	mesh.vertices.resize(header.num_xyz);

	stream.seekg(header.ofs_frames);

	for (size_t i = 0; i < data.frames.size(); i++)
	{
		auto &modelframe = data.frames[i];
		auto &meshframe = mesh.frames[i];

		daliasframe_t frame_header;
		stream >= frame_header;

		modelframe.name = frame_header.name.c_str();

		for (auto &vert : meshframe.vertices)
		{
			dtrivertx_t v;
			stream >= v;

			vert = { MeshFrameVertex { glm::vec3 {
					(v.v[0] * frame_header.scale[0]) + frame_header.translate[0],
					(v.v[1] * frame_header.scale[1]) + frame_header.translate[1],
					(v.v[2] * frame_header.scale[2]) + frame_header.translate[2]
				},
				anorms[v.lightnormalindex]
			} };
		}
	}

	stream.seekg(header.ofs_st);
	mesh.texcoords.resize(header.num_st);

	for (auto &st : mesh.texcoords)
	{
		dstvert_t v;
		stream >= v;
		st.pos = { (float) v.s / header.skinwidth, (float) v.t / header.skinheight };
	}

	stream.seekg(header.ofs_tris);
	mesh.triangles.resize(header.num_tris);

	for (auto &tri : mesh.triangles)
	{
		dtriangle_t t;
		stream >= t;
		
		std::copy(t.index_xyz.begin(), t.index_xyz.end(), tri.vertices.begin());
		std::copy(t.index_st.begin(), t.index_st.end(), tri.texcoords.begin());
	}

	data.skins.resize(header.num_skins);
	
	stream.seekg(header.ofs_skins);

	std::filesystem::path model_dir = file;
	model_dir.remove_filename();

	for (auto &skin : data.skins)
	{
		cstring_t<MD2_MAX_SKINNAME> skin_path;
		stream >= skin_path;
		skin.name = skin_path.c_str();
		
		// try to find the matching image file, convert to rgba
		if (auto skin_file = images().ResolveSkinFile(model_dir, skin_path.c_str(), { "pcx", "tga", "png" }))
		{
			auto image = images().Load(skin_file.value());

			if (!image.is_valid())
				continue;

			skin.image = std::move(image);
			skin.width = skin.image.width;
			skin.height = skin.image.height;
		}
		else
		{
			skin.image = Image::create_rgba(header.skinwidth, header.skinheight);
			skin.width = header.skinwidth;
			skin.height = header.skinheight;
		}
	}

	return std::make_unique<ModelData>(std::move(data));
}

static std::unique_ptr<ModelData> LoadMD2F(const std::filesystem::path &file) { return nullptr; }

// Quake 1 MDL

constexpr int32_t ALIAS_VERSION	= 6;

constexpr int32_t ALIAS_ONSEAM = 0x0020;

// must match definition in spritegn.h
enum synctype_t
{
	ST_SYNC = 0,
	ST_RAND
};

enum aliasgrouptype_t
{
	ALIAS_SINGLE = 0,
	ALIAS_GROUP
};

using vec3_t = glm::vec3;

struct mdl_t
{
	int32_t		ident;
	int32_t		version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int32_t		numskins;
	int32_t		skinwidth;
	int32_t		skinheight;
	int32_t		numverts;
	int32_t		numtris;
	int32_t		numframes;
	synctype_t	synctype;
	int32_t		flags;
	float		size;
	
	auto stream_data()
	{
		return std::tie(ident, version, scale, scale_origin, boundingradius,
			eyeposition, numskins, skinwidth, skinheight, numverts, numtris,
			numframes, (int32_t &) synctype, flags, size);
	}
};

struct stvert_t
{
	int32_t onseam;
	int32_t s;
	int32_t t;
	
	auto stream_data()
	{
		return std::tie(onseam, s, t);
	}
};

struct dmdltriangle_t
{
	int32_t                facesfront;
	std::array<int32_t, 3> vertindex;
	
	auto stream_data()
	{
		return std::tie(facesfront, vertindex);
	}
};

constexpr int32_t DT_FACES_FRONT				= 0x0010;

using trivertx_t = dtrivertx_t;

struct dmdlaliasframe_t
{
	trivertx_t	                 bboxmin;
	trivertx_t	                 bboxmax;
	cstring_t<MD2_MAX_FRAMENAME> name;
	
	auto stream_data()
	{
		return std::tie(bboxmin, bboxmax, name);
	}
};

struct daliasgroup_t
{
	int			numframes;
	trivertx_t	bboxmin;	// lightnormal isn't used
	trivertx_t	bboxmax;	// lightnormal isn't used
	
	auto stream_data()
	{
		return std::tie(numframes, bboxmin, bboxmax);
	}
};

constexpr int32_t IDPOLYHEADER	= (('O'<<24)+('P'<<16)+('D'<<8)+'I');

constexpr uint8_t quakePalette[768] = {
	0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x1F, 0x1F, 0x1F, 0x2F, 0x2F, 0x2F,
	0x3F, 0x3F, 0x3F, 0x4B, 0x4B, 0x4B, 0x5B, 0x5B, 0x5B, 0x6B, 0x6B, 0x6B,
	0x7B, 0x7B, 0x7B, 0x8B, 0x8B, 0x8B, 0x9B, 0x9B, 0x9B, 0xAB, 0xAB, 0xAB,
	0xBB, 0xBB, 0xBB, 0xCB, 0xCB, 0xCB, 0xDB, 0xDB, 0xDB, 0xEB, 0xEB, 0xEB,
	0x0F, 0x0B, 0x07, 0x17, 0x0F, 0x0B, 0x1F, 0x17, 0x0B, 0x27, 0x1B, 0x0F,
	0x2F, 0x23, 0x13, 0x37, 0x2B, 0x17, 0x3F, 0x2F, 0x17, 0x4B, 0x37, 0x1B,
	0x53, 0x3B, 0x1B, 0x5B, 0x43, 0x1F, 0x63, 0x4B, 0x1F, 0x6B, 0x53, 0x1F,
	0x73, 0x57, 0x1F, 0x7B, 0x5F, 0x23, 0x83, 0x67, 0x23, 0x8F, 0x6F, 0x23,
	0x0B, 0x0B, 0x0F, 0x13, 0x13, 0x1B, 0x1B, 0x1B, 0x27, 0x27, 0x27, 0x33,
	0x2F, 0x2F, 0x3F, 0x37, 0x37, 0x4B, 0x3F, 0x3F, 0x57, 0x47, 0x47, 0x67,
	0x4F, 0x4F, 0x73, 0x5B, 0x5B, 0x7F, 0x63, 0x63, 0x8B, 0x6B, 0x6B, 0x97,
	0x73, 0x73, 0xA3, 0x7B, 0x7B, 0xAF, 0x83, 0x83, 0xBB, 0x8B, 0x8B, 0xCB,
	0x00, 0x00, 0x00, 0x07, 0x07, 0x00, 0x0B, 0x0B, 0x00, 0x13, 0x13, 0x00,
	0x1B, 0x1B, 0x00, 0x23, 0x23, 0x00, 0x2B, 0x2B, 0x07, 0x2F, 0x2F, 0x07,
	0x37, 0x37, 0x07, 0x3F, 0x3F, 0x07, 0x47, 0x47, 0x07, 0x4B, 0x4B, 0x0B,
	0x53, 0x53, 0x0B, 0x5B, 0x5B, 0x0B, 0x63, 0x63, 0x0B, 0x6B, 0x6B, 0x0F,
	0x07, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x17, 0x00, 0x00, 0x1F, 0x00, 0x00,
	0x27, 0x00, 0x00, 0x2F, 0x00, 0x00, 0x37, 0x00, 0x00, 0x3F, 0x00, 0x00,
	0x47, 0x00, 0x00, 0x4F, 0x00, 0x00, 0x57, 0x00, 0x00, 0x5F, 0x00, 0x00,
	0x67, 0x00, 0x00, 0x6F, 0x00, 0x00, 0x77, 0x00, 0x00, 0x7F, 0x00, 0x00,
	0x13, 0x13, 0x00, 0x1B, 0x1B, 0x00, 0x23, 0x23, 0x00, 0x2F, 0x2B, 0x00,
	0x37, 0x2F, 0x00, 0x43, 0x37, 0x00, 0x4B, 0x3B, 0x07, 0x57, 0x43, 0x07,
	0x5F, 0x47, 0x07, 0x6B, 0x4B, 0x0B, 0x77, 0x53, 0x0F, 0x83, 0x57, 0x13,
	0x8B, 0x5B, 0x13, 0x97, 0x5F, 0x1B, 0xA3, 0x63, 0x1F, 0xAF, 0x67, 0x23,
	0x23, 0x13, 0x07, 0x2F, 0x17, 0x0B, 0x3B, 0x1F, 0x0F, 0x4B, 0x23, 0x13,
	0x57, 0x2B, 0x17, 0x63, 0x2F, 0x1F, 0x73, 0x37, 0x23, 0x7F, 0x3B, 0x2B,
	0x8F, 0x43, 0x33, 0x9F, 0x4F, 0x33, 0xAF, 0x63, 0x2F, 0xBF, 0x77, 0x2F,
	0xCF, 0x8F, 0x2B, 0xDF, 0xAB, 0x27, 0xEF, 0xCB, 0x1F, 0xFF, 0xF3, 0x1B,
	0x0B, 0x07, 0x00, 0x1B, 0x13, 0x00, 0x2B, 0x23, 0x0F, 0x37, 0x2B, 0x13,
	0x47, 0x33, 0x1B, 0x53, 0x37, 0x23, 0x63, 0x3F, 0x2B, 0x6F, 0x47, 0x33,
	0x7F, 0x53, 0x3F, 0x8B, 0x5F, 0x47, 0x9B, 0x6B, 0x53, 0xA7, 0x7B, 0x5F,
	0xB7, 0x87, 0x6B, 0xC3, 0x93, 0x7B, 0xD3, 0xA3, 0x8B, 0xE3, 0xB3, 0x97,
	0xAB, 0x8B, 0xA3, 0x9F, 0x7F, 0x97, 0x93, 0x73, 0x87, 0x8B, 0x67, 0x7B,
	0x7F, 0x5B, 0x6F, 0x77, 0x53, 0x63, 0x6B, 0x4B, 0x57, 0x5F, 0x3F, 0x4B,
	0x57, 0x37, 0x43, 0x4B, 0x2F, 0x37, 0x43, 0x27, 0x2F, 0x37, 0x1F, 0x23,
	0x2B, 0x17, 0x1B, 0x23, 0x13, 0x13, 0x17, 0x0B, 0x0B, 0x0F, 0x07, 0x07,
	0xBB, 0x73, 0x9F, 0xAF, 0x6B, 0x8F, 0xA3, 0x5F, 0x83, 0x97, 0x57, 0x77,
	0x8B, 0x4F, 0x6B, 0x7F, 0x4B, 0x5F, 0x73, 0x43, 0x53, 0x6B, 0x3B, 0x4B,
	0x5F, 0x33, 0x3F, 0x53, 0x2B, 0x37, 0x47, 0x23, 0x2B, 0x3B, 0x1F, 0x23,
	0x2F, 0x17, 0x1B, 0x23, 0x13, 0x13, 0x17, 0x0B, 0x0B, 0x0F, 0x07, 0x07,
	0xDB, 0xC3, 0xBB, 0xCB, 0xB3, 0xA7, 0xBF, 0xA3, 0x9B, 0xAF, 0x97, 0x8B,
	0xA3, 0x87, 0x7B, 0x97, 0x7B, 0x6F, 0x87, 0x6F, 0x5F, 0x7B, 0x63, 0x53,
	0x6B, 0x57, 0x47, 0x5F, 0x4B, 0x3B, 0x53, 0x3F, 0x33, 0x43, 0x33, 0x27,
	0x37, 0x2B, 0x1F, 0x27, 0x1F, 0x17, 0x1B, 0x13, 0x0F, 0x0F, 0x0B, 0x07,
	0x6F, 0x83, 0x7B, 0x67, 0x7B, 0x6F, 0x5F, 0x73, 0x67, 0x57, 0x6B, 0x5F,
	0x4F, 0x63, 0x57, 0x47, 0x5B, 0x4F, 0x3F, 0x53, 0x47, 0x37, 0x4B, 0x3F,
	0x2F, 0x43, 0x37, 0x2B, 0x3B, 0x2F, 0x23, 0x33, 0x27, 0x1F, 0x2B, 0x1F,
	0x17, 0x23, 0x17, 0x0F, 0x1B, 0x13, 0x0B, 0x13, 0x0B, 0x07, 0x0B, 0x07,
	0xFF, 0xF3, 0x1B, 0xEF, 0xDF, 0x17, 0xDB, 0xCB, 0x13, 0xCB, 0xB7, 0x0F,
	0xBB, 0xA7, 0x0F, 0xAB, 0x97, 0x0B, 0x9B, 0x83, 0x07, 0x8B, 0x73, 0x07,
	0x7B, 0x63, 0x07, 0x6B, 0x53, 0x00, 0x5B, 0x47, 0x00, 0x4B, 0x37, 0x00,
	0x3B, 0x2B, 0x00, 0x2B, 0x1F, 0x00, 0x1B, 0x0F, 0x00, 0x0B, 0x07, 0x00,
	0x00, 0x00, 0xFF, 0x0B, 0x0B, 0xEF, 0x13, 0x13, 0xDF, 0x1B, 0x1B, 0xCF,
	0x23, 0x23, 0xBF, 0x2B, 0x2B, 0xAF, 0x2F, 0x2F, 0x9F, 0x2F, 0x2F, 0x8F,
	0x2F, 0x2F, 0x7F, 0x2F, 0x2F, 0x6F, 0x2F, 0x2F, 0x5F, 0x2B, 0x2B, 0x4F,
	0x23, 0x23, 0x3F, 0x1B, 0x1B, 0x2F, 0x13, 0x13, 0x1F, 0x0B, 0x0B, 0x0F,
	0x2B, 0x00, 0x00, 0x3B, 0x00, 0x00, 0x4B, 0x07, 0x00, 0x5F, 0x07, 0x00,
	0x6F, 0x0F, 0x00, 0x7F, 0x17, 0x07, 0x93, 0x1F, 0x07, 0xA3, 0x27, 0x0B,
	0xB7, 0x33, 0x0F, 0xC3, 0x4B, 0x1B, 0xCF, 0x63, 0x2B, 0xDB, 0x7F, 0x3B,
	0xE3, 0x97, 0x4F, 0xE7, 0xAB, 0x5F, 0xEF, 0xBF, 0x77, 0xF7, 0xD3, 0x8B,
	0xA7, 0x7B, 0x3B, 0xB7, 0x9B, 0x37, 0xC7, 0xC3, 0x37, 0xE7, 0xE3, 0x57,
	0x7F, 0xBF, 0xFF, 0xAB, 0xE7, 0xFF, 0xD7, 0xFF, 0xFF, 0x67, 0x00, 0x00,
	0x8B, 0x00, 0x00, 0xB3, 0x00, 0x00, 0xD7, 0x00, 0x00, 0xFF, 0x00, 0x00,
	0xFF, 0xF3, 0x93, 0xFF, 0xF7, 0xC7, 0xFF, 0xFF, 0xFF, 0x9F, 0x5B, 0x53
};

static std::unique_ptr<ModelData> LoadMDL(const std::filesystem::path &file)
{
	if (!std::filesystem::exists(file))
        throw std::runtime_error("non-existent file");

	std::ifstream stream(file, std::ios_base::binary | std::ios_base::in);

    if (!stream.good())
        throw std::runtime_error("can't open file for reading");

	stream >> endianness<std::endian::little>;

	mdl_t header;
	stream >= header;

	ModelData data;

	auto &mesh = data.meshes.emplace_back();

	int32_t group_id = 0;

	auto parseSingleSkin = [&stream, &header](ModelSkin &skin) {
		skin.name = "Skin";
		skin.width = header.skinwidth;
		skin.height = header.skinheight;
		
		auto &image = skin.image;
		image.width = skin.width;
		image.height = skin.height;
		image.source.data.resize(skin.width * skin.height);
		image.source.palette.resize(256 * 3);
		memcpy(image.palette(), quakePalette, image.palette_size());
		stream.read(reinterpret_cast<char *>(image.indexed()), image.indexed_size());
	};

	// skins may grow with groups
	data.skins.reserve(header.numskins);

	for (int i = 0; i < header.numskins; i++)
	{
		aliasgrouptype_t type;
		stream >= (int32_t &) type;

		if (type == ALIAS_SINGLE)
		{
			auto &outSkin = data.skins.emplace_back();
			parseSingleSkin(outSkin);
		}
		else
		{
			int32_t numskins;
			stream >= numskins;

			size_t frame_start = data.skins.size();
			
			for (int f = 0; f < numskins; f++)
			{
				auto &outskin = data.skins.emplace_back();

				float interval;
				stream >= interval;

				outskin.q1_data = { group_id, interval };
			}

			group_id++;
			
			for (int f = 0; f < numskins; f++)
			{
				auto &outskin = data.skins[frame_start + f];
				parseSingleSkin(outskin);
			}
		}
	}

	std::vector<stvert_t> stverts;
	stverts.resize(header.numverts);
	
	mesh.texcoords.resize(header.numverts);
	mesh.vertices.resize(header.numverts);

	for (int i = 0; i < header.numverts; i++)
	{
		auto &st = stverts[i];
		stream >= st;

		auto &v = mesh.texcoords[i];
		v.pos = { (float) st.s / header.skinwidth, (float) st.t / header.skinheight };
	}

	mesh.triangles.resize(header.numtris);

	for (int i = 0; i < header.numtris; i++)
	{
		dmdltriangle_t t;
		stream >= t;

		auto &tri = mesh.triangles[i];
		tri.vertices = { (uint32_t) t.vertindex[0], (uint32_t) t.vertindex[1], (uint32_t) t.vertindex[2] };

		if (!t.facesfront)
		{
			for (int x = 0; x < 3; x++)
			{
				if (stverts[tri.vertices[x]].onseam)
				{
					tri.texcoords[x] = mesh.texcoords.size();
					mesh.texcoords.emplace_back(
						glm::vec2 { ((float) stverts[tri.vertices[x]].s / header.skinwidth) + 0.5f, (float) stverts[tri.vertices[x]].t / header.skinheight }
					);
				}
				else
					tri.texcoords[x] = tri.vertices[x];
			}
		}
		else
			tri.texcoords = tri.vertices;
	}

	// framegroups can add more, technically
	data.frames.reserve(header.numframes);

	group_id = 0;

	auto parseSingleFrame = [&stream, &header](ModelFrame &outframe, MeshFrame &meshframe) {
		dmdlaliasframe_t frame;
		stream >= frame;

		outframe.name = frame.name.c_str();
		meshframe.vertices.resize(header.numverts);

		for (int x = 0; x < header.numverts; x++)
		{
			dtrivertx_t v;
			stream >= v;

			auto &vert = meshframe.vertices[x];

			vert = { MeshFrameVertex { glm::vec3 {
					(v.v[0] * header.scale[0]) + header.scale_origin[0],
					(v.v[1] * header.scale[1]) + header.scale_origin[1],
					(v.v[2] * header.scale[2]) + header.scale_origin[2]
				},
				anorms[v.lightnormalindex]
			} };
		}
	};

	for (int i = 0; i < header.numframes; i++)
	{
		aliasgrouptype_t type;
		stream >= (int32_t &) type;

		if (type == ALIAS_SINGLE)
		{
			auto &outframe = data.frames.emplace_back();
			auto &meshframe = mesh.frames.emplace_back();
			parseSingleFrame(outframe, meshframe);
		}
		else
		{
			daliasgroup_t group;
			stream >= group;

			size_t frame_start = data.frames.size();
			
			for (int f = 0; f < group.numframes; f++)
			{
				auto &outframe = data.frames.emplace_back();
				auto &meshframe = mesh.frames.emplace_back();

				float interval;
				stream >= interval;

				outframe.q1_data = { group_id, interval };
			}

			group_id++;
			
			for (int f = 0; f < group.numframes; f++)
			{
				auto &outframe = data.frames[frame_start + f];
				auto &meshframe = mesh.frames[frame_start + f];
				parseSingleFrame(outframe, meshframe);
			}
		}
	}
	
	return std::make_unique<ModelData>(std::move(data));
}

static std::unique_ptr<ModelData> LoadMD3(const std::filesystem::path &file) { return nullptr; }

bool ModelLoader::Load(const std::filesystem::path &file)
{
	std::unique_ptr<ModelData> model;

	// TODO: clear state here (as if File < New was set)

	// TODO: detect via header
	if (file.extension() == ".md2")
	    model = LoadMD2(file);
	else if (file.extension() == ".md2f")
	    model = LoadMD2F(file);
	else if (file.extension() == ".mdl")
	    model = LoadMDL(file);
	else if (file.extension() == ".qim")
	    model = LoadQIM(file);
	else if (file.extension() == ".md3")
	    model = LoadMD3(file);
	else
		return false;

	if (file.extension() != ".qim")
	{
		undo().Clear();

		undo().BeginDisabled();

		// post-load operations
		ModelMutator mutator{model.get()};

		if (!model->skins.empty())
			mutator.setSelectedSkin(0);

		undo().EndDisabled();
	}

	_model.swap(model);

	// tell the renderer
	ui().editor3D().renderer().modelLoaded();

	return true;
}

static void SaveMD2(const ModelData &model, const std::filesystem::path &file) { }

void ModelLoader::Save(const std::filesystem::path &file) const
{
	if (file.extension() == ".md2")
		SaveMD2(model(), file);
	else if (file.extension() == ".qim")
		SaveQIM(model(), file);
	else
		throw std::runtime_error("invalid file type");
}

const ModelData &ModelLoader::model() const
{
	return *_model.get();
}

ModelMutator ModelLoader::mutator()
{
	return ModelMutator { _model.get() ? _model.get() : nullptr };
}

ModelLoader &model()
{
	static ModelLoader loader;
	return loader;
}