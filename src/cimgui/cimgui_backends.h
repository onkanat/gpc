#ifndef CIMGUI_BACKENDS_H
#define CIMGUI_BACKENDS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SDL_Window;
union SDL_Event;
struct ImDrawData;
struct ImGuiIO;

// ImGui Core
struct ImGuiIO* cImGui_GetIO(void);

// SDL2 Backend
bool cImGui_ImplSDL2_InitForOpenGL(struct SDL_Window* window, void* sdl_gl_context);
void cImGui_ImplSDL2_Shutdown(void);
void cImGui_ImplSDL2_NewFrame(void);
bool cImGui_ImplSDL2_ProcessEvent(const union SDL_Event* event);

// OpenGL3 Backend
bool cImGui_ImplOpenGL3_Init(const char* glsl_version);
void cImGui_ImplOpenGL3_Shutdown(void);
void cImGui_ImplOpenGL3_NewFrame(void);
void cImGui_ImplOpenGL3_RenderDrawData(struct ImDrawData* draw_data);

#ifdef __cplusplus
}
#endif

#endif // CIMGUI_BACKENDS_H
