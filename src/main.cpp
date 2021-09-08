// #define ENABLE_VSYNC
#define MULTIPLIER 2

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <GL/gl3w.h>
#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <shader.hpp>

#include <iostream>
#include <vector>
#include <chrono>

const int WIDTH = 800, HEIGHT = 800;

struct DrawArraysIndirectCommand {
	GLuint  count;
	GLuint  instanceCount;
	GLuint  first;
	GLuint  baseInstance;
};

struct Vertex {
	glm::vec4 pos;
	glm::vec4 colorR;
};

struct Instance {
	glm::vec4 ctrRad;
};

struct Uniform {
	float deltaTime;
};

template<typename T>
size_t byteSize(const typename std::vector<T>& vec) {
	return sizeof(T) * vec.size();
}

struct state {
	SDL_Window* window = nullptr;
	SDL_GLContext glcontext = nullptr;

	void clear() {
		if (!glcontext) SDL_GL_DeleteContext(glcontext);
		if (!window) SDL_DestroyWindow(window);
		SDL_Quit();
	}
} state;

int main() {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef ENABLE_VSYNC
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif

	state.window = SDL_CreateWindow("AtomicAdd & DrawIndirect",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
	);

	if (!state.window) {
		std::cerr << "Could not create window: " << SDL_GetError() << std::endl;
		state.clear();
		return 1;
	}

	state.glcontext = SDL_GL_CreateContext(state.window);
	if (!state.glcontext) {
		std::cerr << "Could not create OpenGL context: " << SDL_GetError() << std::endl;
		state.clear();
		return 1;
	}

#ifdef ENABLE_VSYNC
	SDL_GL_SetSwapInterval(1);
#endif

	if (gl3wInit()) {
		std::cerr << "failed to initialize OpenGL" << std::endl;
		state.clear();
		return 1;
	}

	if (!gl3wIsSupported(4, 3)) {
		std::cerr << "OpenGL 4.3 not supported" << std::endl;
		state.clear();
		return 1;
	}

	glEnable(GL_DEPTH_TEST);

	GraphicsShader cgProg("drawTriangles.vert", "drawTriangles.frag");
	ComputeShader compProg("genTriangles.comp");
	ComputeShader occProg("dynTriangles.comp");

	const unsigned int triCount = 32 * MULTIPLIER * 32 * MULTIPLIER;

	GLuint baseVertBuf;
	glCreateBuffers(1, &baseVertBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, baseVertBuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triCount * sizeof(Vertex) * 3 + sizeof(int), nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	GLuint baseInstBuf;
	glCreateBuffers(1, &baseInstBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, baseInstBuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triCount * sizeof(Instance), nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, baseVertBuf);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, baseInstBuf);
	compProg.use();
	glDispatchCompute(MULTIPLIER, MULTIPLIER, 1);

	Uniform uniBufData = {};
	GLuint uniBuf;
	glCreateBuffers(1, &uniBuf);
	glBindBuffer(GL_UNIFORM_BUFFER, uniBuf);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(Uniform), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	GLuint dynVertBuf;
	glCreateBuffers(1, &dynVertBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, dynVertBuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triCount * sizeof(Vertex) * 3, nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	DrawArraysIndirectCommand primarySettings = {
		.count = 0,
		.instanceCount = 1,
		.first = 0,
		.baseInstance = 0,
	};

	GLuint drawBuf;
	glCreateBuffers(1, &drawBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawBuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(primarySettings), &primarySettings, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Bindings do not change anyway
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, dynVertBuf);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, drawBuf);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniBuf); // Binds to both target (replaces previous binding) and index

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawBuf);

	SDL_Event windowEvent;
	while (true) {
		if (SDL_PollEvent(&windowEvent)) {
			if (SDL_QUIT == windowEvent.type)
				break;
		}
		static auto startTime = std::chrono::high_resolution_clock::now();
		const auto currentTime = std::chrono::high_resolution_clock::now();
		const float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		// startTime = currentTime;

		// Send data to VRAM
		glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(primarySettings), &primarySettings);

		uniBufData.deltaTime = deltaTime;
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(uniBufData), &uniBufData);

		// Occlude and rotate triangles
		occProg.use();
		glDispatchCompute(MULTIPLIER, MULTIPLIER, 1);

		// Draw triangles
		glViewport(0, 0, WIDTH, HEIGHT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cgProg.use();
		glDrawArraysIndirect(GL_TRIANGLES, 0); // uses drawBuf in GL_DRAW_INDIRECT_BUFFER

		SDL_GL_SwapWindow(state.window);
	}

	state.clear();
	return 0;
}