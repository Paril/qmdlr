#pragma once

#include <memory>
#include <cstdint>
#include <filesystem>
#include <nfd.hpp>
#include <span>
#include "Types.h"

// higher level representation of a 32-bit image
struct Image
{
	uint32_t				width = 0;
	uint32_t				height = 0;
	std::vector<uint8_t>	data;

	// if set, we came from an 8-bit skin
	struct {
		std::vector<uint8_t>    data;
		std::vector<uint8_t>    palette;
	} source;
	
	static Image create_rgba(uint32_t w, uint32_t h)
	{
		Image img;
		img.width = w;
		img.height = h;
		img.data.resize(w * h * 4);
		memset(img.data.data(), 0, img.data.size());
		return img;
	}

	static Image create_indexed(uint32_t w, uint32_t h)
	{
		Image img;
		img.width = w;
		img.height = h;
		img.source.data.resize(w * h);
		img.source.palette.resize(256 * 3);
		memset(img.source.data.data(), 0, img.source.data.size());
		memset(img.source.palette.data(), 0, img.source.palette.size());
		return img;
	}
	
	uint8_t *rgba() { return data.data(); }
	const uint8_t *rgba() const { return data.data(); }
	size_t rgba_size() const { return data.size(); }
	
	uint8_t *indexed() { return source.data.data(); }
	const uint8_t *indexed() const { return source.data.data(); }
	uint8_t *palette() { return source.palette.data(); }
	const uint8_t *palette() const { return source.palette.data(); }
	size_t palette_size() const { return source.palette.size(); }
	size_t indexed_size() const { return source.data.size(); }

	size_t data_size() const
	{
		return vector_element_size(data) + vector_element_size(source.data) + vector_element_size(source.palette);
	}

	bool is_valid() const { return width != 0 && (is_rgba_valid() || is_indexed_valid()); }
	bool is_rgba_valid() const { return rgba_size(); }
	bool is_indexed_valid() const { return indexed_size(); }

	// convert source.data + source.palette to data
	void convert_to_rgba()
	{
		data.resize(width * height * 4);

		uint8_t *out_rgba = rgba();
		uint8_t *in_indexed = indexed();
		uint8_t *pal = palette();

		for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++, out_rgba += 4, in_indexed++)
			{
				uint8_t b = *in_indexed;
				memcpy(out_rgba, &pal[b * 3], 3);
				out_rgba[3] = b == 255 ? 0 : 255;
			}
	}

	// streaming
	void stream_write(std::ostream &stream) const;
	void stream_read(std::istream &stream);

	// operations.
	// make a resized copy. if resizeImage is set, nearest neighbor resize
	// is used, otherwise it clips.
	Image resized(uint32_t width, uint32_t height, bool resizeImage) const
	{
		Image img;

		// FIXME: should be mergable
		if (is_indexed_valid())
		{
			img = Image::create_indexed(width, height);

			const uint8_t *src = reinterpret_cast<const uint8_t *>(indexed());
			uint8_t *dst = reinterpret_cast<uint8_t *>(img.indexed());

			if (resizeImage)
			{
				float x_scale = (float) this->width / width;
				float y_scale = (float) this->height / height;

				for (int y = 0; y < height; y++)
					for (int x = 0; x < width; x++)
						dst[(y * width) + x] = src[(((int) (y * y_scale) * this->width)) + (int) (x * x_scale)];
			}
			else
			{
				for (int y = 0; y < height; y++)
					for (int x = 0; x < width; x++)
					{
						if (y >= this->height || x >= this->width)
							dst[(y * width) + x] = 0;
						else
							dst[(y * width) + x] = src[(y * this->width) + x];
					}
			}
		}
		else
		{
			img = Image::create_rgba(width, height);

			const uint32_t *src = reinterpret_cast<const uint32_t *>(rgba());
			uint32_t *dst = reinterpret_cast<uint32_t *>(img.rgba());

			if (resizeImage)
			{
				float x_scale = (float) this->width / width;
				float y_scale = (float) this->height / height;

				for (int y = 0; y < height; y++)
					for (int x = 0; x < width; x++)
						dst[(y * width) + x] = src[(((int) (y * y_scale) * this->width)) + (int) (x * x_scale)];
			}
			else
			{
				for (int y = 0; y < height; y++)
					for (int x = 0; x < width; x++)
					{
						if (y >= this->height || x >= this->width)
							dst[(y * width) + x] = 0xFF000000;
						else
							dst[(y * width) + x] = src[(y * this->width) + x];
					}
			}
		}

		return img;
	}
};

class ImageLoader
{
public:
	Image Load(const std::filesystem::path &file);
	Image Load(const std::span<const uint8_t, std::dynamic_extent> &data);
	void Save(const Image &skin, const std::filesystem::path &file) const;

	const std::vector<nfdfilteritem_t> &SupportedFormats()
	{
		static const std::vector<nfdfilteritem_t> formats {
			{ "Supported", "png,jpg,jpeg,tga,pcx,lmp" },
			{ "PNG",       "png" },
			{ "JPEG",      "jpg,jpeg" },
			{ "TGA",       "tga" },
			{ "PCX",       "pcx" },
			{ "LMP",       "lmp" }
		};
		return formats;
	}

	// resolve a skin path from a base directory to the actual skin.
	std::optional<std::filesystem::path> ResolveSkinFile(const std::filesystem::path &base_dir, const std::filesystem::path &skin_path, const std::initializer_list<const char *> &formats)
	{
		if (skin_path.is_absolute())
		{
			if (std::filesystem::exists(skin_path))
				return skin_path;

			return std::nullopt;
		}

		// try to find the matching file
		std::filesystem::path skin_dir = base_dir;
		std::filesystem::path skin_file;

		while (skin_dir.has_parent_path())
		{
			for (auto &format : formats)
			{
				skin_file = skin_dir;
				skin_file /= skin_path;
				skin_file.replace_extension(std::string(".") + format);

				if (std::filesystem::exists(skin_file))
					return skin_file;
			}

			auto parent = skin_dir.parent_path();

			if (skin_dir == parent)
				break;

			skin_dir = parent;
		}

		return std::nullopt;
	}
};

ImageLoader &images();