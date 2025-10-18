#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>

// OpenGL için - deprecation uyarılarını sustur
#if defined(__APPLE__)
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
#endif

// SADECE cimgui
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include "cimgui/cimgui_backends.h"

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
        "GPS Monitor",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        printf("SDL_GL_CreateContext Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    // ImGui başlat
    ImGuiContext* ctx = igCreateContext(NULL);
    igSetCurrentContext(ctx);
    ImGuiIO* io = cImGui_GetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // cimgui backend'lerini başlat
    cImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    cImGui_ImplOpenGL3_Init("#version 150");

    bool done = false;
    ImVec4 clear_color;
    clear_color.x = 0.45f;
    clear_color.y = 0.55f;
    clear_color.z = 0.60f;
    clear_color.w = 1.00f;
    
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            cImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && 
                event.window.event == SDL_WINDOWEVENT_CLOSE)
                done = true;
        }

        cImGui_ImplOpenGL3_NewFrame();
        cImGui_ImplSDL2_NewFrame();
        igNewFrame();

        // GPS penceresi
        igBegin("GPS Data", NULL, 0);
        igText("Latitude: 0.0");
        igText("Longitude: 0.0");
        igText("Altitude: 0.0 m");
        igText("Speed: 0.0 km/h");
        igSeparator();
        igText("Satellites: 0");
        igText("Fix Quality: No Fix");
        igEnd();

        igRender();
        glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        cImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
        SDL_GL_SwapWindow(window);
    }

    cImGui_ImplOpenGL3_Shutdown();
    cImGui_ImplSDL2_Shutdown();
    igDestroyContext(ctx);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}