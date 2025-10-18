#include "./imgui/imgui.h"
#ifdef IMGUI_ENABLE_FREETYPE
#include "./imgui/misc/freetype/imgui_freetype.h"
#endif
#include "./imgui/imgui_internal.h"
#include "cimgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#ifdef CIMGUI_USE_GLFW
#include "imgui_impl_glfw.h"
#endif

#ifdef CIMGUI_USE_OPENGL3
#include "imgui_impl_opengl3.h"
#endif

#ifdef CIMGUI_USE_OPENGL2
#include "imgui_impl_opengl2.h"
#endif

#ifdef CIMGUI_USE_SDL2
#include "imgui_impl_sdl2.h"
#endif

#ifdef CIMGUI_USE_SDL3
#include "imgui_impl_sdl3.h"
#endif

#include "cimgui_impl.h"

#ifdef CIMGUI_USE_VULKAN

CIMGUI_API ImGui_ImplVulkanH_Window* ImGui_ImplVulkanH_Window_ImGui_ImplVulkanH_Window()
{
	return IM_NEW(ImGui_ImplVulkanH_Window)();
}
CIMGUI_API void ImGui_ImplVulkanH_Window_Construct(ImGui_ImplVulkanH_Window* self)
{
	IM_PLACEMENT_NEW(self) ImGui_ImplVulkanH_Window();
}

#endif

// C wrapper fonksiyonları
extern "C" {
    ImGuiIO* cImGui_GetIO() {
        return &ImGui::GetIO();
    }
    
    bool cImGui_ImplSDL2_InitForOpenGL(SDL_Window* window, void* sdl_gl_context) {
        return ImGui_ImplSDL2_InitForOpenGL(window, sdl_gl_context);
    }
    
    bool cImGui_ImplOpenGL3_Init(const char* glsl_version) {
        return ImGui_ImplOpenGL3_Init(glsl_version);
    }
    
    void cImGui_ImplSDL2_Shutdown() {
        ImGui_ImplSDL2_Shutdown();
    }
    
    void cImGui_ImplOpenGL3_Shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
    }
    
    bool cImGui_ImplSDL2_ProcessEvent(const SDL_Event* event) {
        return ImGui_ImplSDL2_ProcessEvent(event);
    }
    
    void cImGui_ImplSDL2_NewFrame() {
        ImGui_ImplSDL2_NewFrame();
    }
    
    void cImGui_ImplOpenGL3_NewFrame() {
        ImGui_ImplOpenGL3_NewFrame();
    }
    
    void cImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data) {
        ImGui_ImplOpenGL3_RenderDrawData(draw_data);
    }
}
