###### CONF ######

WINDOWS = true
#LINUX = true

RELEASE = true
#DEBUG = true
#SANITIZE = true

##################

CXX = clang++
CXXFLAGS_BASE = -std=c++20
CXXFLAGS = -Wall -Wextra $(CXXFLAGS_BASE)

ifdef SANITIZE
DEBUG = true
CXXFLAGS_BASE += -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
#LD_LIBS += -shared-libsan
endif
ifdef DEBUG
CXXFLAGS_BASE += -g
CXXFLAGS += -DDEBUG
endif
ifdef RELEASE
CXXFLAGS_BASE += -O3
LD_LIBS += -s
endif

ifdef WINDOWS
LD_LIBS += "$(shell cygpath --unix $(VULKAN_SDK))/Lib/vulkan-1.lib" -lglfw3 -lpthread
endif
ifdef LINUX
LD_LIBS += -lvulkan -lglfw -lpthread
endif

SRCD = src
ROSEED = $(SRCD)/Rosee
ROSEE_SRC = $(ROSEED)/Brush.cpp $(ROSEED)/Cmp.cpp $(ROSEED)/Map.cpp $(ROSEED)/Renderer.cpp $(ROSEED)/Vk.cpp
OBJ_DEP = $(ROSEED)/Vma.o $(ROSEED)/tinyobjloader.o $(ROSEED)/stb_image.o
SRC = $(SRCD)/main.cpp $(ROSEE_SRC)
OBJ = $(SRC:.cpp=.o)

GLSL_RT_FLAGS = --target-env spirv1.4

%.vert.spv: %.vert
	glslangValidator $< -V -o $@
%.frag.spv: %.frag
	glslangValidator $< -V -o $@
%.comp.spv: %.comp
	glslangValidator $< -V -o $@
%.rgen.spv: %.rgen
	glslangValidator $< -V $(GLSL_RT_FLAGS) -o $@
%.rint.spv: %.rint
	glslangValidator $< -V $(GLSL_RT_FLAGS) -o $@
%.rahit.spv: %.rahit
	glslangValidator $< -V $(GLSL_RT_FLAGS) -o $@
%.rchit.spv: %.rchit
	glslangValidator $< -V $(GLSL_RT_FLAGS) -o $@
%.rmiss.spv: %.rmiss
	glslangValidator $< -V $(GLSL_RT_FLAGS) -o $@
%.rcall.spv: %.rcall
	glslangValidator $< -V $(GLSL_RT_FLAGS) -o $@

SHAD = sha
SHA = $(SHAD)/opaque.vert $(SHAD)/opaque.frag $(SHAD)/opaque_uvgen.vert $(SHAD)/opaque_uvgen.frag \
	$(SHAD)/fwd_p2.vert $(SHAD)/color_resolve.frag $(SHAD)/depth_resolve.frag $(SHAD)/depth_resolve_ms.frag $(SHAD)/depth_acc.frag \
	$(SHAD)/potato.frag $(SHAD)/potato_ms.frag $(SHAD)/ssgi.frag $(SHAD)/ssgi_ms.frag \
	$(SHAD)/ray_tracing.rgen \
	$(SHAD)/wsi.frag

SHA_VERT = $(SHA:.vert=.vert.spv)
SHA_FRAG = $(SHA:.frag=.frag.spv)
SHA_COMP = $(SHA:.comp=.comp.spv)
SHA_RGEN = $(SHA:.rgen=.rgen.spv)
SHA_RINT = $(SHA:.rint=.rint.spv)
SHA_RAHIT = $(SHA:.rahit=.rahit.spv)
SHA_RCHIT = $(SHA:.rchit=.rchit.spv)
SHA_MISS = $(SHA:.rmiss=.rmiss.spv)
SHA_CALL = $(SHA:.rcall=.rcall.spv)
SHAS = $(filter-out $(SHA), $(SHA_VERT) $(SHA_FRAG) $(SHA_COMP) $(SHA_RGEN) $(SHA_RINT) $(SHA_RAHIT) $(SHA_RCHIT) $(SHA_MISS) $(SHA_CALL))

TARGET = rosee

all: $(TARGET) $(SHAS)

$(TARGET): $(OBJ) $(OBJ_DEP)
	$(CXX) $(CXXFLAGS) $(OBJ) $(OBJ_DEP) -o $(TARGET) $(LD_LIBS)

$(ROSEED)/Vma.o:
	$(CXX) $(CXXFLAGS_BASE) -Wno-nullability-completeness $(ROSEED)/Vma.cpp -c -o $(ROSEED)/Vma.o
$(ROSEED)/tinyobjloader.o:
	$(CXX) $(CXXFLAGS_BASE) $(ROSEED)/tinyobjloader.cpp -c -o $(ROSEED)/tinyobjloader.o
$(ROSEED)/stb_image.o:
	$(CXX) $(CXXFLAGS_BASE) $(ROSEED)/stb_image.cpp -c -o $(ROSEED)/stb_image.o

RELEASE_DIR = ../Rosee_releases
RELEASE_LATEST_DIR = $(RELEASE_DIR)/latest

release: $(TARGET) $(SHAS)
	rm -rf $(RELEASE_DIR)/latest
	cp -r $(RELEASE_DIR)/template $(RELEASE_LATEST_DIR)
	cp $(TARGET) $(RELEASE_LATEST_DIR)
	cp -r res $(RELEASE_LATEST_DIR)
	mkdir -p $(RELEASE_LATEST_DIR)/sha
	cp $(SHAS) $(RELEASE_LATEST_DIR)/sha

clean:
	rm -f $(OBJ) $(TARGET)

clean_sha:
	rm -f $(SHAS)

clean_dep:
	rm -f $(OBJ_DEP)

clean_all: clean clean_sha clean_dep
