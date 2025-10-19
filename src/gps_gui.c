// GPC — GPS Console
//  Serial port üzerinden GPS verilerini okuyup görselleştiren bir uygulama
//  SDL2, OpenGL ve ImGui kullanır

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(__has_include)
#  if __has_include(<SDL.h>)
#    include <SDL.h>
#  elif __has_include(<SDL2/SDL.h>)
#    include <SDL2/SDL.h>
#  else
#    error "SDL2 headers not found. Please install SDL2 development packages."
#  endif
#else
#  include <SDL2/SDL.h> // fallback changed from <SDL.h> to satisfy linters without __has_include
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

// GPS modules
#include "include/gps_data.h"
#include "include/gps_serial.h"
#include "include/gps_map.h"
#include "include/gps_polar.h"
#include "include/gps_compass.h"
#include "include/gps_console.h"

// GUI Configuration
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define LEFT_PANEL_WIDTH 320
#define STATUS_BAR_HEIGHT 40

// Global application state
typedef struct {
    gps_data_t gps_data;
    gps_serial_t gps_serial;
    map_system_t map_system;
    polar_view_t polar_view;
    compass_t compass;
    console_t console;
    bool show_demo_window;
    bool auto_scroll_log;
    char connection_status_text[256];
    char last_error[512];
    
    // UI state
    int active_tab;              // 0=Telemetry, 1=Map, 2=Satellites, 3=Polar, 4=Compass, 5=Raw Data
    bool show_gpx_export_dialog;
    char gpx_filename[256];
    bool gpx_export_success;
    char gpx_export_message[256];
    
    // Connection dialog state
    bool show_connection_dialog;
    bool auto_connect_enabled;
    char selected_port[256];
    int selected_baud;
    char available_ports[16][256];
    int available_port_count;
} app_state_t;

// Function declarations
void render_header_bar(app_state_t* app);
void render_telemetry_window(app_state_t* app);
void render_telemetry_panel(app_state_t* app);
void render_map_window(app_state_t* app);
void render_enhanced_map_panel(app_state_t* app);
void render_satellite_window(app_state_t* app);
void render_satellite_panel(app_state_t* app);
void render_polar_window(app_state_t* app);
void render_polar_view_panel(app_state_t* app);
void render_compass_window(app_state_t* app);
void render_compass_panel(app_state_t* app);
void render_raw_data_window(app_state_t* app);
void render_raw_data_panel(app_state_t* app);
void render_status_bar(app_state_t* app);
void render_connection_dialog(app_state_t* app);
void render_gpx_export_dialog(app_state_t* app);
void update_gps_data(app_state_t* app);
void setup_imgui_style(void);
void ensure_data_directory(void);

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    
    // Ensure data directory exists
    ensure_data_directory();
    
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
        "GPC — GPS Console",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
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
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // cimgui backend'lerini başlat
    cImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    cImGui_ImplOpenGL3_Init("#version 150");

    // Setup custom style
    setup_imgui_style();

    // Initialize application state
    app_state_t app_state;
    memset(&app_state, 0, sizeof(app_state));
    gps_data_init(&app_state.gps_data);
    gps_serial_init(&app_state.gps_serial);
    map_system_init(&app_state.map_system);
    polar_view_init(&app_state.polar_view);
    compass_init(&app_state.compass);
    console_init(&app_state.console);
    app_state.show_demo_window = false;
    app_state.auto_scroll_log = true;
    app_state.active_tab = 0;
    app_state.show_gpx_export_dialog = false;
    strcpy(app_state.connection_status_text, "Not Connected");
    app_state.last_error[0] = '\0';
    strcpy(app_state.gpx_filename, "data/gps_track.gpx");
    
    // Initialize connection dialog state
    app_state.show_connection_dialog = false;
    app_state.auto_connect_enabled = true;  // Default enabled
    strcpy(app_state.selected_port, "");
    app_state.selected_baud = 9600;
    app_state.available_port_count = 0;

    bool done = false;
    ImVec4 clear_color = {0.11f, 0.11f, 0.11f, 1.00f}; // Dark background
    
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

        // Update GPS data
        update_gps_data(&app_state);
        compass_update(&app_state.compass, &app_state.gps_data);

        cImGui_ImplOpenGL3_NewFrame();
        cImGui_ImplSDL2_NewFrame();
        igNewFrame();

        // Main dockspace
        ImGuiViewport* viewport = igGetMainViewport();
        igSetNextWindowPos(viewport->WorkPos, 0, (ImVec2){0, 0});
        igSetNextWindowSize(viewport->WorkSize, 0);
        igSetNextWindowViewport(viewport->ID);
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        bool open = true;
        igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 0.0f);
        igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0f);
        igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0.0f, 0.0f});
        
        igBegin("DockSpace Demo", &open, window_flags);
        igPopStyleVar(3);

        // Render header/menu bar
        render_header_bar(&app_state);

        // Submit the DockSpace
        ImGuiID dockspace_id = igGetID_Str("MyDockSpace");
        igDockSpace(dockspace_id, (ImVec2){0.0f, 0.0f}, ImGuiDockNodeFlags_None, NULL);

        // Render individual panels as separate dockable windows
        render_telemetry_window(&app_state);
        render_map_window(&app_state);
        render_satellite_window(&app_state);
        render_polar_window(&app_state);
        render_compass_window(&app_state);
        render_raw_data_window(&app_state);

        render_status_bar(&app_state);
        render_connection_dialog(&app_state);
        render_gpx_export_dialog(&app_state);

        // Demo window for development
        if (app_state.show_demo_window) {
            igShowDemoWindow(&app_state.show_demo_window);
        }

        igEnd();

        igRender();
        glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        cImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    gps_data_cleanup(&app_state.gps_data);
    gps_serial_cleanup(&app_state.gps_serial);
    map_system_cleanup(&app_state.map_system);
    polar_view_cleanup(&app_state.polar_view);
    compass_cleanup(&app_state.compass);
    console_cleanup(&app_state.console);

    cImGui_ImplOpenGL3_Shutdown();
    cImGui_ImplSDL2_Shutdown();
    igDestroyContext(ctx);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void setup_imgui_style(void) {
    ImGuiStyle* style = igGetStyle();
    
    // Colors based on NOTE.md specifications
    style->Colors[ImGuiCol_WindowBg] = (ImVec4){0.17f, 0.17f, 0.17f, 1.00f}; // #2B2B2B
    style->Colors[ImGuiCol_ChildBg] = (ImVec4){0.12f, 0.12f, 0.12f, 1.00f}; // #1E1E1E
    style->Colors[ImGuiCol_PopupBg] = (ImVec4){0.17f, 0.17f, 0.17f, 1.00f};
    style->Colors[ImGuiCol_Border] = (ImVec4){0.30f, 0.30f, 0.30f, 1.00f};
    style->Colors[ImGuiCol_FrameBg] = (ImVec4){0.20f, 0.20f, 0.20f, 1.00f};
    style->Colors[ImGuiCol_FrameBgHovered] = (ImVec4){0.25f, 0.25f, 0.25f, 1.00f};
    style->Colors[ImGuiCol_FrameBgActive] = (ImVec4){0.00f, 0.59f, 1.00f, 0.30f}; // #0096FF
    style->Colors[ImGuiCol_TitleBg] = (ImVec4){0.12f, 0.12f, 0.12f, 1.00f};
    style->Colors[ImGuiCol_TitleBgActive] = (ImVec4){0.00f, 0.59f, 1.00f, 1.00f}; // #0096FF
    style->Colors[ImGuiCol_Text] = (ImVec4){0.86f, 0.86f, 0.86f, 1.00f}; // #DADADA
    style->Colors[ImGuiCol_Button] = (ImVec4){0.20f, 0.20f, 0.20f, 1.00f};
    style->Colors[ImGuiCol_ButtonHovered] = (ImVec4){0.00f, 0.59f, 1.00f, 0.50f};
    style->Colors[ImGuiCol_ButtonActive] = (ImVec4){0.00f, 0.59f, 1.00f, 1.00f};
    
    // Rounded corners
    style->WindowRounding = 6.0f;
    style->ChildRounding = 4.0f;
    style->FrameRounding = 4.0f;
    style->PopupRounding = 4.0f;
    style->ScrollbarRounding = 6.0f;
    style->GrabRounding = 4.0f;
    style->TabRounding = 4.0f;
    
    // Spacing
    style->WindowPadding = (ImVec2){10.0f, 10.0f};
    style->FramePadding = (ImVec2){8.0f, 4.0f};
    style->ItemSpacing = (ImVec2){8.0f, 6.0f};
    style->ItemInnerSpacing = (ImVec2){6.0f, 4.0f};
}

