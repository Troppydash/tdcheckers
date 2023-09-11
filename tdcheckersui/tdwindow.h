#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#include "tdobservable.h"

namespace td
{
	class window;
	class window_object
	{
	public:
		virtual void start(window &window) = 0;
		virtual void update(float dt) = 0;
		virtual void render() = 0;
		virtual void end() = 0;
	};

	struct window_settings
	{
		int width;
		int height;
		std::string title;
	};

	struct window_inputs
	{
		// (x,y) window coordinates
		td::observable<std::pair<double, double>> cursor_position;

		td::observable<std::pair<double, double>> cursor_drag_start;

		// (true = pressed, false = depressed
		td::observable<bool> cursor_pressed;

		// updated when a click happened
		td::observable<std::pair<double, double>> cursor_clicked;
	};

	class window
	{
	public:
		window()
		{}

		window(window_settings settings)
			: m_settings(settings)
		{}

		void include(window_object *object)
		{
			m_objects.push_back(object);
		}

		void start()
		{
			// glfw hints
			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_SAMPLES, 4);

			// create window
			m_window = glfwCreateWindow(
				m_settings.width,
				m_settings.height,
				m_settings.title.c_str(),
				nullptr,
				nullptr
			);
			glfwMakeContextCurrent(m_window);

			// init glad
			gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

			// opengl setup
			glViewport(0, 0, m_settings.width, m_settings.height);

			// v-sync
			glfwSwapInterval(1);

			// alpha blending
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// antialiasing
			glEnable(GL_MULTISAMPLE);


			for (auto &object : m_objects)
			{
				object->start(*this);
			}
		}

		void main()
		{

			// register inputs
			glfwSetWindowUserPointer(m_window, this);

			// for the cursor position
			glfwSetCursorPosCallback(m_window, [](GLFWwindow *win, double xpos, double ypos)
			{
				td::window *window = reinterpret_cast<td::window *>(glfwGetWindowUserPointer(win));
				window->m_inputs.cursor_position.update({ xpos, ypos });
			});

			glfwSetCursorEnterCallback(m_window, [](GLFWwindow *win, int entered)
			{
				td::window *window = reinterpret_cast<td::window *>(glfwGetWindowUserPointer(win));
				if (!entered)
				{
					window->m_inputs.cursor_position.update({ -1, -1 });
				}
			});

			// for the mouse clicks
			glfwSetMouseButtonCallback(m_window, [](GLFWwindow *win, int button, int action, int mods)
			{
				td::window *window = reinterpret_cast<td::window *>(glfwGetWindowUserPointer(win));
				if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
				{
					// we pressed it
					window->m_inputs.cursor_pressed.update(true);

					// and the start of the drag is here
					std::pair<double, double> currentpos = window->m_inputs.cursor_position.get();
					window->m_inputs.cursor_drag_start.update(currentpos);
				}

				if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
				{
					// we depressed it
					window->m_inputs.cursor_pressed.update(false);

					// compute if pressed
					auto start = window->m_inputs.cursor_drag_start.get();
					auto end = window->m_inputs.cursor_position.get();
					double distance = std::sqrt(pow(start.first - end.first, 2) + pow(start.second - end.second, 2));
					if (distance < 15.0)
					{
						window->m_inputs.cursor_clicked.update(end);
					}

					// end drag
					window->m_inputs.cursor_drag_start.update({ -1, -1 });
				}
			});


			double last_time = glfwGetTime();

			while (!glfwWindowShouldClose(m_window))
			{
				double time = glfwGetTime();
				double dt = time - last_time;
				last_time = time;

				glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				for (auto &object : m_objects)
				{
					object->update((float)dt);
				}

				for (auto &object : m_objects)
				{
					object->render();
				}

				glfwPollEvents();
				glfwSwapBuffers(m_window);
			}
		}


		void end()
		{
			for (auto &object : m_objects)
			{
				object->end();
			}

			glfwDestroyWindow(m_window);
			glfwTerminate();
		}

		GLFWwindow *get_window()
		{
			return m_window;
		}

		const window_settings &get_settings() const
		{
			return m_settings;
		}

		const window_inputs &get_inputs() const
		{
			return m_inputs;
		}


		
	public:
		window_inputs m_inputs = {
			{{-1, -1}},
			{{-1, -1}},
			{false},
			{{-1, -1}}
		};

	private:
		window_settings m_settings = {
			750,
			750,
			"tdfluid"
		};

		// non-portable members
		GLFWwindow *m_window = nullptr;

		std::vector<window_object *> m_objects;
	};
}
