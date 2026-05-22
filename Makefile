CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -pedantic -std=c99
CXXFLAGS = -Wall -Wextra -std=c++11
SDL2_FLAGS = $(shell sdl2-config --cflags --libs)

CIMGUI_DIR = src/cimgui
IMGUI_DIR = $(CIMGUI_DIR)/imgui
BACKEND_DIR = $(IMGUI_DIR)/backends

# C++ kaynakları (sadece imgui çekirdeği ve backend'leri)
CIMGUI_SOURCES = $(CIMGUI_DIR)/cimgui.cpp \
                 $(CIMGUI_DIR)/cimgui_impl.cpp \
                 $(IMGUI_DIR)/imgui.cpp \
                 $(IMGUI_DIR)/imgui_draw.cpp \
                 $(IMGUI_DIR)/imgui_tables.cpp \
                 $(IMGUI_DIR)/imgui_widgets.cpp \
                 $(BACKEND_DIR)/imgui_impl_sdl2.cpp \
                 $(BACKEND_DIR)/imgui_impl_opengl3.cpp

# C kaynakları
GPS_SOURCES = src/gps_gui.c src/include/minmea.c src/include/gps_data.c src/include/gps_serial.c src/include/gps_map.c src/include/gps_polar.c

INCLUDES = -I $(CIMGUI_DIR) \
           -I $(IMGUI_DIR) \
           -I $(BACKEND_DIR) \
           -I src/include

OPENGL_FLAGS = -framework OpenGL

all: gps_gui

gps_gui:
	g++ -Wall -Wextra -std=c++11 -I src/cimgui -I src/cimgui/imgui -I src/cimgui/imgui/backends -I src/include \
	src/gps_gui.c src/include/minmea.c src/include/gps_data.c src/include/gps_serial.c src/include/gps_map.c src/include/gps_polar.c src/include/gps_compass.c src/include/gps_console.c src/include/tools.c src/include/gps_tiles.c src/include/gps_tile_loader.c src/include/gps_mbtiles.c src/include/gps_poi.c \
	src/cimgui/cimgui.cpp src/cimgui/cimgui_impl.cpp \
	src/cimgui/imgui/imgui.cpp src/cimgui/imgui/imgui_draw.cpp \
	src/cimgui/imgui/imgui_tables.cpp src/cimgui/imgui/imgui_widgets.cpp \
	src/cimgui/imgui/imgui_demo.cpp \
	src/cimgui/imgui/backends/imgui_impl_sdl2.cpp \
	src/cimgui/imgui/backends/imgui_impl_opengl3.cpp \
	`sdl2-config --cflags --libs` -framework OpenGL -lsqlite3 -o gps_gui

clean:
	rm -f gps_gui

.PHONY: all clean