void render_header_bar(app_state_t* app) {
    if (igBeginMenuBar()) {
        // Logo/Title
        igText("GPC — GPS Console");
        
        igSeparator();
        
        // Connection menu
        if (igBeginMenu("Connection", true)) {
            if (igMenuItem_Bool("Connect...", NULL, false, !gps_serial_is_open(&app->gps_serial))) {
                app->show_connection_dialog = true;
                // Refresh available ports when opening dialog
                gps_serial_list_ports(app->available_ports, 16, &app->available_port_count);
            }
            if (igMenuItem_Bool("Disconnect", NULL, false, gps_serial_is_open(&app->gps_serial))) {
                gps_serial_close(&app->gps_serial);
                app->gps_data.status = GPS_STATUS_DISCONNECTED;
                strcpy(app->connection_status_text, "Disconnected");
                strcpy(app->gps_data.port_name, "");
                app->gps_data.baud_rate = 0;
            }
            igSeparator();
            bool auto_connect = app->auto_connect_enabled;
            if (igMenuItem_Bool("Auto-connect", NULL, auto_connect, true)) {
                app->auto_connect_enabled = !app->auto_connect_enabled;
                if (app->auto_connect_enabled && !gps_serial_is_open(&app->gps_serial)) {
                    // Try to auto-connect immediately
                    char ports[16][256];
                    int port_count;
                    if (gps_serial_list_ports(ports, 16, &port_count) && port_count > 0) {
                        if (gps_serial_open(&app->gps_serial, ports[0], 9600)) {
                            app->gps_data.status = GPS_STATUS_CONNECTED;
                            strcpy(app->gps_data.port_name, ports[0]);
                            app->gps_data.baud_rate = 9600;
                            snprintf(app->connection_status_text, sizeof(app->connection_status_text), 
                                    "Auto-connected to %s", ports[0]);
                        }
                    }
                }
            }
            igEndMenu();
        }
        
        // Tools menu
        if (igBeginMenu("Tools", true)) {
            if (igMenuItem_Bool("Start Recording", NULL, false, !app->map_system.track.recording)) {
                map_system_start_recording(&app->map_system);
            }
            if (igMenuItem_Bool("Stop Recording", NULL, false, app->map_system.track.recording)) {
                map_system_stop_recording(&app->map_system);
            }
            igSeparator();
            if (igMenuItem_Bool("Clear Track", NULL, false, app->map_system.track.point_count > 0)) {
                map_system_clear_track(&app->map_system);
            }
            if (igMenuItem_Bool("Export GPX...", NULL, false, app->map_system.track.point_count > 0)) {
                app->show_gpx_export_dialog = true;
            }
            igSeparator();
            if (igMenuItem_Bool("Start Logging", NULL, false, !app->gps_data.logging_enabled)) {
                char filename[256];
                time_t t = time(NULL);
                struct tm* tm_info = localtime(&t);
                strftime(filename, sizeof(filename), "data/gps_log_%Y%m%d_%H%M%S.nmea", tm_info);
                gps_data_start_logging(&app->gps_data, filename);
            }
            if (igMenuItem_Bool("Stop Logging", NULL, false, app->gps_data.logging_enabled)) {
                gps_data_stop_logging(&app->gps_data);
            }
            igSeparator();
            bool show_demo = app->show_demo_window;
            if (igMenuItem_Bool("Show Demo Window", NULL, show_demo, true)) {
                app->show_demo_window = !app->show_demo_window;
            }
            igEndMenu();
        }
        
        // Right side icons would go here
        
        igEndMenuBar();
    }
}

void render_telemetry_window(app_state_t* app) {
    // Set default size and position for first use
    igSetNextWindowSize((ImVec2){LEFT_PANEL_WIDTH, 400}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){10, 30}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    
    if (igBegin("Telemetry", NULL, ImGuiWindowFlags_None)) {
        render_telemetry_panel(app);
    }
    igEnd();
}

void render_telemetry_panel(app_state_t* app) {
    gps_data_t* gps = &app->gps_data;
        
        // Connection status
        igText("Status: %s", gps_status_to_string(gps->status));
        igSameLine(0, -1);
        if (gps->status == GPS_STATUS_CONNECTED) {
            igTextColored((ImVec4){0.0f, 1.0f, 0.0f, 1.0f}, "●");
        } else {
            igTextColored((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}, "●");
        }
        
        igSeparator();
        
        // Position
        igText("Position:");
        if (gps->position_valid) {
            igText("  Lat: %.6f°", gps->latitude);
            igText("  Lon: %.6f°", gps->longitude);
            igText("  Alt: %.1f m", gps->altitude);
        } else {
            igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, "  No position fix");
        }
        
        igSeparator();
        
        // Time
        igText("Time:");
        if (gps->time_valid) {
            igText("  UTC: %02d:%02d:%02d", 
                   gps->time.hours, gps->time.minutes, gps->time.seconds);
            igText("  Date: %02d/%02d/%04d", 
                   gps->date.day, gps->date.month, 
                   gps->date.year < 100 ? gps->date.year + 2000 : gps->date.year);
        } else {
            igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, "  No time fix");
        }
        
        igSeparator();
        
        // Motion
        igText("Motion:");
        if (gps->motion_valid) {
            igText("  Speed: %.1f km/h", gps->speed_kmh);
            igText("  Course: %.1f°", gps->course);
        } else {
            igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, "  No motion data");
        }
        
        igSeparator();
        
        // Fix quality
        igText("Fix Quality: %s", gps_fix_quality_to_string(gps->fix_quality));
        igText("Satellites: %d/%d", gps->satellites_used, gps->satellites_visible);
        igText("HDOP: %.2f", gps->hdop);
        igText("VDOP: %.2f", gps->vdop);
        igText("PDOP: %.2f", gps->pdop);
        
        igSeparator();
        
        // Statistics
        igText("Statistics:");
        igText("  Total: %lu", gps->total_sentences);
        igText("  Valid: %lu", gps->valid_sentences);
        igText("  Invalid: %lu", gps->invalid_sentences);
        if (gps->total_sentences > 0) {
            float success_rate = (float)gps->valid_sentences / gps->total_sentences * 100.0f;
            igText("  Success: %.1f%%", success_rate);
        }
        
        igSeparator();
        
        // Logging status
        igText("Logging: %s", gps->logging_enabled ? "ON" : "OFF");
        if (gps->logging_enabled) {
            igText("File: %s", gps->log_filename);
        }
}

