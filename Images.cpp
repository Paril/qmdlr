#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_TGA
#include <stb_image/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

#include <fstream>
#include "Images.h"
#include "ModelData.h"

void Image::stream_write(std::ostream &stream) const
{
    if (!width)
    {
        stream <= false;
        return;
    }
    
    stream <= true;
    stream <= width <= height;

    if (!rgba_size())
    {
        stream <= false;
    }
    else
    {
        stream <= true;
        stream.write(reinterpret_cast<const char *>(rgba()), rgba_size());
    }

    if (!indexed_size())
    {
        stream <= false;
    }
    else
    {
        stream <= true;
        stream.write(reinterpret_cast<const char *>(indexed()), indexed_size());
        stream.write(reinterpret_cast<const char *>(palette()), palette_size());
    }
}

void Image::stream_read(std::istream &stream)
{
    bool valid;
    stream >= valid;

    if (!valid)
        return;

    stream >= width >= height;

    bool has_rgba;
    stream >= has_rgba;

    if (has_rgba)
    {
        data.resize(width * height * 4);
        stream.read(reinterpret_cast<char *>(data.data()), rgba_size());
    }

    bool has_indexed;
    stream >= has_indexed;

    if (has_indexed)
    {
        source.data.resize(width * height);
        source.palette.resize(256 * 3);
        stream.read(reinterpret_cast<char *>(source.data.data()), indexed_size());
        stream.read(reinterpret_cast<char *>(source.palette.data()), source.palette.size());
    }
}

// PCX Support
struct pcx_t
{
    int8_t		manufacturer;
    int8_t		version;
    int8_t		encoding;
    int8_t		bits_per_pixel;
    uint16_t	xmin, ymin, xmax, ymax;
    uint16_t	hres, vres;
    padding<48>	palette;
    int8_t		reserved;
    int8_t		color_planes;
    uint16_t	bytes_per_line;
    uint16_t	palette_type;
    padding<58>	filler;

	auto stream_data()
	{
		return std::tie(manufacturer, version, encoding, bits_per_pixel,
			xmin, ymin, xmax, ymax, hres, vres, palette,
			reserved, color_planes, bytes_per_line, palette_type, filler);
	}
};

static Image LoadPCX (const std::filesystem::path &file)
{
	if (!std::filesystem::exists(file))
        throw std::runtime_error("non-existent file");

	std::ifstream stream(file, std::ios_base::binary | std::ios_base::in);

    if (!stream.good())
        throw std::runtime_error("can't open file for reading");

	stream >> endianness<std::endian::little>;

	pcx_t pcx;

	stream >= pcx;

    if (pcx.manufacturer != 0x0a
		|| pcx.version != 5
		|| pcx.encoding != 1
		|| pcx.bits_per_pixel != 8
		|| pcx.xmax >= 640
		|| pcx.ymax >= 480)
        return {};

    Image img = Image::create_indexed(pcx.xmax + 1, pcx.ymax + 1);

	uint8_t *pix = img.indexed();

	for (int y = 0; y <= pcx.ymax; y++, pix += pcx.xmax + 1)
	{
		for (int x = 0; x <= pcx.xmax; )
		{
			uint8_t dataByte;
            int runLength = 1;

            stream >= dataByte;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
                stream >= dataByte;
			}

			while (runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

    uint8_t paletteId = 0;

    stream >= paletteId;

    if (stream && paletteId == 0x0c)
        stream.read(reinterpret_cast<char *>(img.palette()), img.palette_size());

    return img;
}

static void SavePCX (const Image &image, const std::filesystem::path &file)
{
	if (!image.is_indexed_valid())
		throw std::runtime_error("not an indexed image");

	std::ofstream stream(file, std::ios_base::binary | std::ios_base::out);

    if (!stream.good())
        throw std::runtime_error("can't open file for writing");

	stream << endianness<std::endian::little>;

	pcx_t pcx {};

	pcx.manufacturer = 0x0a;	// PCX id
	pcx.version = 5;			// 256 color
 	pcx.encoding = 1;		// uncompressed
	pcx.bits_per_pixel = 8;		// 256 color
	pcx.xmin = 0;
	pcx.ymin = 0;
	pcx.xmax = image.width - 1;
	pcx.ymax = image.height - 1;
	pcx.hres = image.width;
	pcx.vres = image.height;
	pcx.color_planes = 1;		// chunky image
	pcx.bytes_per_line = image.width;
	pcx.palette_type = 2;		// not a grey scale

	stream <= pcx;

	// pack the image
	// TODO: RLE
	const uint8_t *data = image.indexed();

	for (int i = 0; i < image.height; i++)
		for (int j = 0; j < image.width; j++)
		{
			if ((*data & 0xc0) != 0xc0)
				stream <= *data++;
			else
			{
				stream <= 0xc1;
				stream <= *data++;
			}
		}

	// write the palette
	stream <= 0x0c;	// palette ID byte
	
	stream.write(reinterpret_cast<const char *>(image.palette()), image.palette_size());
}

Image ImageLoader::Load(const std::filesystem::path &file)
{
	// TODO: detect via header
	if (file.extension() == ".pcx")
	    return LoadPCX(file);
    else if (stbi_info(file.string().c_str(), nullptr, nullptr, nullptr) == 1)
    {
        int w, h;
        stbi_uc *stbi = stbi_load(file.string().c_str(), &w, &h, nullptr, 4);
        Image img = Image::create_rgba(w, h);
        memcpy(img.data.data(), stbi, w * h * 4);
        stbi_image_free(stbi);
        return img;
    }
	else
        return {};
}

Image ImageLoader::Load(const std::span<const uint8_t, std::dynamic_extent> &data)
{
    if (stbi_info_from_memory(data.data(), data.size(), nullptr, nullptr, nullptr) == 1)
    {
        int w, h;
        stbi_uc *stbi = stbi_load_from_memory(data.data(), data.size(), &w, &h, nullptr, 4);
        Image img = Image::create_rgba(w, h);
        memcpy(img.data.data(), stbi, w * h * 4);
        stbi_image_free(stbi);
        return img;
    }

    return {};
}

void ImageLoader::Save(const Image &skin, const std::filesystem::path &file) const
{
	if (file.extension() == ".pcx")
		SavePCX(skin, file);
	else if (file.extension() == ".png")
        stbi_write_png(file.string().c_str(), skin.width, skin.height, 4, skin.rgba(), skin.width * 4);
	else if (file.extension() == ".tga")
        stbi_write_tga(file.string().c_str(), skin.width, skin.height, 4, skin.rgba());
	else if (file.extension() == ".jpg" ||
             file.extension() == ".jpeg")
        stbi_write_jpg(file.string().c_str(), skin.width, skin.height, 4, skin.rgba(), 100);
	else
		throw std::runtime_error("invalid file type");
}

ImageLoader &images()
{
    static ImageLoader instance;
    return instance;
}