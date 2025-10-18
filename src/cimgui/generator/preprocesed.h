
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Gamepad;
typedef union SDL_Event SDL_Event;
               bool ImGui_ImplSDL3_InitForOpenGL(SDL_Window* window, void* sdl_gl_context);
               bool ImGui_ImplSDL3_InitForVulkan(SDL_Window* window);
               bool ImGui_ImplSDL3_InitForD3D(SDL_Window* window);
               bool ImGui_ImplSDL3_InitForMetal(SDL_Window* window);
               bool ImGui_ImplSDL3_InitForSDLRenderer(SDL_Window* window, SDL_Renderer* renderer);
               bool ImGui_ImplSDL3_InitForSDLGPU(SDL_Window* window);
               bool ImGui_ImplSDL3_InitForOther(SDL_Window* window);
               void ImGui_ImplSDL3_Shutdown();
               void ImGui_ImplSDL3_NewFrame();
               bool ImGui_ImplSDL3_ProcessEvent(const SDL_Event* event);
enum ImGui_ImplSDL3_GamepadMode { ImGui_ImplSDL3_GamepadMode_AutoFirst, ImGui_ImplSDL3_GamepadMode_AutoAll, ImGui_ImplSDL3_GamepadMode_Manual };
               void ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode mode, SDL_Gamepad** manual_gamepads_array = nullptr, int manual_gamepads_count = -1);