void render_map_window(app_state_t* app) {
    // Set default size and position for main map window
    igSetNextWindowSize((ImVec2){600, 400}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){LEFT_PANEL_WIDTH + 20, 30}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    
    if (igBegin("Map", NULL, ImGuiWindowFlags_None)) {
        render_enhanced_map_panel(app);
    }
    igEnd();
}

void render_enhanced_map_panel(app_state_t* app) {
    gps_data_t* gps = &app->gps_data;
    map_system_t* map = &app->map_system;
        
        // Map controls
        igText("Map Controls:");
        igSameLine(0, 20);
        
        bool recording = map->track.recording;
        if (igCheckbox("Recording", &recording)) {
            if (recording) {
                map_system_start_recording(map);
            } else {
                map_system_stop_recording(map);
            }
        }
        
        igSameLine(0, 20);
        if (igButton("Clear Track", (ImVec2){0, 0})) {
            map_system_clear_track(map);
        }
        
        igSameLine(0, 20);
        if (igButton("Fit Track", (ImVec2){0, 0}) && map->track.point_count > 0) {
            map_system_zoom_to_fit_track(map);
        }
        
        igSameLine(0, 20);
        if (igCheckbox("Auto Center", &map->view.auto_center)) {
            // Toggle handled by checkbox
        }
        
        // Zoom controls
        igText("Zoom: %.2fx", map->view.zoom_level);
        igSameLine(0, 20);
        if (igButton("Zoom In", (ImVec2){0, 0})) {
            map_system_set_zoom(map, map->view.zoom_level * 1.5);
        }
        igSameLine(0, 10);
        if (igButton("Zoom Out", (ImVec2){0, 0})) {
            map_system_set_zoom(map, map->view.zoom_level / 1.5);
        }
        igSameLine(0, 10);
        if (igButton("Reset Zoom", (ImVec2){0, 0})) {
            map_system_set_zoom(map, 10.0); // 1.0 yerine 10.0
        }
        
        // Track statistics
        if (map->track.point_count > 0) {
            igSeparator();
            igText("Track Info:");
            igText("  Points: %d", map->track.point_count);
            igText("  Distance: %.2f km", map->track.total_distance / 1000.0);
            igText("  Max Speed: %.1f km/h", map->track.max_speed);
            
            if (map->track.start_time > 0) {
                time_t duration = time(NULL) - map->track.start_time;
                igText("  Duration: %02ld:%02ld:%02ld", 
                       duration / 3600, (duration % 3600) / 60, duration % 60);
            }
        }
        
        igSeparator();
        
        // Map canvas
        ImVec2 canvas_pos, canvas_size;
        igGetCursorScreenPos(&canvas_pos);
        igGetContentRegionAvail(&canvas_size);
        
        if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
        if (canvas_size.y < 50.0f) canvas_size.y = 50.0f;
        
        ImDrawList* draw_list = igGetWindowDrawList();
        
        // Background
        ImU32 bg_color = igGetColorU32_Col(ImGuiCol_ChildBg, 1.0f);
        ImDrawList_AddRectFilled(draw_list, canvas_pos, 
                                (ImVec2){canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y}, 
                                bg_color, 4.0f, ImDrawFlags_RoundCornersAll);
        
        // Border
        ImU32 border_color = igGetColorU32_Col(ImGuiCol_Border, 1.0f);
        ImDrawList_AddRect(draw_list, canvas_pos, 
                          (ImVec2){canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y}, 
                          border_color, 4.0f, ImDrawFlags_RoundCornersAll, 1.0f);
        
        // Draw grid lines for reference
        ImU32 grid_color = igGetColorU32_Vec4((ImVec4){0.3f, 0.3f, 0.3f, 0.5f});
        for (int i = 1; i < 4; i++) {
            float x = canvas_pos.x + (canvas_size.x * i / 4);
            float y = canvas_pos.y + (canvas_size.y * i / 4);
            
            // Vertical lines
            ImDrawList_AddLine(draw_list, (ImVec2){x, canvas_pos.y}, 
                              (ImVec2){x, canvas_pos.y + canvas_size.y}, grid_color, 1.0f);
            // Horizontal lines
            ImDrawList_AddLine(draw_list, (ImVec2){canvas_pos.x, y}, 
                              (ImVec2){canvas_pos.x + canvas_size.x, y}, grid_color, 1.0f);
        }
        
        // FIXME: No screen space !!check src/include/gps_map.c  Draw track history 
        if (map->track.point_count > 1 && map->view.show_track) {
            ImU32 track_color = igGetColorU32_Vec4((ImVec4){0.0f, 0.8f, 1.0f, 0.8f});
            int start_idx = map->track.point_count < MAX_TRACK_POINTS ? 0 : map->track.current_index;

            for (int i = 1; i < map->track.point_count; i++) {
                int prev_idx = (start_idx + i - 1) % MAX_TRACK_POINTS;
                int curr_idx = (start_idx + i) % MAX_TRACK_POINTS;

                const track_point_t* prev_point = &map->track.points[prev_idx];
                const track_point_t* curr_point = &map->track.points[curr_idx];
                if (!prev_point->valid || !curr_point->valid) continue;

                float prev_x, prev_y, curr_x, curr_y;
                map_lat_lon_to_screen(&map->view, prev_point->latitude, prev_point->longitude,
                                     canvas_size.x, canvas_size.y, &prev_x, &prev_y);
                map_lat_lon_to_screen(&map->view, curr_point->latitude, curr_point->longitude,
                                     canvas_size.x, canvas_size.y, &curr_x, &curr_y);

                prev_x += canvas_pos.x; prev_y += canvas_pos.y;
                curr_x += canvas_pos.x; curr_y += canvas_pos.y;

                ImDrawList_AddLine(draw_list, (ImVec2){prev_x, prev_y}, (ImVec2){curr_x, curr_y},
                                   track_color, map->view.track_width);

                // Her 20 noktada bir küçük işaret bırak
                if ((i % 20) == 0) {
                    ImDrawList_AddCircleFilled(draw_list, (ImVec2){curr_x, curr_y}, 2.0f, track_color, 8);
                }
            }
        }
        
        // Draw current position
        if (gps->position_valid) {
            float pos_x, pos_y;
            map_lat_lon_to_screen(&map->view, gps->latitude, gps->longitude,
                                 canvas_size.x, canvas_size.y, &pos_x, &pos_y);
            
            pos_x += canvas_pos.x;
            pos_y += canvas_pos.y;
            
            // Current position marker (larger circle)
            ImU32 pos_color = igGetColorU32_Vec4((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}); // Red
            ImDrawList_AddCircleFilled(draw_list, (ImVec2){pos_x, pos_y}, 8.0f, pos_color, 16);
            
            // Direction indicator
            if (gps->motion_valid && gps->speed_kmh > 1.0f) {
                float course_rad = gps->course * M_PI / 180.0f;
                float arrow_len = 15.0f;
                float end_x = pos_x + arrow_len * sinf(course_rad);
                float end_y = pos_y - arrow_len * cosf(course_rad);
                
                ImU32 arrow_color = igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 0.0f, 1.0f}); // Yellow
                ImDrawList_AddLine(draw_list, (ImVec2){pos_x, pos_y}, 
                                  (ImVec2){end_x, end_y}, arrow_color, 3.0f);
            }
            
            // Position text overlay
            char pos_text[64];
            snprintf(pos_text, sizeof(pos_text), "%.6f, %.6f", gps->latitude, gps->longitude);
            ImDrawList_AddText_Vec2(draw_list, 
                                   (ImVec2){canvas_pos.x + 10, canvas_pos.y + 10}, 
                                   igGetColorU32_Col(ImGuiCol_Text, 1.0f), 
                                   pos_text, NULL);
            
            // Speed and course overlay
            if (gps->motion_valid) {
                char motion_text[64];
                snprintf(motion_text, sizeof(motion_text), "%.1f km/h, %.0f°", 
                        gps->speed_kmh, gps->course);
                ImDrawList_AddText_Vec2(draw_list, 
                                       (ImVec2){canvas_pos.x + 10, canvas_pos.y + 30}, 
                                       igGetColorU32_Col(ImGuiCol_Text, 1.0f), 
                                       motion_text, NULL);
            }
        } else {
            // No GPS fix message
            const char* no_fix_text = "No GPS fix available";
            ImVec2 text_size;
            igCalcTextSize(&text_size, no_fix_text, NULL, false, -1.0f);
            ImVec2 text_pos = {
                canvas_pos.x + (canvas_size.x - text_size.x) * 0.5f,
                canvas_pos.y + (canvas_size.y - text_size.y) * 0.5f
            };
            ImDrawList_AddText_Vec2(draw_list, text_pos, 
                                   igGetColorU32_Vec4((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}), 
                                   no_fix_text, NULL);
        }
        
        // Handle mouse interaction
        if (igInvisibleButton("map_canvas", canvas_size, ImGuiButtonFlags_None)) {
            // Handle click events (future: add waypoints, etc.)
        }
        
        // Mouse wheel zoom
        ImGuiIO* io = cImGui_GetIO();
        if (igIsItemHovered(ImGuiHoveredFlags_None) && io->MouseWheel != 0.0f) {
            double zoom_factor = (io->MouseWheel > 0) ? 1.2 : (1.0 / 1.2);
            map_system_set_zoom(map, map->view.zoom_level * zoom_factor);
        }
        
        // Pan with middle mouse button
        if (igIsItemActive() && igIsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
            ImVec2 delta;
            igGetMouseDragDelta(&delta, ImGuiMouseButton_Middle, 0.0f);

            // Ekran deltası -> lat/lon farkı (ters dönüşüm)
            double lat0, lon0, lat1, lon1;
            map_screen_to_lat_lon(&map->view, 0.0f, 0.0f, canvas_size.x, canvas_size.y, &lat0, &lon0);
            map_screen_to_lat_lon(&map->view, -delta.x, -delta.y, canvas_size.x, canvas_size.y, &lat1, &lon1);

            map_system_set_center(map,
                                  map->view.center_lat + (lat1 - lat0),
                                  map->view.center_lon + (lon1 - lon0));
        }
}

