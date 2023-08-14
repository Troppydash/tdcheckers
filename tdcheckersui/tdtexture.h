#pragma once



#include <glad/glad.h>

#include <vector>
#include <iostream>

#include <stb_image.h>

namespace td
{
	class texture
	{
	public:
		texture() {}

		// Load a RGB image texture
		texture &load(const char *filename)
		{
			// flip axis
			stbi_set_flip_vertically_on_load(true);

			// load texture image
			int width, height, channels;
			unsigned char *data = stbi_load(
				filename,
				&width,
				&height,
				&channels,
				0
			);

			m_width = width;
			m_height = height;

			if (!data)
			{
				std::cout << "Failed to load textures form file: " << filename << std::endl;
				throw std::runtime_error("");
			}

			// allocate texture
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);

			// set texture wrapping and filtering options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// the line tells OpenGL that our pixel is 1 byte aligned
			// meaning a tightly packed image data
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// push data to the texture
			// and generate mipmap
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			stbi_image_free(data);

			// save id
			m_texture = texture;
		}

		texture &load_empty_r32f(int width, int height)
		{
			// generate texture
			unsigned int data;
			glGenTextures(1, &data);

			// activate the texture
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, data);

			// this shouldnt be needed
			// for we are using an image
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			// zero the texture image data
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);

			m_texture = data;
			m_width = width;
			m_height = height;

			return *this;
		}

		texture &load_empty_rg32f(int width, int height)
		{
			// generate texture
			unsigned int data;
			glGenTextures(1, &data);

			// activate the texture
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, data);

			// this shouldnt be needed
			// for we are using an image
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			// zero the texture image data
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);

			m_texture = data;
			m_width = width;
			m_height = height;

			return *this;
		}

		void use(int offset = 0) const
		{
			glActiveTexture(GL_TEXTURE0 + offset);
			glBindTexture(GL_TEXTURE_2D, m_texture);
		}

		template <unsigned int Components = 1>
		void use_image(int offset = 0) const
		{
			int format;
			switch (Components)
			{
			case 1:
				format = GL_R32F;
				break;
			case 2:
				format = GL_RG32F;
				break;
			default:
				throw std::runtime_error("unknown use_image Component");
			}

			// bind texture to the m_offset image texture slot
			glBindImageTexture(offset, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, format);
		}

		void end()
		{
			glDeleteTextures(1, &m_texture);
		}

		void upload_r32f(const std::vector<float> &data)
		{
			// sanity check
			if (data.size() != m_width * m_height)
			{
				throw std::runtime_error("compute data size mismatch");
			}

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_texture);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_width, m_height, 0, GL_RED, GL_FLOAT, data.data());
		}

		void upload_rg32f(const std::vector<float> &data)
		{
			// sanity check
			if (data.size() / 2 != m_width * m_height)
			{
				throw std::runtime_error("compute data size mismatch");
			}

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_texture);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, m_width, m_height, 0, GL_RG, GL_FLOAT, data.data());
		}

		int get_width() const
		{
			return m_width;
		}

		int get_height() const
		{
			return m_height;
		}

	private:
		unsigned int m_texture = -1;
		int m_width = -1;
		int m_height = -1;
	};
}
