#pragma once


#include <glad/glad.h>

#include <vector>
#include <iostream>

namespace td
{
	// TODO, Implement partial vertex rendering to account for multiple materials for
	// patches of vertex faces
	class mesh
	{
	public:
		mesh() {}

		// Load vertex data with vertex positions
		void load3(
			const std::vector<float> &positions  // multiple of 3 for vertices are 3D
		)
		{
			// create vao
			glGenVertexArrays(1, &m_vao);
			glBindVertexArray(m_vao);

			// create vbo
			glGenBuffers(1, &m_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

			// add data to vbo
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * positions.size(), positions.data(), GL_STATIC_DRAW);

			// set vertex attribute pointers
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *)0);
			glEnableVertexAttribArray(0);

			// and we are done
			m_vertices = (int)positions.size() / 3;
		}

		// Load vertex data with vertex positions and textures
		void load5(
			// contains the position, texture coordinates
			std::vector<float> &positions
		)
		{
			glGenVertexArrays(1, &m_vao);
			glBindVertexArray(m_vao);

			glGenBuffers(1, &m_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * positions.size(), positions.data(), GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *)0);
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void *)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);


			// drawElements takes the number of indices (number of vertices) to draw
			m_vertices = (int)positions.size() / 5;
		}

		// Load vertex data with vertex positions, colors, and textures
		void load8(
			// containing position, color, and texture vertex attributes
			std::vector<float> &positions
		)
		{
			glGenVertexArrays(1, &m_vao);
			glBindVertexArray(m_vao);

			glGenBuffers(1, &m_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * positions.size(), positions.data(), GL_STATIC_DRAW);

			const int stride = 3 + 3 + 2;
			// position vertex attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void *)0);
			glEnableVertexAttribArray(0);

			// color vertex attribute, with offset = 3
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void *)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);

			// texture coordinate vertex attribute, with offset = 3 + 3 = 6
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void *)(6 * sizeof(float)));
			glEnableVertexAttribArray(2);

			m_vertices = (int)positions.size() / stride;
		}

		// Askes the mesh to interpet the vertex data using indices
		void load_indices(
			std::vector<unsigned int> &indices
		)
		{
			// sanity check
			if (indices.size() % 3 != 0)
			{
				std::cout << "Index Data not a multiple of three: got " << indices.size() << std::endl;
				throw std::runtime_error("");
			}

			glGenBuffers(1, &m_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);

			m_vertices = (int)indices.size();
			m_using_indices = true;
		}


		// Draws the mesh using OpenGL draw calls
		void render() const
		{
			glBindVertexArray(m_vao);

			if (m_using_indices)
			{
				// ebo drawing
				glDrawElements(GL_TRIANGLES, m_vertices, GL_UNSIGNED_INT, nullptr);
				return;
			}

			// vbo drawing
			glDrawArrays(GL_TRIANGLES, 0, m_vertices);
		}

		void end()
		{
			glDeleteVertexArrays(1, &m_vao);
			glDeleteBuffers(1, &m_vbo);
			if (m_using_indices)
				glDeleteBuffers(1, &m_ebo);
		}

	private:
		// the total amount of vertices to draw
		int m_vertices = 0;

		unsigned int m_vbo = 0;
		unsigned int m_vao = 0;
		unsigned int m_ebo = 0;

		bool m_using_indices = false;
	};
}