void render_polar_window(app_state_t* app) {
    // Set default size for sky plot window
    igSetNextWindowSize((ImVec2){350, 350}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){LEFT_PANEL_WIDTH + 630, 30}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    
    if (igBegin("Sky Plot", NULL, ImGuiWindowFlags_None)) {
        render_polar_view_panel(app);
    }
    igEnd();
}

void render_polar_view_panel(app_state_t* app) {
    polar_view_t* polar = &app->polar_view;
    gps_data_t* gps = &app->gps_data;
        
        // Update polar view with current GPS data
        polar_view_update(polar, gps);
        
        // Statistics
        igText("Sky Plot - Satellite Positions");
        igText("Visible: %d | Used: %d", polar->satellites_visible, polar->satellites_used);
        if (polar->satellites_visible > 0) {
            igText("Average SNR: %.1f dB | Max SNR: %.1f dB", 
                   polar->average_snr, polar->max_snr);
        }
        
        igSeparator();
        
        // Polar plot canvas
        ImVec2 canvas_pos, canvas_size;
        igGetCursorScreenPos(&canvas_pos);
        igGetContentRegionAvail(&canvas_size);
        
        // Make it square
        float size = fminf(canvas_size.x, canvas_size.y) - 20.0f;
        if (size < 200.0f) size = 200.0f;
        
        float center_x = canvas_pos.x + size * 0.5f;
        float center_y = canvas_pos.y + size * 0.5f;
        float radius = size * 0.45f;
        
        polar_view_set_size(polar, center_x, center_y, radius);
        
        ImDrawList* draw_list = igGetWindowDrawList();
        
        // Background circle
        ImU32 bg_color = igGetColorU32_Col(ImGuiCol_ChildBg, 1.00f);
        ImDrawList_AddCircleFilled(draw_list, (ImVec2){center_x, center_y}, radius, bg_color, 64);
        
        // Grid circles (elevation)
        ImU32 grid_color = igGetColorU32_Vec4((ImVec4){0.4f, 0.4f, 0.4f, 0.6f});
        for (int elev = 30; elev <= 90; elev += 30) {
            float r = radius * (1.0f - elev / 90.0f);
            ImDrawList_AddCircle(draw_list, (ImVec2){center_x, center_y}, r, grid_color, 32, 1.0f);
        }
        
        // Grid lines (azimuth)
        for (int az = 0; az < 360; az += 30) {
            float angle_rad = (az - 90.0f) * M_PI / 180.0f;
            float end_x = center_x + radius * cosf(angle_rad);
            float end_y = center_y + radius * sinf(angle_rad);
            ImDrawList_AddLine(draw_list, (ImVec2){center_x, center_y}, 
                              (ImVec2){end_x, end_y}, grid_color, 1.0f);
        }
        
        // Compass labels
        ImU32 text_color = igGetColorU32_Col(ImGuiCol_Text, 1.0f);
        float label_radius = radius + 15.0f;
        
        const char* compass_labels[] = {"N", "E", "S", "W"};
        for (int i = 0; i < 4; i++) {
            float angle_rad = (i * 90.0f - 90.0f) * M_PI / 180.0f;
            float label_x = center_x + label_radius * cosf(angle_rad) - 6.0f;
            float label_y = center_y + label_radius * sinf(angle_rad) - 6.0f;
            ImDrawList_AddText_Vec2(draw_list, (ImVec2){label_x, label_y}, 
                                   text_color, compass_labels[i], NULL);
        }
        
        // Draw satellites
        for (int i = 0; i < polar->satellite_count; i++) {
            const satellite_visual_t* sat = &polar->satellites[i];
            
            if (!sat->visible) continue;
            
            // Color based on SNR
            ImVec4 sat_color;
            if (sat->snr >= 35) {
                sat_color = (ImVec4){0.0f, 1.0f, 0.0f, 1.0f}; // Green
            } else if (sat->snr >= 20) {
                sat_color = (ImVec4){1.0f, 1.0f, 0.0f, 1.0f}; // Yellow
            } else if (sat->snr > 0) {
                sat_color = (ImVec4){1.0f, 0.5f, 0.0f, 1.0f}; // Orange
            } else {
                sat_color = (ImVec4){0.5f, 0.5f, 0.5f, 1.0f}; // Gray
            }
            
            // Highlight if used in fix
            if (sat->used_in_fix) {
                sat_color.w = 1.0f;
                // Draw outer ring for used satellites
                ImU32 ring_color = igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 1.0f, 0.8f});
                ImDrawList_AddCircle(draw_list, (ImVec2){sat->screen_x, sat->screen_y}, 
                                    12.0f, ring_color, 16, 2.0f);
            }
            
            // Satellite circle
            ImU32 sat_color_u32 = igGetColorU32_Vec4(sat_color);
            float sat_radius = sat->used_in_fix ? 8.0f : 6.0f;
            ImDrawList_AddCircleFilled(draw_list, (ImVec2){sat->screen_x, sat->screen_y}, 
                                      sat_radius, sat_color_u32, 16);
            
            // PRN label
            char prn_text[8];
            snprintf(prn_text, sizeof(prn_text), "%d", sat->prn);
            
            ImVec2 text_size;
            igCalcTextSize(&text_size, prn_text, NULL, false, -1.0f);
            
            float text_x = sat->screen_x - text_size.x * 0.5f;
            float text_y = sat->screen_y - text_size.y * 0.5f;
            
            // Text background for readability
            ImU32 text_bg = igGetColorU32_Vec4((ImVec4){0.0f, 0.0f, 0.0f, 0.7f});
            ImDrawList_AddRectFilled(draw_list, 
                                    (ImVec2){text_x - 2, text_y - 1}, 
                                    (ImVec2){text_x + text_size.x + 2, text_y + text_size.y + 1}, 
                                    text_bg, 2.0f, ImDrawFlags_RoundCornersAll);
            
            ImDrawList_AddText_Vec2(draw_list, (ImVec2){text_x, text_y}, 
                                   igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 1.0f, 1.0f}), 
                                   prn_text, NULL);
        }
        
        // Outer border
        ImU32 border_color = igGetColorU32_Col(ImGuiCol_Border, 1.0f);
        ImDrawList_AddCircle(draw_list, (ImVec2){center_x, center_y}, radius, border_color, 64, 2.0f);
        
        // Handle mouse interaction
        ImVec2 mouse_pos;
        igGetMousePos(&mouse_pos);
        bool mouse_in_canvas = (mouse_pos.x >= canvas_pos.x && mouse_pos.x <= canvas_pos.x + size &&
                               mouse_pos.y >= canvas_pos.y && mouse_pos.y <= canvas_pos.y + size);
        
        if (mouse_in_canvas) {
            bool clicked = igIsMouseClicked_Bool(ImGuiMouseButton_Left, false);
            polar_view_handle_mouse(polar, mouse_pos.x, mouse_pos.y, clicked);
        }
        
        // Selected satellite info
        if (polar->selected_satellite >= 0 && polar->selected_satellite < polar->satellite_count) {
            const satellite_visual_t* sel_sat = &polar->satellites[polar->selected_satellite];
            igSeparator();
            igText("Selected Satellite: PRN %d", sel_sat->prn);
            igText("  Elevation: %.0f°", sel_sat->elevation);
            igText("  Azimuth: %.0f°", sel_sat->azimuth);
            igText("  SNR: %.0f dB", sel_sat->snr);
            igText("  Used in fix: %s", sel_sat->used_in_fix ? "Yes" : "No");
        }
        
        // Legend
        igSeparator();
        igText("Legend:");
        igTextColored((ImVec4){0.0f, 1.0f, 0.0f, 1.0f}, "●");
        igSameLine(0, 5);
        igText("Strong signal (≥35 dB)");
        
        igTextColored((ImVec4){1.0f, 1.0f, 0.0f, 1.0f}, "●");
        igSameLine(0, 5);
        igText("Good signal (≥20 dB)");

        igTextColored((ImVec4){1.0f, 0.5f, 0.0f, 1.0f}, "●");
        igSameLine(0, 5);
        igText("Weak signal (<20 dB)");

        igTextColored((ImVec4){0.5f, 0.5f, 0.5f, 1.0f}, "●");
        igSameLine(0, 5);
        igText("No signal");
        
        igText("○ White ring = Used in position fix");
        
        // Reserve space for the canvas
        igInvisibleButton("polar_canvas", (ImVec2){size, size}, ImGuiButtonFlags_None);
}

