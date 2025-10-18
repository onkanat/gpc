#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
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

// GPS modules
#include "include/gps_data.h"
#include "include/gps_serial.h"

// GUI Configuration
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define LEFT_PANEL_WIDTH 320
#define STATUS_BAR_HEIGHT 40

// Global application state
typedef struct {
    gps_data_t gps_data;
    gps_serial_t gps_serial;
    bool show_demo_window;
    bool auto_scroll_log;
    char connection_status_text[256];
    char last_error[512];
} app_state_t;

// Function declarations
void render_header_bar(app_state_t* app);
void render_telemetry_panel(app_state_t* app);
void render_map_panel(app_state_t* app);
void render_satellite_panel(app_state_t* app);
void render_status_bar(app_state_t* app);
void render_connection_dialog(app_state_t* app);
void update_gps_data(app_state_t* app);
void setup_imgui_style(void);

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
    app_state.show_demo_window = false;
    app_state.auto_scroll_log = true;
    strcpy(app_state.connection_status_text, "Not Connected");
    app_state.last_error[0] = '\0';

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

        // Submit the DockSpace
        ImGuiID dockspace_id = igGetID_Str("MyDockSpace");
        igDockSpace(dockspace_id, (ImVec2){0.0f, 0.0f}, ImGuiDockNodeFlags_None, NULL);

        // Render components
        render_header_bar(&app_state);
        render_telemetry_panel(&app_state);
        render_map_panel(&app_state);
        render_satellite_panel(&app_state);
        render_status_bar(&app_state);
        render_connection_dialog(&app_state);

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
                // Open connection dialog
            }
            if (igMenuItem_Bool("Disconnect", NULL, false, gps_serial_is_open(&app->gps_serial))) {
                gps_serial_close(&app->gps_serial);
                app->gps_data.status = GPS_STATUS_DISCONNECTED;
                strcpy(app->connection_status_text, "Disconnected");
            }
            igSeparator();
            if (igMenuItem_Bool("Auto-connect", NULL, false, true)) {
                // TODO: Implement auto-connect
            }
            igEndMenu();
        }
        
        // Tools menu
        if (igBeginMenu("Tools", true)) {
            if (igMenuItem_Bool("Start Logging", NULL, false, !app->gps_data.logging_enabled)) {
                char filename[256];
                time_t t = time(NULL);
                struct tm* tm_info = localtime(&t);
                strftime(filename, sizeof(filename), "gps_log_%Y%m%d_%H%M%S.nmea", tm_info);
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

void render_telemetry_panel(app_state_t* app) {
    igSetNextWindowSize((ImVec2){LEFT_PANEL_WIDTH, -1}, ImGuiCond_FirstUseEver);
    
    if (igBegin("Telemetry", NULL, ImGuiWindowFlags_None)) {
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
    igEnd();
}

void render_map_panel(app_state_t* app) {
    if (igBegin("Map View", NULL, ImGuiWindowFlags_None)) {
        gps_data_t* gps = &app->gps_data;
        
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
        
        if (gps->position_valid) {
            // Center point (current position)
            ImVec2 center = {canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f};
            ImU32 pos_color = igGetColorU32_Vec4((ImVec4){0.0f, 0.59f, 1.0f, 1.0f}); // Blue
            ImDrawList_AddCircleFilled(draw_list, center, 8.0f, pos_color, 16);
            
            // Position text
            char pos_text[64];
            snprintf(pos_text, sizeof(pos_text), "%.4f, %.4f", gps->latitude, gps->longitude);
            ImDrawList_AddText_Vec2(draw_list, 
                                   (ImVec2){canvas_pos.x + 10, canvas_pos.y + 10}, 
                                   igGetColorU32_Col(ImGuiCol_Text, 1.0f), 
                                   pos_text, NULL);
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
        
        // Invisible button for interaction
        igInvisibleButton("map_canvas", canvas_size, ImGuiButtonFlags_None);
    }
    igEnd();
}

void render_satellite_panel(app_state_t* app) {
    if (igBegin("Satellites", NULL, ImGuiWindowFlags_None)) {
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
    igEnd();
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
    // This would show a connection dialog when needed
    // For now, we'll implement a simple auto-connect for available ports
    
    static bool tried_auto_connect = false;
    if (!tried_auto_connect && !gps_serial_is_open(&app->gps_serial)) {
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
                        "Connected to %s", ports[0]);
            }
        }
    }
}

void update_gps_data(app_state_t* app) {
    if (gps_serial_is_open(&app->gps_serial)) {
        int processed = gps_serial_read_data(&app->gps_serial, &app->gps_data);
        if (processed < 0) {
            // Error reading data
            gps_serial_close(&app->gps_serial);
            app->gps_data.status = GPS_STATUS_ERROR;
            strcpy(app->connection_status_text, "Connection Error");
        }
    }
}