#pragma once

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <iostream>
#include <optional>

#include "tdtexture.h"


static std::string read_file(const char *filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file");
	}

	std::string content = "";

	std::string line;
	while (std::getline(file, line))
	{
		content += line + "\n";
	}

	file.close();

	return content;
}



namespace td
{
	class shader
	{
	public:
		virtual void use() const = 0;
		virtual void end() = 0;


		void set_uniform_scalar(const char *name, float data)
		{
			int location = find_uniform(name);

			use();
			glUniform1f(location, data);
		}

		template <unsigned int N = 4>
		void set_uniform_matrix(const char *name, const float *data)
		{
			int location = find_uniform(name);

			use();
			switch (N)
			{
			case 4:
				glUniformMatrix4fv(location, 1, GL_FALSE, data);
				break;
			default:
				std::cout << "Shader: Unable to set the uniform of a matrix with size outside [1, 4]" << std::endl;
				throw std::runtime_error("");
			}
		}

		template <unsigned int N = 3>
		void set_uniform_vector(const char *name, const std::vector<float> &data)
		{
			int location = find_uniform(name);

			use();
			switch (N)
			{
			case 3:
				glUniform3f(location, data[0], data[1], data[2]);
				break;
			default:
				std::cout << "Shader: Unable to set the uniform of a vector with size outside [1, 4]" << std::endl;
				throw std::runtime_error("");
			}
		}

	protected:
		int find_uniform(const char *name) const
		{
			int location = glGetUniformLocation(m_program, name);
			if (location == -1)
			{
				std::cout << "Failed to find the shader uniform location: " << name << std::endl;
				throw std::runtime_error("");
			}
			return location;
		}

		unsigned int m_program = 0;
	};

	class graphics_shader : public shader
	{
	public:
		graphics_shader() {};

		graphics_shader &load(const char *vertex_file, const char *fragment_file)
		{
			std::string vertex = read_file(vertex_file);
			std::string fragment = read_file(fragment_file);

			const char *vertexSource = vertex.c_str();
			const char *fragmentSource = fragment.c_str();

			int success;
			char infoLog[512];

			// compile the vertex shader
			unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vertexShader, 1, &vertexSource, nullptr);
			glCompileShader(vertexShader);

			glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);

				std::cout << "Failed to create the vertex shader\n" << infoLog << std::endl;
				throw std::runtime_error("");
			}

			// compile the fragment shader
			unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
			glCompileShader(fragmentShader);

			glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);

				std::cout << "Failed to create the fragment shader\n" << infoLog << std::endl;
				throw std::runtime_error("");
			}

			// link to shader program
			m_program = glCreateProgram();
			glAttachShader(m_program, vertexShader);
			glAttachShader(m_program, fragmentShader);
			glLinkProgram(m_program);

			// check link status
			glGetProgramiv(m_program, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(m_program, 512, nullptr, infoLog);

				std::cout << "Failed to link the shader program\n" << infoLog << std::endl;
				throw std::runtime_error("");
			}

			// we are done with the vertex and fragment shaders
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			return *this;
		}

		void use() const override
		{
			glUseProgram(m_program);
		}

		void end() override
		{
			glDeleteProgram(m_program);
		}
	};


	class compute_shader : public shader
	{
	public:
		compute_shader()
		{}

		void load(const char *compute_file, int width, int height)
		{
			m_width = width;
			m_height = height;

			std::string content = read_file(compute_file);
			const char *source = content.c_str();

			// create shader
			unsigned int shader = glCreateShader(GL_COMPUTE_SHADER);
			glShaderSource(shader, 1, &source, nullptr);
			glCompileShader(shader);

			// check compile status
			int success;
			char infoLog[512];

			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 512, nullptr, infoLog);

				std::cout << "Failed to create the compute shader:\n" << infoLog << std::endl;
				throw std::runtime_error("Compute shader compile error");
			}

			m_program = glCreateProgram();
			glAttachShader(m_program, shader);
			glLinkProgram(m_program);

			// check link status
			glGetProgramiv(m_program, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(m_program, 512, nullptr, infoLog);

				std::cout << "Failed to link the compute program:\n" << infoLog << std::endl;
				throw std::runtime_error("Compute program link error");
			}

			glDeleteShader(shader);
		}

		void set_local_scale(int scale)
		{
			m_local_scale = scale;
		}

		void use() const override
		{
			glUseProgram(m_program);
		}

		void end() override
		{
			glDeleteProgram(m_program);
		}

		void execute()
		{
			glDispatchCompute(m_width / m_local_scale, m_height / m_local_scale, 1);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

	private:
		int m_width = -1;
		int m_height = -1;
		int m_local_scale = 32;
	};
}