void render_compass_window(app_state_t* app) {
    // Set default size for compass window
    igSetNextWindowSize((ImVec2){350, 400}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){LEFT_PANEL_WIDTH + 320, 450}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    
    if (igBegin("Compass", NULL, ImGuiWindowFlags_None)) {
        render_compass_panel(app);
    }
    igEnd();
}

void render_compass_panel(app_state_t* app) {
    gps_data_t* gps = &app->gps_data;
    compass_t* compass = &app->compass;
    
    // Controls
    igText("Compass & Direction");
    igSeparator();
    
    igCheckbox("Auto-rotate with GPS", &compass->auto_rotate);
    float declination = compass->declination;
    if (igSliderFloat("Magnetic Declination", &declination, -30.0f, 30.0f, "%.1f°", ImGuiSliderFlags_None)) {
        compass_set_declination(compass, declination);
    }
    
    // Current heading display
    float current_heading = compass->heading;
    if (compass->auto_rotate && gps->motion_valid) {
        current_heading = gps->course;
    }
    
    igText("Current Heading: %.1f°", current_heading);
    igText("Magnetic Declination: %.1f°", compass->declination);
    igText("True Heading: %.1f°", compass_get_true_heading(compass));
    
    igSeparator();
    
    // Compass canvas
    ImVec2 canvas_pos, canvas_size;
    igGetCursorScreenPos(&canvas_pos);
    igGetContentRegionAvail(&canvas_size);
    
    // Make it square
    float size = fminf(canvas_size.x, canvas_size.y) - 20.0f;
    if (size < 200.0f) size = 200.0f;
    
    float center_x = canvas_pos.x + size * 0.5f;
    float center_y = canvas_pos.y + size * 0.5f;
    float radius = size * 0.45f;
    
    ImDrawList* draw_list = igGetWindowDrawList();
    
    // Background circle
    ImU32 bg_color = igGetColorU32_Col(ImGuiCol_ChildBg, 1.0f);
    ImDrawList_AddCircleFilled(draw_list, (ImVec2){center_x, center_y}, radius, bg_color, 64);
    
    // Outer ring
    ImU32 ring_color = igGetColorU32_Col(ImGuiCol_Border, 1.0f);
    ImDrawList_AddCircle(draw_list, (ImVec2){center_x, center_y}, radius, ring_color, 64, 3.0f);
    
    // Inner rings for reference
    ImU32 grid_color = igGetColorU32_Vec4((ImVec4){0.4f, 0.4f, 0.4f, 0.4f});
    ImDrawList_AddCircle(draw_list, (ImVec2){center_x, center_y}, radius * 0.7f, grid_color, 32, 1.0f);
    ImDrawList_AddCircle(draw_list, (ImVec2){center_x, center_y}, radius * 0.4f, grid_color, 32, 1.0f);
    
    // Cardinal direction lines
    for (int angle = 0; angle < 360; angle += 30) {
        float line_thickness = (angle % 90 == 0) ? 2.0f : 1.0f;
        ImU32 line_color = (angle % 90 == 0) ? 
            igGetColorU32_Vec4((ImVec4){0.8f, 0.8f, 0.8f, 1.0f}) : grid_color;
        
        float angle_rad = (angle - 90.0f) * M_PI / 180.0f;
        float start_r = (angle % 90 == 0) ? radius * 0.85f : radius * 0.9f;
        
        float start_x = center_x + start_r * cosf(angle_rad);
        float start_y = center_y + start_r * sinf(angle_rad);
        float end_x = center_x + radius * cosf(angle_rad);
        float end_y = center_y + radius * sinf(angle_rad);
        
        ImDrawList_AddLine(draw_list, (ImVec2){start_x, start_y}, 
                          (ImVec2){end_x, end_y}, line_color, line_thickness);
    }
    
    // Cardinal direction labels
    ImU32 text_color = igGetColorU32_Col(ImGuiCol_Text, 1.0f);
    float label_radius = radius + 15.0f;
    
    const char* compass_labels[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    for (int i = 0; i < 8; i++) {
        float angle_rad = (i * 45.0f - 90.0f) * M_PI / 180.0f;
        float label_x = center_x + label_radius * cosf(angle_rad) - 8.0f;
        float label_y = center_y + label_radius * sinf(angle_rad) - 8.0f;
        
        // Highlight N, E, S, W
        ImU32 label_color = (i % 2 == 0) ? 
            igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 0.0f, 1.0f}) : text_color;
        
        ImDrawList_AddText_Vec2(draw_list, (ImVec2){label_x, label_y}, 
                               label_color, compass_labels[i], NULL);
    }
    
    // Heading arrow (North indicator)
    float heading_rad = (current_heading - 90.0f) * M_PI / 180.0f;
    float arrow_length = radius * 0.8f;
    
    // Main arrow
    float arrow_end_x = center_x + arrow_length * cosf(heading_rad);
    float arrow_end_y = center_y + arrow_length * sinf(heading_rad);
    
    ImU32 arrow_color = igGetColorU32_Vec4((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}); // Red
    ImDrawList_AddLine(draw_list, (ImVec2){center_x, center_y}, 
                      (ImVec2){arrow_end_x, arrow_end_y}, arrow_color, 4.0f);
    
    // Arrow head
    float arrow_angle1 = heading_rad + (150.0f * M_PI / 180.0f);
    float arrow_angle2 = heading_rad - (150.0f * M_PI / 180.0f);
    float head_length = 20.0f;
    
    float head1_x = arrow_end_x + head_length * cosf(arrow_angle1);
    float head1_y = arrow_end_y + head_length * sinf(arrow_angle1);
    float head2_x = arrow_end_x + head_length * cosf(arrow_angle2);
    float head2_y = arrow_end_y + head_length * sinf(arrow_angle2);
    
    ImDrawList_AddLine(draw_list, (ImVec2){arrow_end_x, arrow_end_y}, 
                      (ImVec2){head1_x, head1_y}, arrow_color, 3.0f);
    ImDrawList_AddLine(draw_list, (ImVec2){arrow_end_x, arrow_end_y}, 
                      (ImVec2){head2_x, head2_y}, arrow_color, 3.0f);
    
    // Center dot
    ImU32 center_color = igGetColorU32_Vec4((ImVec4){1.0f, 1.0f, 1.0f, 1.0f});
    ImDrawList_AddCircleFilled(draw_list, (ImVec2){center_x, center_y}, 5.0f, center_color, 16);
    
    // Speed and course info overlay
    if (gps->motion_valid) {
        char info_text[128];
        snprintf(info_text, sizeof(info_text), "Speed: %.1f km/h", gps->speed_kmh);
        ImDrawList_AddText_Vec2(draw_list, 
                               (ImVec2){canvas_pos.x + 10, canvas_pos.y + 10}, 
                               text_color, info_text, NULL);
        
        snprintf(info_text, sizeof(info_text), "Course: %.1f°", gps->course);
        ImDrawList_AddText_Vec2(draw_list, 
                               (ImVec2){canvas_pos.x + 10, canvas_pos.y + 30}, 
                               text_color, info_text, NULL);
    }
    
    // Reserve space for the canvas
    igInvisibleButton("compass_canvas", (ImVec2){size, size}, ImGuiButtonFlags_None);
}

void render_raw_data_window(app_state_t* app) {
    // Set default size for raw data window
    igSetNextWindowSize((ImVec2){600, 300}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){LEFT_PANEL_WIDTH + 20, 450}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    
    if (igBegin("Raw Data Console", NULL, ImGuiWindowFlags_None)) {
        render_raw_data_panel(app);
    }
    igEnd();
}

void render_raw_data_panel(app_state_t* app) {
    gps_data_t* gps = &app->gps_data;
    console_t* console = &app->console;
    
    igText("GPS Device Configuration & Raw Data Monitor");
    igSeparator();
    
    // Connection info
    igText("Port: %s", gps->port_name);
    igSameLine(0, 20);
    igText("Baud: %d", gps->baud_rate);
    igSameLine(0, 20);
    igText("Status: %s", gps_status_to_string(gps->status));
    
    igSeparator();
    
    // Controls
    igCheckbox("Auto-scroll console", &console->auto_scroll);
    igSameLine(0, 20);
    
    if (igButton("Clear Console", (ImVec2){0, 0})) {
        console_clear(console);
    }
    
    igSameLine(0, 20);
    if (igButton("Request Version", (ImVec2){0, 0})) {
        if (gps_serial_send_command(&app->gps_serial, "$PMTK605*31")) {
            console_add_line(console, ">>> $PMTK605*31 (Request firmware version)");
        } else {
            console_add_line(console, "Error: Failed to send command");
        }
    }
    
    igSameLine(0, 20);
    if (igButton("Reset Config", (ImVec2){0, 0})) {
        if (gps_serial_send_command(&app->gps_serial, "$PMTK104*37")) {
            console_add_line(console, ">>> $PMTK104*37 (Cold restart)");
        } else {
            console_add_line(console, "Error: Failed to send command");
        }
    }
    
    igSeparator();
    
    // Raw data console (5 lines max)
    igText("Raw NMEA Data (Last 5 sentences):");
    
    ImGuiWindowFlags console_flags = ImGuiWindowFlags_HorizontalScrollbar;
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){5.0f, 5.0f});
    
    if (igBeginChild_Str("RawConsole", (ImVec2){0, 120}, ImGuiChildFlags_Borders, console_flags)) {
        // Remove font push since it's not supported in this CImGui version
        
        for (int i = 0; i < 5; i++) {
            const char* line = console_get_line(console, i);
            if (line) {
                // Color code different NMEA sentence types
                ImVec4 line_color = {0.8f, 0.8f, 0.8f, 1.0f}; // Default white
                
                if (strstr(line, "$GPRMC") || strstr(line, "$GNRMC")) {
                    line_color = (ImVec4){0.0f, 1.0f, 0.0f, 1.0f}; // Green for RMC
                } else if (strstr(line, "$GPGGA") || strstr(line, "$GNGGA")) {
                    line_color = (ImVec4){0.0f, 0.8f, 1.0f, 1.0f}; // Cyan for GGA
                } else if (strstr(line, "$GPGSV") || strstr(line, "$GNGSV")) {
                    line_color = (ImVec4){1.0f, 0.8f, 0.0f, 1.0f}; // Orange for GSV
                } else if (strstr(line, "$PMTK")) {
                    line_color = (ImVec4){1.0f, 0.0f, 1.0f, 1.0f}; // Magenta for MTK responses
                } else if (strstr(line, ">>>")) {
                    line_color = (ImVec4){1.0f, 1.0f, 0.0f, 1.0f}; // Yellow for commands
                }
                
                igTextColored(line_color, "%s", line);
            }
        }
        
        // Auto-scroll to bottom only if auto-scroll is enabled AND there's new data
        if (console->auto_scroll && console_has_new_data(console)) {
            igSetScrollHereY(1.0f);
            console_mark_data_consumed(console);
        }
    }
    igEndChild();
    igPopStyleVar(1);
    
    igSeparator();
    
    // Command input
    igText("Send Command to GPS:");
    igSetNextItemWidth(-100.0f);
    
    // Get mutable copy of command for input
    char command_buffer[256];
    strncpy(command_buffer, console_get_command(console), sizeof(command_buffer));
    command_buffer[255] = '\0';
    
    bool enter_pressed = igInputText("##command", command_buffer, sizeof(command_buffer), 
                                    ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL);
    
    // Update console command buffer
    console_set_command(console, command_buffer);
    
    igSameLine(0, 5);
    bool send_clicked = igButton("Send", (ImVec2){80, 0});
    
    if (enter_pressed || send_clicked) {
        const char* cmd = console_get_command(console);
        if (strlen(cmd) > 0) {
            if (gps_serial_send_command(&app->gps_serial, cmd)) {
                char cmd_line[300];
                snprintf(cmd_line, sizeof(cmd_line), ">>> %s", cmd);
                console_add_line(console, cmd_line);
            } else {
                console_add_line(console, "Error: Failed to send command");
            }
            console_clear_command(console);
        }
    }
    
    // Common GPS commands help
    igSeparator();
    igText("Common Commands:");
    igBulletText("$PMTK605*31 - Query firmware version");
    igBulletText("$PMTK104*37 - Cold restart");
    igBulletText("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28 - RMC+GGA only");
    igBulletText("$PMTK314,-1*04 - Restore default NMEA sentences");
    igBulletText("$PMTK220,1000*1F - Set update rate to 1Hz");
}

void render_gpx_export_dialog(app_state_t* app) {
    if (!app->show_gpx_export_dialog) return;
    
    bool open = app->show_gpx_export_dialog;
    if (igBegin("Export GPX", &open, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("Export track to GPX file");
        igSeparator();
        
        igText("Filename:");
        igInputText("##filename", app->gpx_filename, sizeof(app->gpx_filename), 
                   ImGuiInputTextFlags_None, NULL, NULL);
        
        igText("Track points: %d", app->map_system.track.point_count);
        igText("Distance: %.2f km", app->map_system.track.total_distance / 1000.0);
        
        igSeparator();
        
        if (igButton("Export", (ImVec2){80, 0})) {
            bool success = map_system_save_gpx(&app->map_system, app->gpx_filename);
            app->gpx_export_success = success;
            
            if (success) {
                snprintf(app->gpx_export_message, sizeof(app->gpx_export_message),
                        "GPX file exported successfully to %s", app->gpx_filename);
            } else {
                snprintf(app->gpx_export_message, sizeof(app->gpx_export_message),
                        "Failed to export GPX file: %s", app->gpx_filename);
            }
            
            app->show_gpx_export_dialog = false;
        }
        
        igSameLine(0, 10);
        if (igButton("Cancel", (ImVec2){80, 0})) {
            app->show_gpx_export_dialog = false;
        }
        
        // Show export result message
        if (strlen(app->gpx_export_message) > 0) {
            igSeparator();
            if (app->gpx_export_success) {
                igTextColored((ImVec4){0.0f, 1.0f, 0.0f, 1.0f}, "%s", app->gpx_export_message);
            } else {
                igTextColored((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}, "%s", app->gpx_export_message);
            }
        }
    }
    
    if (!open) {
        app->show_gpx_export_dialog = false;
    }
    
    igEnd();
}

void render_satellite_window(app_state_t* app) {
    // Set default size for satellite window 
    igSetNextWindowSize((ImVec2){300, 250}, ImGuiCond_FirstUseEver);
    igSetNextWindowPos((ImVec2){10, 450}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    
    if (igBegin("Satellites", NULL, ImGuiWindowFlags_None)) {
        render_satellite_panel(app);
    }
    igEnd();
}

void render_satellite_panel(app_state_t* app) {
    gps_data_t* gps = &app->gps_data;
        
        igText("Visible: %d | Used: %d", gps->satellites_visible, gps->satellites_used);
        igSeparator();
        
        if (gps->satellite_count > 0) {
            // Table header
            if (igBeginTable("satellites", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, (ImVec2){0, 0}, 0)) {
                igTableSetupColumn("PRN", ImGuiTableColumnFlags_WidthFixed, 50.0f, 0);
                igTableSetupColumn("Elev", ImGuiTableColumnFlags_WidthFixed, 50.0f, 0);
                igTableSetupColumn("Azim", ImGuiTableColumnFlags_WidthFixed, 50.0f, 0);
                igTableSetupColumn("SNR", ImGuiTableColumnFlags_WidthFixed, 80.0f, 0);
                igTableHeadersRow();
                
                for (int i = 0; i < gps->satellite_count; i++) {
                    satellite_info_t* sat = &gps->satellites[i];
                    
                    igTableNextRow(0, 0);
                    
                    igTableSetColumnIndex(0);
                    igText("%d", sat->prn);
                    
                    igTableSetColumnIndex(1);
                    igText("%d°", sat->elevation);
                    
                    igTableSetColumnIndex(2);
                    igText("%d°", sat->azimuth);
                    
                    igTableSetColumnIndex(3);
                    if (sat->snr > 0) {
                        // Color code SNR: Green > 35, Yellow 20-35, Red < 20
                        ImVec4 color;
                        if (sat->snr >= 35) {
                            color = (ImVec4){0.0f, 1.0f, 0.0f, 1.0f}; // Green
                        } else if (sat->snr >= 20) {
                            color = (ImVec4){1.0f, 1.0f, 0.0f, 1.0f}; // Yellow
                        } else {
                            color = (ImVec4){1.0f, 0.0f, 0.0f, 1.0f}; // Red
                        }
                        igTextColored(color, "%d dB", sat->snr);
                    } else {
                        igTextColored((ImVec4){0.5f, 0.5f, 0.5f, 1.0f}, "--");
                    }
                }
                
                igEndTable();
            }
        } else {
            igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, "No satellite data available");
        }
}

void render_status_bar(app_state_t* app) {
    ImGuiViewport* viewport = igGetMainViewport();
    ImVec2 status_pos = {viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - STATUS_BAR_HEIGHT};
    ImVec2 status_size = {viewport->WorkSize.x, STATUS_BAR_HEIGHT};
    
    igSetNextWindowPos(status_pos, ImGuiCond_Always, (ImVec2){0, 0});
    igSetNextWindowSize(status_size, ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                            ImGuiWindowFlags_NoCollapse;
    
    if (igBegin("Status Bar", NULL, flags)) {
        gps_data_t* gps = &app->gps_data;
        
        // Connection status
        const char* status_str = gps_status_to_string(gps->status);
        if (gps->status == GPS_STATUS_CONNECTED) {
            igTextColored((ImVec4){0.0f, 1.0f, 0.0f, 1.0f}, "[%s]", status_str);
        } else {
            igTextColored((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}, "[%s]", status_str);
        }
        
        igSameLine(0, 20);
        igText("Port: %s", gps->port_name);
        
        igSameLine(0, 20);
        igText("Fix: %s", gps_fix_quality_to_string(gps->fix_quality));
        
        igSameLine(0, 20);
        igText("Sats: %d", gps->satellites_used);
        
        igSameLine(0, 20);
        igText("Log: %s", gps->logging_enabled ? "ON" : "OFF");
        
        // FPS on the right
        ImVec2 fps_size;
        char fps_text[32];
        snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", cImGui_GetIO()->Framerate);
        igCalcTextSize(&fps_size, fps_text, NULL, false, -1.0f);
        igSameLine(viewport->WorkSize.x - fps_size.x - 20, -1);
        igText("%s", fps_text);
    }
    igEnd();
}

void render_connection_dialog(app_state_t* app) {
    // Show connection dialog when requested
    if (app->show_connection_dialog) {
        bool open = app->show_connection_dialog;
        igSetNextWindowSize((ImVec2){400, 300}, ImGuiCond_FirstUseEver);
        
        if (igBegin("GPS Connection", &open, ImGuiWindowFlags_AlwaysAutoResize)) {
            igText("Connect to GPS Device");
            igSeparator();
            
            // Port selection
            igText("Available Ports:");
            if (app->available_port_count > 0) {
                for (int i = 0; i < app->available_port_count; i++) {
                    bool is_selected = (strcmp(app->selected_port, app->available_ports[i]) == 0);
                    if (igRadioButton_Bool(app->available_ports[i], is_selected)) {
                        strcpy(app->selected_port, app->available_ports[i]);
                    }
                }
            } else {
                igTextColored((ImVec4){1.0f, 0.7f, 0.0f, 1.0f}, "No GPS devices found!");
                igText("Make sure your GPS device is connected via USB.");
            }
            
            igSeparator();
            
            // Baud rate selection
            igText("Baud Rate:");
            int current_baud_index = 1; // Default 9600
            const char* baud_options[] = {"4800", "9600", "19200", "38400", "57600", "115200"};
            const int baud_values[] = {4800, 9600, 19200, 38400, 57600, 115200};
            
            // Find current baud index
            for (int i = 0; i < 6; i++) {
                if (baud_values[i] == app->selected_baud) {
                    current_baud_index = i;
                    break;
                }
            }
            
            if (igCombo_Str_arr("##baud", &current_baud_index, baud_options, 6, -1)) {
                app->selected_baud = baud_values[current_baud_index];
            }
            
            igSeparator();
            
            // Auto-connect checkbox
            igCheckbox("Enable Auto-connect", &app->auto_connect_enabled);
            igText("Automatically connect to first available GPS device on startup");
            
            igSeparator();
            
            // Connection buttons
            bool can_connect = (strlen(app->selected_port) > 0 && !gps_serial_is_open(&app->gps_serial));
            
            if (!can_connect) igBeginDisabled(true);
            if (igButton("Connect", (ImVec2){80, 0})) {
                if (gps_serial_open(&app->gps_serial, app->selected_port, app->selected_baud)) {
                    app->gps_data.status = GPS_STATUS_CONNECTED;
                    strcpy(app->gps_data.port_name, app->selected_port);
                    app->gps_data.baud_rate = app->selected_baud;
                    snprintf(app->connection_status_text, sizeof(app->connection_status_text), 
                            "Connected to %s at %d baud", app->selected_port, app->selected_baud);
                    app->show_connection_dialog = false;
                } else {
                    snprintf(app->last_error, sizeof(app->last_error), 
                            "Failed to connect to %s", app->selected_port);
                }
            }
            if (!can_connect) igEndDisabled();
            
            igSameLine(0, 10);
            if (igButton("Refresh Ports", (ImVec2){100, 0})) {
                gps_serial_list_ports(app->available_ports, 16, &app->available_port_count);
                strcpy(app->selected_port, ""); // Clear selection
            }
            
            igSameLine(0, 10);
            if (igButton("Cancel", (ImVec2){80, 0})) {
                app->show_connection_dialog = false;
            }
            
            // Show error if any
            if (strlen(app->last_error) > 0) {
                igSeparator();
                igTextColored((ImVec4){1.0f, 0.0f, 0.0f, 1.0f}, "Error: %s", app->last_error);
            }
        }
        
        if (!open) {
            app->show_connection_dialog = false;
        }
        
        igEnd();
    }
    
    // Auto-connect logic (only runs once at startup)
    static bool tried_auto_connect = false;
    if (!tried_auto_connect && app->auto_connect_enabled && !gps_serial_is_open(&app->gps_serial)) {
        tried_auto_connect = true;
        
        char ports[16][256];
        int port_count;
        if (gps_serial_list_ports(ports, 16, &port_count) && port_count > 0) {
            // Try to connect to the first available port
            if (gps_serial_open(&app->gps_serial, ports[0], 9600)) {
                app->gps_data.status = GPS_STATUS_CONNECTED;
                strcpy(app->gps_data.port_name, ports[0]);
                app->gps_data.baud_rate = 9600;
                snprintf(app->connection_status_text, sizeof(app->connection_status_text), 
                        "Auto-connected to %s", ports[0]);
            }
        }
    }
}

void update_gps_data(app_state_t* app) {
    if (gps_serial_is_open(&app->gps_serial)) {
        // Use the new function that passes console to get raw NMEA data
        int processed = gps_serial_read_data_with_console(&app->gps_serial, &app->gps_data, &app->console);
        if (processed < 0) {
            // Error reading data
            gps_serial_close(&app->gps_serial);
            app->gps_data.status = GPS_STATUS_ERROR;
            strcpy(app->connection_status_text, "Connection Error");
        } else if (processed > 0) {
            // Update map system with new GPS data
            map_system_update(&app->map_system, &app->gps_data);
        }
    }
}

void ensure_data_directory(void) {
    struct stat st;
    
    // Check if data directory exists
    if (stat("data", &st) == -1) {
        // Create data directory if it doesn't exist
        if (mkdir("data", 0755) != 0) {
            printf("Warning: Could not create data directory\n");
        } else {

            printf("Created data directory\n");
        }
    }
}